#include <windows.h>
#include <stdio.h>
#include <iostream>
using namespace std;

bool ReadSect(const char *_dsk, char*& _buff, unsigned int _nsect) {
	DWORD dwRead;   
	HANDLE hDisk = CreateFile(_dsk,GENERIC_READ,FILE_SHARE_VALID_FLAGS,0,OPEN_EXISTING,0,0);
	if(hDisk == INVALID_HANDLE_VALUE){
		CloseHandle(hDisk);
		return false;
	}

	SetFilePointer(hDisk, _nsect * 512, 0, FILE_BEGIN); // which sector to read
	ReadFile(hDisk, _buff, 512, &dwRead, 0);
	CloseHandle(hDisk);
	return true;
}

int main(){
	char* dsk = "\\\\.\\PhysicalDrive1";
	int sector = 0;
	char const hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	char* buff = new char[512];

	if(ReadSect(dsk, buff, sector)){
		if((unsigned char)(buff[510] == 0x55 && (unsigned char)buff[511] == 0xaa))
			cout << "Disk is bootable!" << endl;

		for (int i = 0; i < 512; ++i){
			if (i % 16 == 0)
				cout << endl;
			cout << hex_chars[ (buff[i] & 0xF0) >> 4 ];
			cout << hex_chars[ (buff[i] & 0x0F) >> 0 ] << " ";
		}

		for (int i = 0; i < 512; ++i){
			if (i % 16 == 0)
				cout << endl;

			if (isascii(buff[i]))
				cout << buff[i] << " ";
			else 
				cout << ". ";
		}
	}
	else {
		cout << "Run as admin";
	}
} 
