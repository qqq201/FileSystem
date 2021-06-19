#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;

char const hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
int default_sector_size = 512;

bool ReadSector(const char *disk, char*& buff, unsigned int sector) {
	DWORD dwRead;
	HANDLE hDisk = CreateFile(disk,GENERIC_READ,FILE_SHARE_VALID_FLAGS,0,OPEN_EXISTING,0,0);
	if(hDisk == INVALID_HANDLE_VALUE){
		CloseHandle(hDisk);
		return false;
	}

	SetFilePointer(hDisk, sector * default_sector_size, 0, FILE_BEGIN); // which sector to read
	ReadFile(hDisk, buff, default_sector_size, &dwRead, 0);
	CloseHandle(hDisk);
	return true;
}

void print_sector(char*& buff){
	for (int i = 0; i < default_sector_size; ++i){
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

long little_edian_read(char* data, int start, int nbyte){
	std::stringstream ss;
	for (int i = start + nbyte - 1; i >= start; --i){
		ss << hex_chars[ (data[i] & 0xF0) >> 4 ];
		ss << hex_chars[ (data[i] & 0x0F) >> 0 ];
	}

	return stoi(ss.str(), 0, 16);
}

void readCharacter (string& data, char* buff, int start, int nbyte, int nextra) {
	int pos = start;
	char character;
	for(int i = 0; i < nbyte; i++) {
		character = buff[pos + 32 * nextra];
		data += character;
		pos += 1;
	}
}

class Entry {
	protected:
		char* dataEntry;
		int numExtraEntry = 0;
		long firstCluster = 0;
		string status = "";
		string name = "";
	public:
		Entry(char* dataEntry, int numExtraEntry) {
			this->dataEntry = dataEntry;
			this->numExtraEntry = numExtraEntry;
		}

		void readStatus() {
			vector <int> binary0B(8, 0);
			int offset0B = dataEntry[11 + numExtraEntry * 32];
			for(int i = 0; offset0B > 0; i++) {
				binary0B[i] = offset0B % 2;
				offset0B = offset0B / 2;
			}
			string status[6] = { "Read Only", "Hidden", "System", "Vol Label", "Directory", "Archive" };
			for(int i = 0; i < 6; i++) {
				if(binary0B[i] == 1) {
					this->status += status[i];
					this->status += " | ";
				}
			}
		}

		void readFirstCluster() {
			//* 2 high byte at 14
			long high = little_edian_read(this->dataEntry, 20 + this->numExtraEntry * 32, 2);
			//* 2 high low at 1A
			long low = little_edian_read(this->dataEntry, 26 + this->numExtraEntry * 32, 2);
			this->firstCluster = high * 65536 + low;
			//256: AABB / 65536: AAAABBBB
		}

		void readName() {
			if(this->numExtraEntry == 0) {
				//* 8 byte at 00
				readCharacter(this->name, this->dataEntry, 0, 8, this->numExtraEntry);
			}
			else {
				int pos;
				int temp = 0;
				for(int extraEntry = this->numExtraEntry - 1; extraEntry >= 0 ; extraEntry--) {
					//* 10 byte at 01
					readCharacter(this->name, this->dataEntry, 1, 10, extraEntry);
					//* 12 byte at 0E
					readCharacter(this->name, this->dataEntry, 14, 12, extraEntry);
					//* 4 byte at 1C
					readCharacter(this->name, this->dataEntry, 28, 4, extraEntry);
				}
			}
		}

		virtual void readEntry() = 0;

		virtual void print_info() {
			cout << "Status: " << this->status << endl;
			cout << "First cluster: " << this->firstCluster << endl;
		}
};

class File : public Entry{
	private:
		string ext = "";
		int fileSize = 0;
	public:
		File(char* dataEntry, int numExtraEntry) : Entry(dataEntry, numExtraEntry) {}

		void readExt() {
			//* 3 byte at 08
			readCharacter(this->ext, this->dataEntry, 8, 3, this->numExtraEntry);
		}

		void readSize() {
			//* 4 byte at 1C
			this->fileSize = little_edian_read(this->dataEntry, 28, 4);
		}

		virtual void readEntry() {
			this->readName();
			this->readExt();
			this->readStatus();
			this->readSize();
			this->readFirstCluster();
		}

		virtual void print_info() {
			cout << "File name: " << this->name << this->ext << endl;
			cout << "Size of file : " << this->fileSize << " byte" << endl;
			Entry::print_info();
		}
};

class Folder : public Entry{
	private:
		vector<Entry*> entries;
	public:
		Folder(char* dataEntry, int numExtraEntry) : Entry(dataEntry, numExtraEntry) {}

		virtual void readEntry() {
			this->readName();
			this->readStatus();
			this->readFirstCluster();
		}

		virtual void print_info() {
			cout << "Folder name: " << this->name << endl;
			Entry::print_info();
			// for (Entry* entry : this->entries){
				// entry->print_info();
			// }
		}
};

// Todo: return Folder or File instance
Entry* getEntry(char* data, int extraEntries) {
	Entry* entry;
	long offset0B = data[11 + extraEntries * 32];

	vector <int> binary0B(8, 0);
	for(int i = 0; offset0B > 0; i++) {
		binary0B[i] = (offset0B % 2);
		offset0B = offset0B / 2;
	}

	if(binary0B[5] == 1) {
		entry = new File(data, extraEntries);
	}
	else if(binary0B[4] == 1) {
		entry = new Folder(data, extraEntries);
	}
	return entry;
}

class FAT32 {
	private:
		const char* disk = NULL;
		long first_cluster = 0;
		long Sb = 0;
		int Nf = 0;
		int Sf = 0;
		long Srdet = 0;
		long Sv = 0;
		int Sc = 0;
		vector <Entry*> entries;

	public:
		FAT32(const char* disk) {
			this->disk = disk;
		}

		void read_sector_size(char* data){
			default_sector_size = little_edian_read(data, 11, 2);
		}

		void read_Sb(char* data){
			this->Sb = little_edian_read(data, 14, 2);
		}

		void read_Nf(char* data){
			this->Nf = data[16];
		}

		void read_Sf(char* data){
			this->Sf = little_edian_read(data, 36, 4);
		}

		void read_Srdet(char* data){
			this->Srdet = little_edian_read(data, 17, 2);
			this->first_cluster = little_edian_read(data, 44, 4);
		}

		void read_Sv(char* data){
			this->Sv = little_edian_read(data, 32, 4);
		}

		void read_Sc(char* data){
			this->Sc = data[13];
		}

		void read_bootsector(bool print_raw){
			int sector = 0;
			char* buff = new char[512];

			if (ReadSector(this->disk, buff, sector)){
				read_sector_size(buff);
				read_Sb(buff);
				read_Nf(buff);
				read_Srdet(buff);
				read_Sv(buff);
				read_Sc(buff);
				read_Sf(buff);

				if (print_raw){
					print_sector(buff);
				}
			}
			else {
				cout << "Please run as administrator.";
			}
		}

		void get_bs_info(){
			cout << "Sector size: " << default_sector_size << " Bytes\n";
			cout << "Volume size: " << Sv / 2097152.0 << " GB\n";
			cout << "Boot sector size: " << Sb << " Sectors\n";
			cout << "Number of fat: " << Nf << endl;
			cout << "Fat size: " << Sf << " Sectors\n";
			cout << "RDET size: " << Srdet << " Sectors\n";
			cout << "Sectors per cluster: " << Sc << endl;
			cout << "RDET first cluster: " << first_cluster << endl;
		}

		void read_fat(int entry){}

		void read_rdet(bool print_raw) {
			int sector = this->Sb + this->Sf * this->Nf;
			bool hasNextSector = true;
			char* buff = new char[512];

			while(hasNextSector) {
				if(ReadSector(this->disk, buff, sector)) {
					int extraEntries = 0;
					char* entryData = new char[512];
					for(int byte = 0; byte < 512; byte+=32) {
						for(int i = 0 ; i < 32 ; i++) {
							if(extraEntries == 0) {
								entryData[i] = buff[byte + i];
							}
							else {
								entryData[i + 32 * extraEntries] = buff[byte + i];
							}
						}
						//* Check extra entry
						// long offset0B = little_edian_read(entryData, 11 + extraEntries * 32, 1); //decimal
						long offset0B = entryData[11 + extraEntries * 32];
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
							entries.push_back(getEntry(entryData, extraEntries));
							entryData = new char;
							extraEntries = 0;
						}
					}
				}
				if(print_raw) {
					print_sector(buff);
				}
			}
		}

		void get_rdet_info() {
			for(Entry* entry: this->entries) {
				entry->readEntry();
				entry->print_info();
			}
			cout << "Number of entry in DRET: " << entries.size();
		}

		void read_file(int cluster) {}
};

int main(){
	cout << "Which drive: ";
	char c = cin.get();
	cin.ignore();

	string disk = "\\\\.\\?:";
	disk[4] = toupper(c);

	FAT32 fat(disk.c_str());
	fat.read_bootsector(false);
	fat.get_bs_info();
	fat.read_rdet(true);

	cout << "\nPress any key to get info...";
	cin.get();

	fat.get_rdet_info();

	cout << "\nPress any key to continue...";
	cin.get();
}
