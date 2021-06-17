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

class NTFS {
	private:
		const char* disk = NULL;
	public:
		NTFS(const char* disk) {
			this->disk = disk;
		}
};

int main(){
	cout << "Which drive: ";
	char c = cin.get();
	cin.ignore();

	string disk = "\\\\.\\?:";
	disk[4] = toupper(c);

	NTFS ntfs(disk.c_str());

	cout << "\nPress any key to continue...";
	cin.get();
}
