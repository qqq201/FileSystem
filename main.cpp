#include "fat.h"

int main(){
	//set title
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
			cout << "├━ " << disk << ":\\\n│\n";
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
				//string Filesystem;
				//for (int i = 82; i < 90; ++i){
				//	if (buff[i] == ' ')
						//break;
					//Filesystem += buff[i];
				//}

				//cout << "File system: "<< Filesystem << endl;

				//if (Filesystem.compare("FAT32") == 0){
				FAT32 fat(disk.c_str());
				if(fat.read_bootsector(buff)){
					fat.get_bs_info();
					fat.read_rdet();
					fat.get_rdet_info();
					fat.print_tree();
					char* buf = new char[512];
					ReadSector(disk.c_str(), buf, 0);
					print_sector(buf);
				}
				//}
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
