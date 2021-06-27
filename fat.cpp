#include "fat.h"

char const hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

WORD default_sector_size = 512;

int FAT32::Sc = 0;
WORD FAT32::Sb = 0;
int FAT32::Nf = 0;
DWORD FAT32::Sv = 0;
DWORD FAT32::Sf = 0;
const char* FAT32::disk = NULL;

string filter_name(string s, bool subentry){
	int end = s.length() - 1;

	while(end > 0 && (!isascii(s[end]) || s[end] == ' ')){
		--end;
	}

	//if has sub entries, cut the extension
	if (subentry){
		int temp = end;

		while (end > 0 && s[end] != '.')
			--end;

		--end;
		//if doesn't have extension, restore end
		if (end < 1)
			end = temp;

		string news;
		s = s.substr(0, end + 1);
		//filter null char
		for (char c : s){
			if (c)
				news += c;
		}

		return news;
	}
	else
		return s.substr(0, end + 1);
}

bool get_logical_disks(string& disks){
	DWORD mydrives = 64;// buffer length
	char lpBuffer[64] {0};// buffer for drive string storage

	if (GetLogicalDriveStrings(mydrives, lpBuffer)){
		for(int i = 0; i < 50; i++){
			if (isalpha(lpBuffer[i]))
				disks += lpBuffer[i];
		}
		return (disks.length() > 0);
	}
	else
		return false;
}

bool ReadSector(const char *disk, char*& buff, DWORD sector) {
	DWORD dwRead;
	HANDLE hDisk = CreateFile(disk,GENERIC_READ,FILE_SHARE_VALID_FLAGS,0,OPEN_EXISTING,0,0);
	if(hDisk == INVALID_HANDLE_VALUE){
		CloseHandle(hDisk);
		return false;
	}

	SetFilePointer(hDisk, sector * default_sector_size, 0, FILE_BEGIN);
	ReadFile(hDisk, buff, default_sector_size, &dwRead, 0);
	CloseHandle(hDisk);
	return true;
}

void print_sector(char*& buff){
	for (int i = 0; i < default_sector_size; ++i){
		if (i % 16 == 0){
			cout << setfill('0') << setw(7) << hex_chars[ ((i / 16) & 0xF0) >> 4 ] << hex_chars[ ((i / 16) & 0x0F) >> 0 ] << "0: ";
		}

		cout << hex_chars[ (buff[i] & 0xF0) >> 4 ];
		cout << hex_chars[ (buff[i] & 0x0F) >> 0 ] << " ";

		if ((i + 1) % 16 == 0){
			cout << "   ";
			for (int j = i - 15; j <= i; ++j){
				if (isalpha(buff[j]) || isdigit(buff[j]))
					cout << buff[j];
				else
					cout << ".";
			}
			cout << endl;
		}
	}
	cout << endl;
}

unsigned long long little_edian_char(char* data, int start, int nbyte){
	stringstream ss;
	for (int i = start + nbyte - 1; i >= start; --i){
		ss << hex_chars[ (data[i] & 0xF0) >> 4 ];
		ss << hex_chars[ (data[i] & 0x0F) >> 0 ];
	}

	char* p;
	return strtol(ss.str().c_str(), &p, 16);
}

unsigned long long little_edian_string(string data, int start, int nbyte){
	stringstream ss;
	for (int i = start + nbyte - 1; i >= start; --i){
		ss << hex_chars[ (data[i] & 0xF0) >> 4 ];
		ss << hex_chars[ (data[i] & 0x0F) >> 0 ];
	}

	char* p;
	return strtol(ss.str().c_str(), &p, 16);
}

Entry32* getEntry(string data, int extraEntries, Entry32* parent, string path) {
	Entry32* entry = NULL;

	if(((data[11 + extraEntries * 32] >> 5) & 1) == 1) {
		entry = new File32(path, parent);
	}
	else if (((data[11 + extraEntries * 32] >> 4) & 1) == 1){
		entry = new Folder32(path, parent);
	}

	entry->readEntry(data, extraEntries);

	return entry;
}

Entry32::Entry32(string rootName, Entry32* parent) {
	this->root = rootName;
	this->parent = parent;
}

void Entry32::readStatus(string dataEntry, int numExtraEntry) {
	if (dataEntry.length() > 31){
		for(int i = 0; i < 6; ++i)
			this->status.push_back((dataEntry[11 + numExtraEntry * 32] >> i) & 1);
	}
}

void Entry32::readState(string dataEntry, int numExtraEntry) {
	if(dataEntry.length() > 31){
		if(little_edian_string(dataEntry, 0 + numExtraEntry * 32, 1) == 229)
			this->isDeleted = true;
		else
			this->isDeleted = false;
	}
}

void Entry32::readClusters(string dataEntry, int numExtraEntry) {
	if (dataEntry.length() > 31){
		DWORD high = little_edian_string(dataEntry, 20 + numExtraEntry * 32, 2);
		DWORD low = little_edian_string(dataEntry, 26 + numExtraEntry * 32, 2);

		if (this->isDeleted)
			this->clusters = FAT32::read_fat(high * 65536 + low);
	}
}

void Entry32::readName(string dataEntry, int numExtraEntry) {
	if (dataEntry.length() > 31){
		if(numExtraEntry == 0) {
			this->name = dataEntry.substr(0, 8);
			this->name = filter_name(this->name, false);
		}
		else {
			for(int extraEntry = numExtraEntry - 1; extraEntry >= 0 ; --extraEntry) {
				this->name += dataEntry.substr(1 + extraEntry * 32, 10);
				this->name += dataEntry.substr(14 + extraEntry * 32, 12);
				this->name += dataEntry.substr(28 + extraEntry * 32, 4);
			}
			this->name = filter_name(this->name, true);
		}
	}
}

void Entry32::readDate(string dataEntry, int numExtraEntry) {
	WORD createdTime = little_edian_string(dataEntry, 14 + 32 * numExtraEntry, 2);
	WORD createdDate = little_edian_string(dataEntry, 16 + 32 * numExtraEntry, 2);
	WORD modifiedTime = little_edian_string(dataEntry, 22 + 32 * numExtraEntry, 2);
	WORD modifiedDate = little_edian_string(dataEntry, 24 + 32 * numExtraEntry, 2);

	this->created.day = createdDate & 0x1F;
	this->created.month = (createdDate >> 5) & 0xF;
	this->created.year = (createdDate >> 9) + 1980;

	this->created.hour = createdTime >> 11;
	this->created.minute = (createdTime >> 5) & 0x3F;
	this->created.second = (createdTime&0x1F) << 1;

	this->modified.day = modifiedDate & 0x1F;
	this->modified.month = (modifiedDate >> 5) & 0xF;
	this->modified.year = (modifiedDate >> 9) + 1980;

	this->modified.hour = modifiedTime >> 11;
	this->modified.minute = (modifiedTime >> 5) & 0x3F;
}

string add_space_right(string str, int len){
	for (int i = len - str.length(); i > 0; --i)
		str += ' ';
	return str;
}

string add_space_left(string str, int len){
	if (len > str.length()){
		string new_str(len - str.length(), ' ');
		return new_str + str;
	}
	else
		return "";
}

File32::File32(string rootName, Entry32* parent) : Entry32(rootName, parent) {}

void File32::readExt(string dataEntry, int numExtraEntry) {
	if (dataEntry.length() > 31)
		this->ext = dataEntry.substr(8 + 32 * numExtraEntry, 3);
}

void File32::readSize(string dataEntry) {
	if (dataEntry.length() > 31){
		this->size = little_edian_string(dataEntry, 28, 4);
	}
}

void File32::readEntry(string dataEntry, int numExtraEntry) {
	this->readName(dataEntry, numExtraEntry);
	this->readExt(dataEntry, numExtraEntry);
	this->readStatus(dataEntry, numExtraEntry);
	this->readSize(dataEntry);
	this->readState(dataEntry, numExtraEntry);
	this->readClusters(dataEntry, numExtraEntry);
	this->readDate(dataEntry, numExtraEntry);
}

Entry32* File32::get_entry(int id){
	return NULL;
}

string File32::get_path(){
	return this->root + this->name + "." + this->ext;
}

vector<Entry32*> File32::get_directory(){
	return {};
}

string File32::get_info() {
	string info, date;
	info = add_space_right(this->name + "." + this->ext, 60);

	stringstream ss;
	ss << this->modified.day << "/" << this->modified.month << "/" << this->modified.year << " " << this->modified.hour << ":" << this->modified.minute;
	date = add_space_right(ss.str(), 30);

	info += date;
	info += add_space_right(this->ext, 30);

	ss.clear();

	if (this->size > 0){
		ss << this->size << " KB";
		info += add_space_left(ss.str(), 20);
	}

	return info;
}

string File32::get_content() {
	stringstream ss;
	for (long long cluster : this->clusters){
		int sector = FAT32::cluster_to_sector(cluster);
		for (int s = sector; s < sector + FAT32::Sc; ++s){
			char* buff = new char[512];
			if (ReadSector(FAT32::disk, buff, s)){
				for (int c = 0; c < 512; ++c)
					if (buff[c] != 0)
						ss << buff[c];
			}
		}
	}
	return ss.str();
}

bool File32::type(){
	return false;
}

Folder32::Folder32(string rootName, Entry32* parent) : Entry32(rootName, parent) {}

void Folder32::set_as_root(vector<DWORD>& clusters){
	isRoot = true;
	this->clusters = clusters;
	this->name += FAT32::disk[4];
	this->name += ":";
}

void Folder32::readEntry(string dataEntry, int numExtraEntry) {
	if (!isRoot){
		this->readName(dataEntry, numExtraEntry);
		this->readStatus(dataEntry, numExtraEntry);
		this->readClusters(dataEntry, numExtraEntry);
		this->readDate(dataEntry, numExtraEntry);
		this->readState(dataEntry, numExtraEntry);
	}

	char* buff = new char[512];
	bool hasNextSector = true;
	bool first_sector = true;

	string entryData;
	int extraEntries = 0;

	for (DWORD cluster : clusters){
		if (!hasNextSector)
			break;

		DWORD start_sector = FAT32::cluster_to_sector(cluster);
		DWORD end_sector = start_sector + FAT32::Sc;

		for (DWORD sector = start_sector; sector < end_sector && hasNextSector; ++sector){
			if(ReadSector(FAT32::disk, buff, sector)) {
				for(int byte = (first_sector)? 64 : 0; byte < 512; byte += 32) {
					if (first_sector)
						first_sector = false;

					for(int i = 0 ; i < 32 ; ++i) {
						entryData += buff[byte + i];
					}

					//* Check extra entry
					int offset0B = entryData[11 + extraEntries * 32];

					if(offset0B == 15) {
						extraEntries++;
					}
					else if(offset0B == 0) {
						hasNextSector = false;
					}
					else if(offset0B < 10) {
						continue;
					}
					else {
						string path = this->root + this->name + "/";
						// print_sector(buff);
						this->entries.push_back(getEntry(entryData, extraEntries, this, path));
						entryData = "";
						extraEntries = 0;
					}
				}
			}
			else
				hasNextSector = false;
		}
	}
}

bool Folder32::type(){
	return true;
}

Entry32* Folder32::get_entry(int id){
	if (id < entries.size())
		return entries[id];
	else
		return NULL;
}

string Folder32::get_path(){
	return this->root + this->name;
}

vector<Entry32*> Folder32::get_directory(){
	return entries;
}

string Folder32::get_content() {
	return "";
}

string Folder32::get_info() {
	string info, date;
	info = add_space_right(this->name, 60);

	stringstream ss;
	ss << this->modified.day << "/" << this->modified.month << "/" << this->modified.year << " " << this->modified.hour << ":" << this->modified.minute;
	date = add_space_right(ss.str(), 30);

	info += date;
	info += add_space_right("File folder", 30);
	return info;
}

FAT32::FAT32(const char* disk) { this->disk = disk; }

FAT32::~FAT32() {
	delete root;
	root = NULL;
}

DWORD FAT32::cluster_to_sector(DWORD cluster){
	return Sb + Nf * Sf + (cluster - 2) * Sc;
}

bool FAT32::read_bootsector(char* data){
	if((unsigned char)data[510] == 0x55 && (unsigned char)data[511] == 0xaa){
		default_sector_size = little_edian_char(data, 11, 2);
		Sb = little_edian_char(data, 14, 2);
		Nf = data[16];
		Sf = little_edian_char(data, 36, 4);

		DWORD rdet_first_cluster = little_edian_char(data, 44, 4);
		rdet_clusters = read_fat(rdet_first_cluster);

		Sv = little_edian_char(data, 32, 4);
		Sc = data[13];

		return true;
	}
	else {
		return false;
	}
}

string FAT32::get_bs_info(){
	stringstream ss;
	ss << "Sector size: " << default_sector_size << " Bytes\n";
	ss << "Volume size: " << Sv / 2097152.0 << " GB\n";
	ss << "Boot sector size: " << FAT32::Sb << " Sectors\n";
	ss << "Number of fat: " << Nf << endl;
	ss << "Fat size: " << Sf << " Sectors\n";
	ss << "Sectors per cluster: " << Sc << endl;
	ss << "RDET first cluster: " << rdet_clusters[0] << endl;
	return ss.str();
}

void FAT32::read_rdet() {
	root = new Folder32("", NULL);
	root->set_as_root(rdet_clusters);
	root->readEntry("", 0);
	current_directory = root;

	char* buff = new char[512];
	ReadSector(disk, buff, Sb);
	print_sector(buff);
}

Entry32* FAT32::back_parent_directory(){
	if (this->current_directory != this->root)
		this->current_directory = this->current_directory->parent;
	return this->current_directory;
}

Entry32* FAT32::change_directory(int id){
	Entry32* entry = this->current_directory->get_entry(id);
	if (entry)
		this->current_directory = entry;
	return entry;
}

Entry32* FAT32::get_current_directory() {
	return this->current_directory;
}

string FAT32::get_cd_path(){
	return this->current_directory->get_path();
}

vector<DWORD> FAT32::read_fat(DWORD clus){
	DWORD sectorSize = default_sector_size / 4;
	DWORD maxCluster = sectorSize * Sf;
	if (clus < maxCluster){
		DWORD cluster = clus;

		vector<DWORD> clusters;

		DWORD sector = 0;
		char* buff = NULL;

		do {
			clusters.push_back(cluster);
			buff = new char[512];
			if(sector != cluster / sectorSize) {
				sector = cluster / sectorSize + Sb;
				ReadSector(disk, buff, sector);
			}

			cluster = little_edian_char(buff, (cluster % sectorSize) * 4, 4);
		} while(cluster != 268435447 && cluster != 268435455 && cluster < maxCluster); //OFFFFFF7 & 0FFFFFFF & max cluster
		return clusters;
	}
	else {
		return {};
	}
}
