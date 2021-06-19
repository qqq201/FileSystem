#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <vector>
using namespace std;

char const hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
int default_sector_size = 512;

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

	SetFilePointer(hDisk, sector * default_sector_size, 0, FILE_BEGIN); // which sector to read
	ReadFile(hDisk, buff, default_sector_size, &dwRead, 0);
	CloseHandle(hDisk);
	return true;
}

void print_sector(char*& buff){
	for (int i = 0; i < default_sector_size; ++i){
		if (i % 16 == 0){
			cout << setfill('0') << setw(7) << hex << i / 16 << "0: ";
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

long little_edian_read(char* data, int start, int nbyte){
	std::stringstream ss;
	for (int i = start + nbyte - 1; i >= start; --i){
		ss << hex_chars[ (data[i] & 0xF0) >> 4 ];
		ss << hex_chars[ (data[i] & 0x0F) >> 0 ];
	}

	return stoi(ss.str(), 0, 16);
}

class Entry {
	protected:
		string name;
	public:
		Entry() {}
		void print_info() {}
};

class File : protected Entry{
	private:
	public:
		File() : Entry() {}
		void print_info() {}
};

class Folder : protected Entry{
	private:
		vector<Entry*> entries;
	public:
		Folder() : Entry() {}
		void print_info() {
			for (Entry* entry : this->entries){
				entry->print_info();
			}
		}
};

class NTFS {
	private:
		const char* disk = NULL;
		uint16_t first_cluster = 0;
		uint8_t Sc = 0;
		uint16_t Sb = 0;
		uint16_t St = 0;
		uint16_t h = 0;
		long long Sv = 0;
		long long mft_cluster = 0;
		vector<Entry*> root;

	public:
		NTFS(const char* disk) {
			this->disk = disk;
		}

		void read_pbs(char* buff) {
			
		}

		void read_mft() {
			
		}
};

int main(){
	//set tile
	SetConsoleTitleA("Filesystem reader");
	//set white console
	system("color f0");

	//set console can print unicode char
	SetConsoleOutputCP(65001);

	string disks;
	if (get_logical_disks(disks)){
		//list drives
		cout << "Your current disks:\n";
		for (char disk : disks){
			cout << "└- " << disk << ":\\\n│\n";
		}

		//select drive
		cout << "Which disk to inspect: ";
		char c = toupper(cin.get());
		cin.ignore();

		size_t found = disks.find(c);
	    if (found != string::npos){
			//get drive ID
			string disk = "\\\\.\\?:";
			disk[4] = c;

			//read first sector and determine which filesystem
			char* buff = new char[512];
			if (ReadSector(disk.c_str(), buff, 0)){
				NTFS ntfs(disk.c_str());
				ntfs.read_pbs(buff);
			}
			else {
				cout << "Error while reading\n";
			}
		}
		else
			cout << "Your input disk does not exist!\n";
	}
	else {
		cout << "Can not get your logical disk name\n";
	}

	cout << "\nPress any key to exit...";
	cin.get();
}
