#include "ntfs.h"

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
			if (ReadSector(disk.c_str(), buff, 0, 1)){
				NTFS ntfs(disk.c_str());
				ntfs.read_pbs(buff);
				ntfs.read_mft();
				//ntfs.read_mft_folder();
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
