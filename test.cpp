#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
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

int ReadSector(int numSector,char* buf)
{
    int retCode = 0;
    BYTE sector[512];
    DWORD bytesRead;
    HANDLE device = NULL;

    device = CreateFile("\\\\.\\E:", GENERIC_READ, FILE_SHARE_READ, NULL,OPEN_EXISTING, 0, NULL);

    if(device != NULL){
        // Read one sector
        SetFilePointer (device, numSector*512, NULL, FILE_BEGIN) ;
        if (!ReadFile(device, sector, 512, &bytesRead, NULL)) {
            cout << "Error in reading1 floppy disk\n";
        }
        else {
            // Copy boot sector into buffer and set retCode
            memcpy(buf,sector, 512);
			retCode=1;
        }
        CloseHandle(device);
    }

    return retCode;
}

char const hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
void print_sector(char*& buff){
	cout << "\nHex:";
	for (int i = 0; i < 512; ++i){
		if (i % 16 == 0)
			cout << endl;
		cout << hex_chars[ (buff[i] & 0xF0) >> 4 ];
		cout << hex_chars[ (buff[i] & 0x0F) >> 0 ] << " ";
	}

	cout << "\nascii:";
	for (int i = 0; i < 512; ++i){
		if (i % 16 == 0)
			cout << endl;

		if (isascii(buff[i]))
			cout << buff[i] << " ";
		else 
			cout << ". ";
	}
}

int main(){
	char* dsk = "\\\\.\\PhysicalDrive1";
	int sector = 0;
	char* buff = new char[512];

	if(ReadSector(sector, buff)){
		print_sector(buff);
	}
} 
