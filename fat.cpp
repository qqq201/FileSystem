#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
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

class FAT32 {
	private:
		const char* disk = NULL;
		long Sb = 0;
		int Nf = 0;
		int Sf = 0;
		long Srdet = 0;
		long first_cluster = 0;
		long Sv = 0;
		int Sc = 0;

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

		void read_fat(int entry){
			
		}

		void read_rdet() {
		}

		void read_file(int cluster) {
			
		}
};

int main(){
	cout << "Which drive: ";
	char c = cin.get();
	cin.ignore();

	string disk = "\\\\.\\?:";
	disk[4] = toupper(c);

	FAT32 fat(disk.c_str());
	fat.read_bootsector(true);
	fat.get_bs_info();

	cout << "\nPress any key to continue...";
	cin.get();
} 
