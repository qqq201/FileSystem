#include "fat.h"

char const hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

WORD default_sector_size = 512;

int FAT32::Sc = 8;
WORD FAT32::Sb = 0;
int FAT32::Nf = 2;
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

bool ReadSector(const char *disk, char*& buff, unsigned long long sector, unsigned long long nsector) {
	HANDLE hDisk = CreateFile(disk, GENERIC_READ, FILE_SHARE_VALID_FLAGS, 0, OPEN_EXISTING, 0, 0);
	DWORD dwRead;
	if(hDisk == INVALID_HANDLE_VALUE){
		CloseHandle(hDisk);
		return false;
	}

	ULARGE_INTEGER location;
	location.QuadPart = sector * default_sector_size;
	DWORD low = location.LowPart;
	LONG high = location.HighPart;

	SetFilePointer(hDisk, low, (PLONG)&high, FILE_BEGIN); // which sector to read
	ReadFile(hDisk, buff, default_sector_size * nsector, &dwRead, 0);
	CloseHandle(hDisk);
	return true;
}

void print_sector(char*& buff, int n){
	for (int i = 0; i < default_sector_size * n; ++i){
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

Entry32::~Entry32(){
	parent = NULL;
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

		//if (!this->isDeleted)
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

	this->created.wDay = createdDate & 0x1F;
	this->created.wMonth = (createdDate >> 5) & 0xF;
	this->created.wYear = (createdDate >> 9) + 1980;

	this->created.wHour = createdTime >> 11;
	this->created.wMinute = (createdTime >> 5) & 0x3F;
	this->created.wSecond = (createdTime & 0x1F) << 1;

	this->modified.wDay = modifiedDate & 0x1F;
	this->modified.wMonth = (modifiedDate >> 5) & 0xF;
	this->modified.wYear = (modifiedDate >> 9) + 1980;

	this->modified.wHour = modifiedTime >> 11;
	this->modified.wMinute = (modifiedTime >> 5) & 0x3F;
}

string Entry32::get_modified_date(){
	stringstream ss;
	ss << this->modified.wDay << "/" << this->modified.wMonth << "/" << this->modified.wYear << " " << this->modified.wHour << ":";
	if (this->modified.wMinute < 10)
		ss << "0" << this->modified.wMinute;
	else
		ss << this->modified.wMinute;
	return ss.str();
}

vector<string> Entry32::get_info(){
	vector<string> info;

	info.push_back(this->name);

	info.push_back(get_modified_date());
	if (type())
		info.push_back("File folder");
	else
		info.push_back(this->ext);

	info.push_back(get_size());

	return info;
}

float round(float var){
	int value = (int)(var * 100);
  return (float)value / 100;
}

string Entry32::get_size(){
	if (this->type())
		return "";
	else if (this->size > 0){
		vector<string> unit = {" Bytes", " KB", " MB", " GB"};
		float temp = this->size;
		stringstream ss;

		for (int i = 0; i < 4; ++i){
			if (to_string(int(temp)).length() < 5){
				ss << round(temp) << unit[i];
				return ss.str();
			}
			temp /= 1024.0;
		}

		ss << round(temp) << " TB";
		return ss.str();
	}
	else {
		return "0 Bytes";
	}
}

File32::File32(string rootName, Entry32* parent) : Entry32(rootName, parent) {}

File32::~File32(){}

void File32::readExt(string dataEntry, int numExtraEntry) {
	if (dataEntry.length() > 31)
		this->ext = dataEntry.substr(8 + 32 * numExtraEntry, 3);
}

void File32::readSize(string dataEntry) {
	if (dataEntry.length() > 31 && this->clusters.size() > 0){
		this->size = little_edian_string(dataEntry, 28, 4);
	}
}

void File32::readEntry(string dataEntry, int numExtraEntry) {
	this->readName(dataEntry, numExtraEntry);
	this->readExt(dataEntry, numExtraEntry);
	this->readStatus(dataEntry, numExtraEntry);
	this->readState(dataEntry, numExtraEntry);
	this->readClusters(dataEntry, numExtraEntry);
	this->readSize(dataEntry);
	this->readDate(dataEntry, numExtraEntry);
}

void File32::clear(){}

Entry32* File32::get_entry(int id){
	return NULL;
}

string File32::get_path(){
	return this->root + this->name + "." + this->ext;
}

vector<Entry32*> File32::get_directory(){
	return {};
}

string File32::get_content() {
	if (this->ext.compare("TXT") == 0){
		string str;
		bool stop = false;
		for (long long cluster : this->clusters){
			if (stop)
				break;

			int sector = FAT32::cluster_to_sector(cluster);
			for (int s = sector; s < sector + FAT32::Sc && !stop; ++s){
				char* buff = new char[512];
				if (ReadSector(FAT32::disk, buff, s)){
					for (int c = 0; c < 512 && !stop; ++c)
						if (buff[c] != 0)
							str.push_back(buff[c]);
						else
							stop = true;
				}
				delete buff;
			}
		}
		return str;
	}
	else {
		return "-- SORRY, CURRENTLY WE CAN ONLY OPEN TEXT (TXT) FILES --";
	}
}

bool File32::type(){
	return false;
}

Folder32::Folder32(string rootName, Entry32* parent) : Entry32(rootName, parent) {}

Folder32::~Folder32(){
	this->clear();
}

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

void Folder32::clear(){
	for (Entry32* entry : entries){
		if (entry->type())
			entry->clear();
		else {
			delete entry;
			entry = NULL;
		}
	}
	entries.clear();
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

FAT32::FAT32(const char* disk) { this->disk = disk; }

FAT32::~FAT32() {
	root->clear();
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
	ss << "Boot sector size: " << Sb << " Sectors\n";
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

vector<DWORD> FAT32::read_fat(DWORD cluster){
	DWORD sectorSize = default_sector_size / 4;
	DWORD maxCluster = sectorSize * Sf;

	if (cluster < maxCluster && cluster > 0){
		vector<DWORD> clusters;

		DWORD sector = 0;
		char* buff = new char[512];

		do {
			clusters.push_back(cluster);
			if(sector != cluster / sectorSize + Sb) {
				delete[] buff;
				buff = new char[512];
				sector = cluster / sectorSize + Sb;
				ReadSector(disk, buff, sector);
			}

			cluster = little_edian_char(buff, (cluster % sectorSize) * 4, 4);
		} while(cluster != 268435447 && cluster != 268435455 && cluster < maxCluster && cluster > 0); //OFFFFFF7 & 0FFFFFFF & max cluster
		return clusters;
	}
	else {
		return {};
	}
}


/*---------------------  NTFS  ----------------------*/
const char* NTFS::disk = NULL;
int NTFS::Sc = 8;
unsigned long long NTFS::Sv = 0;
unsigned long long NTFS::mft_cluster = 0;
int NTFS::size_mft_entry = 2;

vector<unsigned long long> read;

signed long long TwoElement(unsigned long long num){
  // Find binary vector for number
  vector<int> bit_vector;

  while (num >  0) {
    bit_vector.push_back(num % 2);
    num /= 2;
  }
	reverse(bit_vector.begin(), bit_vector.end());

  // substract -1 from binary
  bool borrow_bit = 0;
  if (bit_vector.back() == 0){
      borrow_bit =1;
      bit_vector.back() = 1;
  }
	else{
      bit_vector.back() = 0;
  }

  int i = bit_vector.size() - 2;

  while (borrow_bit != 0) {
    if(bit_vector[i] == 0){
      borrow_bit = 1;
      bit_vector[i] = 1;
      i = i - 1;
    }
		else{
      borrow_bit = 0;
      bit_vector[i] = 0;
    }
  }

  // XOR
  vector<int> vt;
  for(int j = 0; j < bit_vector.size(); ++j){
      if(bit_vector[j] == 1) vt.push_back(0);
      else vt.push_back(1);
  }

  signed long long result = 0;
  int m = 0;
  for(int i = vt.size() - 1; i >= 0; --i){
    result += vt[i] * (1 << m);
    ++m;
  }

  return -1 * result;
}

EntryNTFS::EntryNTFS(unsigned long long mft_id, string path, EntryNTFS* parent) {
	this->mft_id = mft_id;
	this->parent = parent;
	this->root = path;
}

EntryNTFS::~EntryNTFS(){
	for (auto entry : entries)
		delete entry;
	entries.clear();
	parent = NULL;
}

void EntryNTFS::read_index_buffer(){
	if (is_nonresident){
		for (auto cluster : clusters){
			//index header 24 bytes
			int ih_start_offset = 0;
			int nh_start_offset = 24;

			// check if this is an index buffer
			char* buff = new char[cluster.second];
			ReadSector(NTFS::disk, buff, cluster.first * NTFS::Sc, cluster.second / default_sector_size);

			string indx;
			for (int i = ih_start_offset; i < 4; ++i)
				indx.push_back(buff[i]);

			if (indx != "INDX")
				continue;

			DWORD eh_start_offset = little_edian_char(buff, nh_start_offset, 4) + nh_start_offset;
			DWORD index_node_length = little_edian_char(buff, nh_start_offset + 4, 4) - 16 - eh_start_offset + nh_start_offset;
			int offset_attribute_content = 16;

			while (index_node_length > 16){
				unsigned long long child_mft_id = little_edian_char(buff, eh_start_offset, 6);
				WORD index_length = little_edian_char(buff, eh_start_offset + 8, 2);
				if (index_length > 50 && *find(read.begin(), read.end(), child_mft_id) != child_mft_id){
					EntryNTFS* new_file = new EntryNTFS(child_mft_id, this->root + this->name + "/", this);
					if (new_file->read_content())
						entries.push_back(new_file);
					else
						delete new_file;
				}

				eh_start_offset += index_length;
				index_node_length -= index_length;
			}
			delete[] buff;
		}
	}
}

string to_normal_string(string s){
	string newstr;
	for (char c : s){
		if (c)
			newstr.push_back(c);
	}
	return newstr;
}

void EntryNTFS::file_name_attribute(char* buff) {
	if (isRoot){
		this->name = NTFS::disk[4];
	}
	else{
		DWORD start_offset = attributes[2].first;
		DWORD length = attributes[2].second;

		int offset_start_content_filename = little_edian_char(buff, start_offset + 20, 2) + start_offset;
		int length_filename = little_edian_char(buff, offset_start_content_filename + 64, 1)*2;
		int offset_start_name =  offset_start_content_filename + 66;
		int offset_end_name =  offset_start_name + length_filename;

		for (int i = offset_start_name; i < offset_end_name; i++)
			name.push_back(buff[i]);

		if (!type()){
			int pos = name.length() - 1;
			while (name[pos] != '.' && pos > 0)
				--pos;

			if (pos > 0){
				this->ext = to_normal_string(this->name.substr(pos + 1, name.length() - pos));
				this->name = to_normal_string(this->name.substr(0, pos));
			}
			else {
				this->name = to_normal_string(this->name);
			}
		}
		else {
			this->name = to_normal_string(this->name);
		}
	}
}

void EntryNTFS::standard_info_attribute(char* buff){
	DWORD start_offset = attributes[0].first;
	DWORD length = attributes[0].second;

	int offset_creation_time = little_edian_char(buff, start_offset + 20, 2) + start_offset;
	int offset_modified_time = offset_creation_time + 8;

	FILETIME ft = { 0 };
	ft.dwHighDateTime = little_edian_char(buff, offset_modified_time + 4, 4);
	ft.dwLowDateTime = little_edian_char(buff, offset_modified_time, 4);

	FileTimeToSystemTime(&ft, &this->modified);
}

void EntryNTFS::data_attribute(char* buff){
	DWORD start_offset = attributes[7].first;
	DWORD length = attributes[7].second;

	is_nonresident = buff[start_offset + 8];

	if (!is_nonresident){
		int start_data = little_edian_char(buff, start_offset + 20, 4);
		int end_data = buff[start_offset + 4];
		int offset_start_data = start_offset + start_data;
		int offset_end_data = start_offset + end_data - 1; // offset truoc FF
		this->size = offset_end_data - offset_start_data + 1;	// size data in resident

		//data start
		file_content.erase();
		for(int z = offset_start_data; z < offset_end_data; ++z)
			if (buff[z])
				file_content.push_back(buff[z]);
			else
				break;
	}
	else{
		this->size = little_edian_char(buff, start_offset + 48, 8);

		unsigned long long start_runlist = start_offset + little_edian_char(buff, start_offset + 32, 2);
		unsigned long long check_runlist = buff[start_runlist];

		unsigned long long size_fragment;
		signed long long location_fragment = 0; //clusters

		while(check_runlist != 0){
			int s1 = (buff[start_runlist] & 0xF0) >> 4;
			int s2 = (buff[start_runlist] & 0x0F) >> 0;

			size_fragment = little_edian_char(buff, start_runlist + 1, s2) * NTFS::Sc * default_sector_size;

			unsigned long long dec_compare = little_edian_char(buff, start_runlist + 1 + s2, s1);

			if (dec_compare > (1 << s1 * 8 - 1))
				location_fragment = TwoElement(dec_compare);
			else
				location_fragment = dec_compare;

			clusters.push_back(make_pair(location_fragment, size_fragment));

			start_runlist += 1 + s1 + s2;
			check_runlist = buff[start_runlist];
		}

		for (int i = 1; i < clusters.size(); ++i){
			clusters[i].first += clusters[i - 1].first;
		}
	}
}

void EntryNTFS::index_root_attribute(char* buff){
	DWORD start_offset = attributes[8].first;
	DWORD length = attributes[8].second;

	int offset_to_AtrContent = little_edian_char(buff, start_offset + 20, 2) + start_offset;
	int att_type = little_edian_char(buff, offset_to_AtrContent, 4);

	if (buff[offset_to_AtrContent + 28] == 1 && attributes[9].first != 0)
		is_nonresident = true;
	else
		is_nonresident = false;

	if(!is_nonresident && attributes[9].first == 0 && att_type == 48){
		int offset_start_node = offset_to_AtrContent + 16 + little_edian_char(buff, offset_to_AtrContent + 16, 4);
		int length_node = little_edian_char(buff, offset_to_AtrContent + 20, 4) - 16;

		while (length_node > 16){
			int child_mft_id = little_edian_char(buff, offset_start_node, 6);
			int index_length = little_edian_char(buff, offset_start_node + 8, 2);
			if (index_length > 50 && *find(read.begin(), read.end(), child_mft_id) != child_mft_id){
				EntryNTFS* new_file = new EntryNTFS(child_mft_id, this->root + this->name + "/", this);
				if (new_file->read_content())
					entries.push_back(new_file);
				else
					delete new_file;
			}
			length_node -= index_length;
			offset_start_node += index_length;
		}
	}
}

void EntryNTFS::index_location_attribute(char* buff){
	if (is_nonresident){
		DWORD start_offset = attributes[9].first;
		DWORD length = attributes[9].second;

		unsigned long long start_runlist = start_offset + little_edian_char(buff, start_offset + 32, 2);
		unsigned long long check_runlist = buff[start_runlist];

		unsigned long long size_fragment;
		signed long long location_fragment = 0; //clusters

		while(check_runlist != 0){
			int s1 = (buff[start_runlist] & 0xF0) >> 4;
			int s2 = (buff[start_runlist] & 0x0F) >> 0;

			size_fragment = little_edian_char(buff, start_runlist + 1, s2) * NTFS::Sc * default_sector_size;

			unsigned long long dec_compare = little_edian_char(buff, start_runlist + 1 + s2, s1);

			if (dec_compare > (1 << s1 * 8 - 1))
				location_fragment = TwoElement(dec_compare);
			else
				location_fragment = dec_compare;

			clusters.push_back(make_pair(location_fragment, size_fragment));
			start_runlist += 1 + s1 + s2;
			check_runlist = buff[start_runlist];
		}

		for (int i = 1; i < clusters.size(); ++i){
			clusters[i].first += clusters[i - 1].first;	//clusters
		}

		read_index_buffer();
	}
}

void EntryNTFS::attributes_handler(char* buff, int type) {
	if (attributes[type].second == 0)
		return;

	switch(type){
		case 0: //x10
			standard_info_attribute(buff);
			break;
		case 2: //x30
			file_name_attribute(buff);
			break;
		case 7: //x80
			data_attribute(buff);
			break;
	}
}

bool EntryNTFS::read_content() {
	read.push_back(mft_id);

	char* buff = new char[default_sector_size * 2];
	ReadSector(NTFS::disk, buff, mft_id * 2 + NTFS::mft_cluster * NTFS::Sc, 2);

	//check if this is a file record
	string file;
	for (int i = 0; i < 5; ++i)
		file.push_back(buff[i]);

	if (file.compare("FILE0") != 0)
		return false;

	unsigned long long  offset_to_start;
	offset_to_start = little_edian_char(buff, 20, 2);

	attributes.push_back(make_pair(0, 0));
	attributes[0].first = offset_to_start;
	attributes[0].second = little_edian_char(buff, offset_to_start + 4, 4);

	for (int i = 1; i < 10; ++i){
		attributes.push_back(make_pair(0, 0));

		int location = attributes[0].first;
		for (int j = i - 1; j >= 0; --j)
			location +=	attributes[j].second;

		int type = little_edian_char(buff, location, 4);

		if ((i + 1) * 16 == type){
			attributes[i].first = location;
			attributes[i].second = little_edian_char(buff, attributes[i].first + 4, 4);
		}
		else{
			continue;
		}
	}

	//for (auto att : attributes)
	//	cout << att.first << " " << att.second << endl;

	for(int i = 0; i < 10; ++i){
		attributes_handler(buff, i);
	}

	return true;
}

string EntryNTFS::get_content(){
	if (is_nonresident){
		if (this->ext.compare("txt") == 0){
			string str;
			bool stop = false;
			for (auto cluster : clusters){
				if (stop)
					break;
				char* buff = new char[cluster.second];
				ReadSector(NTFS::disk, buff, cluster.first * NTFS::Sc, cluster.second / default_sector_size);
				for (int i = 0; i < cluster.second && !stop; ++i)
					if (buff[i])
						str.push_back(buff[i]);
					else
						stop = true;

				delete[] buff;
			}
			return str;
		}
		else
			return "-- SORRY, CURRENTLY WE CAN ONLY OPEN TEXT (TXT) FILES --";
	}
	else {
		return file_content;
	}
}

bool EntryNTFS::type(){
	return (attributes[8].first > 0 && attributes[8].second > 0);
}

string EntryNTFS::get_modified_date(){
	stringstream ss;
	ss << this->modified.wDay << "/" << this->modified.wMonth << "/" << this->modified.wYear << " " << this->modified.wHour << ":";
	if (this->modified.wMinute < 10)
		ss << "0" << this->modified.wMinute;
	else
		ss << this->modified.wMinute;

	return ss.str();
}

string EntryNTFS::get_size(){
	if (this->type())
		return "";
	else if (this->size > 0){
		vector<string> unit = {" Bytes", " KB", " MB", " GB"};
		float temp = this->size;
		stringstream ss;

		for (int i = 0; i < 4; ++i){
			if (to_string(int(temp)).length() < 5){
				ss << round(temp) << unit[i];
				return ss.str();
			}
			temp /= 1024.0;
		}

		ss << round(temp) << " TB";
		return ss.str();
	}
	else {
		return "0 Bytes";
	}
}

vector<string> EntryNTFS::get_info(){
	vector<string> info;

	info.push_back(this->name);

	info.push_back(get_modified_date());
	if (type())
		info.push_back("File folder");
	else if (this->name[0] == '$')
		info.push_back("Attribute");
	else
		info.push_back(this->ext);

	info.push_back(get_size());

	return info;
}

vector<EntryNTFS*> EntryNTFS::get_directory(){
	if (type() && entries.size() == 0){
		char* buff = new char[2 * default_sector_size];
		ReadSector(NTFS::disk, buff, mft_id * 2 + NTFS::Sc * NTFS::mft_cluster, 2);
		index_root_attribute(buff);
		index_location_attribute(buff);
		delete buff;
	}
	return entries;
}

string EntryNTFS::get_path(){
	if (type())
		return this->root + this->name;
	else
		return this->root + this->name + "." + this->ext;
}

EntryNTFS* EntryNTFS::get_entry(int id){
	if (id < this->entries.size())
		return this->entries[id];
	return NULL;
}

NTFS::NTFS(const char* disk) { this->disk = disk; }

NTFS::~NTFS(){
	delete root;
	current_directory = NULL;
	read.clear();
}

bool NTFS::read_pbs(char* buff) {
	if((unsigned char)buff[510] == 0x55 && (unsigned char)buff[511] == 0xaa){
		default_sector_size = little_edian_char(buff, 11, 2);
		Sc = buff[13];
		mft_cluster = little_edian_char(buff, 48, 8);
		size_mft_entry = little_edian_char(buff, 64, 1);
		Sv = little_edian_char(buff, 40, 8);
		return true;
	}
	else
		return false;
}

void NTFS::read_mft(){
	root = new EntryNTFS(5, "", NULL);
	root->isRoot = true;
	root->read_content();
	current_directory = root;
}

string NTFS::get_pbs_info(){
	stringstream ss;

	ss << "Sector size: " << default_sector_size << " Bytes\n";
	ss << "Volume size: " << Sv / 2097152.0 << " GB\n";
	ss << "Sectors per cluster: " << Sc << endl;
	ss << "MFT first cluster: " << mft_cluster << endl;
	return ss.str();
}

EntryNTFS* NTFS::change_directory(int id){
	this->current_directory = this->current_directory->get_entry(id);
	return this->current_directory;
}

EntryNTFS* NTFS::back_parent_directory(){
	if (this->current_directory != this->root)
		this->current_directory = this->current_directory->parent;
	return this->current_directory;
}

EntryNTFS* NTFS::get_current_directory(){
	return this->current_directory;
}

string NTFS::get_cd_path(){
	return this->current_directory->get_path();
}
