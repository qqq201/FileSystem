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

class FAT32 {
	private:
		const char* disk = NULL;
		long Sb = 0;
		int Nf = 0;
		int Sf = 0;
		long first_cluster = 0;
		long Sv = 0;
		int Sc = 0;

	public:
		FAT32(const char* disk) {
			this->disk = disk;
		}

		void read_bootsector(char* data){
			default_sector_size = little_edian_read(data, 11, 2);
			this->Sb = little_edian_read(data, 14, 2); 
			this->Nf = data[16];
			this->first_cluster = little_edian_read(data, 44, 4);
			this->Sf = little_edian_read(data, 36, 4);
			this->first_cluster = little_edian_read(data, 44, 4);
			this->Sv = little_edian_read(data, 32, 4);
			this->Sc = data[13];
		}

		void get_bs_info(){
			cout << "Sector size: " << default_sector_size << " Bytes\n";
			cout << "Volume size: " << this->Sv / 2097152.0 << " GB\n";
			cout << "Boot sector size: " << this->Sb << " Sectors\n"; 
			cout << "Number of fat: " << this->Nf << endl;
			cout << "Fat size: " << this->Sf << " Sectors\n";
			cout << "Sectors per cluster: " << this->Sc << endl;
			cout << "RDET first cluster: " << this->first_cluster << endl;
		}

		void read_fat(int entry){
			
		}

		void read_rdet() {

		}

		void read_file(vector<int> clusters) {
			for (int cluster : clusters){
				int sector = Sb + Nf * Sf + (cluster - first_cluster) * Sc;
				for (int s = sector; s < sector + Sc; ++s){
					char* buff = new char[512];
					if (ReadSector(disk, buff, s)){
						for (int c = 0; c < 512; ++c)
							if (buff[c] != 0)
								cout << buff[c];
					}
				}
			}
		}
};

int main(){
	//set tile
	SetConsoleTitleA("File system reader");
	//set white console
	system("color f0");

	//list drives

	//select drive
	cout << "Which drive: ";
	char c = cin.get();
	cin.ignore();

	//get drive ID
	string disk = "\\\\.\\?:";
	disk[4] = toupper(c);

	//read first sector and determine which filesystem
	char* buff = new char[512];
	if (ReadSector(disk.c_str(), buff, 0)){
		string Filesystem;
		
		for (int i = 82; i < 90; ++i){
			if (buff[i] == ' ')
				break;
			Filesystem += buff[i];
		}

		cout << "File system: "<< Filesystem << endl;

		if (Filesystem.compare("FAT32") == 0){
			FAT32 fat(disk.c_str());
			fat.read_bootsector(buff);
			fat.get_bs_info();
		}
	}
	else {
		cout << "Please run as administrator\n";
	}

	cout << "\nPress any key to exit...";
	cin.get();
} 
