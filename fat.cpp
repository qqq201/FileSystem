#include "fat.h"

char const hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

long long default_sector_size = 512;

int FAT32::Sc = 0;
long long FAT32::Sb = 0;
int FAT32::Nf = 0;
long long FAT32::Sv = 0;
long long FAT32::Sf = 0;
const char* FAT32::disk = NULL;

string filter_name(string s){
	int end = s.length() - 1;
	int stop = 0;

	while(end > 0 && !isalpha(s[end]) && !isdigit(s[end])){
		--end;
	}

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

bool ReadSector(const char *disk, char*& buff, unsigned int sector) {
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

long long little_edian_char(char* data, int start, int nbyte){
	stringstream ss;
	for (int i = start + nbyte - 1; i >= start; --i){
		ss << hex_chars[ (data[i] & 0xF0) >> 4 ];
		ss << hex_chars[ (data[i] & 0x0F) >> 0 ];
	}

	char* p;
	return strtol(ss.str().c_str(), &p, 16);
}

long long little_edian_string(string data, int start, int nbyte){
	stringstream ss;
	for (int i = start + nbyte - 1; i >= start; --i){
		ss << hex_chars[ (data[i] & 0xF0) >> 4 ];
		ss << hex_chars[ (data[i] & 0x0F) >> 0 ];
	}
 
	char* p;
	return strtol(ss.str().c_str(), &p, 16);
}

static inline void ltrim(string &s) {
    s.erase(s.begin(), find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !isspace(ch);
    }));
}

static inline void rtrim(string &s) {
    s.erase(find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !isspace(ch);
    }).base(), s.end());
}

static inline void trim(string &s) {
    ltrim(s);
    rtrim(s);
}

Entry32* getEntry(string data, int extraEntries, string path) {
	Entry32* entry = NULL;

	if(((data[11 + extraEntries * 32] >> 5) & 1) == 1) {
		entry = new File32(data, extraEntries, path);
	}
	else if (((data[11 + extraEntries * 32] >> 4) & 1) == 1){
		entry = new Folder32(data, extraEntries, path);
	}

	entry->readEntry();

	return entry;
}

Entry32::Entry32(string dataEntry, int numExtraEntry, string rootName) {
	this->dataEntry = dataEntry;
	this->numExtraEntry = numExtraEntry;
	this->root = rootName;
}

void Entry32::readStatus() {
	if (dataEntry.length() > 31){
		for(int i = 0; i < 6; ++i)
			this->status.push_back((dataEntry[11 + numExtraEntry * 32] >> i) & 1);
	}
}

void Entry32::readClusters() {
	if (dataEntry.length() > 31){
		long long high = little_edian_string(this->dataEntry, 20 + this->numExtraEntry * 32, 2);
		long long low = little_edian_string(this->dataEntry, 26 + this->numExtraEntry * 32, 2);

		this->clusters = FAT32::read_fat(high * 65536 + low);
	}
}

void Entry32::readName() {
	if (dataEntry.length() > 31){
		if(this->numExtraEntry == 0) {
			this->name = this->dataEntry.substr(0, 8);
		}
		else {
			for(int extraEntry = this->numExtraEntry - 1; extraEntry >= 0 ; --extraEntry) {
				this->name += this->dataEntry.substr(1 + extraEntry * 32, 10);
				this->name += this->dataEntry.substr(14 + extraEntry * 32, 12);
				this->name += this->dataEntry.substr(28 + extraEntry * 32, 4);
			}
		}
	}

	this->name = filter_name(this->name);
	// trim(this->name);
}

void Entry32::print_info() {
	for (bool bit : status)
		if (bit)
			cout << setw(12) << "    1";
		else
			cout << setw(12) << "    0";
}

File32::File32(string dataEntry, int numExtraEntry, string rootName) : Entry32(dataEntry, numExtraEntry, rootName) {}

void File32::readExt() {
	if (dataEntry.length() > 31)
		this->ext = this->dataEntry.substr(8 + 32 * this->numExtraEntry, 3);
	// trim(this->ext);
}

void File32::readSize() {
	if (dataEntry.length() > 31){
		this->fileSize = little_edian_string(this->dataEntry, 28, 4);
	}
}

void File32::readEntry() {
	this->readName();
	this->readExt();
	this->readStatus();
	this->readSize();
	this->readClusters();
}

void File32::get_directory(int step){
	for (int i = 0; i < step; ++i)
		cout << "\t";
	cout << this->name << endl;
}

void File32::print_info() {
	cout << setw(50) << this->root + "/" + this->name + "." + this->ext;
	cout << setw(10) << this->fileSize / 1024.0;
	Entry32::print_info();
}

void File32::print_content() {
	for (long long cluster : this->clusters){
		int sector = FAT32::cluster_to_sector(cluster);
		for (int s = sector; s < sector + FAT32::Sc; ++s){
			char* buff = new char[512];
			if (ReadSector(FAT32::disk, buff, s)){
				for (int c = 0; c < 512; ++c)
					if (buff[c] != 0)
						cout << buff[c];
			}
		}
	}
}

bool File32::type(){
	return false;
}

Folder32::Folder32(string dataEntry, int numExtraEntry, string rootName) : Entry32(dataEntry, numExtraEntry, rootName) {}

void Folder32::set_as_root(vector<long long>& clusters){
	isRoot = true;
	this->clusters = clusters;
}

void Folder32::readEntry() {
	if (!isRoot){
		this->readName();
		this->readStatus();
		this->readClusters();
	}

	char* buff = new char[512];
	bool hasNextSector = true;
	string entryData;


	for (long long cluster : clusters){
		if (!hasNextSector)
			break;

		long long start_sector = FAT32::cluster_to_sector(cluster);
		long long end_sector = start_sector + FAT32::Sc;

		for (long long sector = start_sector; sector < end_sector && hasNextSector; ++sector){
			if(ReadSector(FAT32::disk, buff, sector)) {
				int extraEntries = 0;
				entryData = "";

				for(int byte = 64; byte < 512; byte += 32) {
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
						cout << endl;
						string path = this->root + "/" + this->name;
						this->entries.push_back(getEntry(entryData, extraEntries, path));
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

void Folder32::get_directory(int step){
	for (int i = 0; i < step; ++i)
		cout << "\t";
	cout << this->name << endl;

	for (Entry32* entry : entries){
		entry->get_directory(step + 1);
	}
}

void Folder32::print_info() {
	cout << setw(50) << this->root + "/" + this->name + "/";
	cout << setw(10) << " ";
	
	Entry32::print_info();
}

void Folder32::print_content(){
	for (Entry32* entry : this->entries){
		entry->print_info();
		cout << endl;
		if (entry->type()){
			entry->print_content();
			cout << endl;
		}
	}
}

FAT32::FAT32(const char* disk) { this->disk = disk; }

FAT32::~FAT32() {
	delete root;
	root = NULL;
}

long long FAT32::cluster_to_sector(long long cluster){
	return Sb + Nf * Sf + (cluster - 2) * Sc;
}

bool FAT32::read_bootsector(char* data){
	if((unsigned char)data[510] == 0x55 && (unsigned char)data[511] == 0xaa){
		default_sector_size = little_edian_char(data, 11, 2);
		Sb = little_edian_char(data, 14, 2);
		Nf = data[16];
		Sf = little_edian_char(data, 36, 4);

		long long rdet_first_cluster = little_edian_char(data, 44, 4);
		rdet_clusters = read_fat(rdet_first_cluster);

		Sv = little_edian_char(data, 32, 4);
		Sc = data[13];

		return true;
	}
	else {
		return false;
	}
}

void FAT32::get_bs_info(){
	cout << "Sector size: " << default_sector_size << " Bytes\n";
	cout << "Volume size: " << Sv / 2097152.0 << " GB\n";
	cout << "Boot sector size: " << FAT32::Sb << " Sectors\n";
	cout << "Number of fat: " << Nf << endl;
	cout << "Fat size: " << Sf << " Sectors\n";
	cout << "Sectors per cluster: " << Sc << endl;
	cout << "RDET first cluster: " << rdet_clusters[0] << endl;
}

void FAT32::read_rdet() {
	root = new Folder32("", 0, "~");
	root->set_as_root(rdet_clusters);
	root->readEntry();
}

void FAT32::get_rdet_info() {
	cout << "=========== RDET ===========\n";

	string status[6] = { "Read Only | ", "Hidden | ", "System | ", "Vol Label | ", "Directory | ", "Archive |" };
	cout << setw(50) << left << "Name";
	cout << setw(10) << "Size";

	for (int i = 0; i < 6; ++i)
		cout << setw(12) << status[i];
	cout << endl;

	this->root->print_content();
	cout << "=============================\n";
}

vector<long long> FAT32::read_fat(long long cluster){
	return {cluster};
}

void FAT32::print_tree(){
	root->get_directory(0);
}
