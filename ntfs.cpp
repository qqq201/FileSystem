#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <math.h>
#include <algorithm>
using namespace std;

signed long long TwoElement(unsigned long long num);
string hex_to_val(string str);

char const hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
WORD default_sector_size = 512;

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

bool ReadSector(const char *disk, char*& buff, unsigned long long sector) {
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

void ReadSector_mft(DWORD low, LONG high, unsigned long long bytepercluster, const char *disk, char*& buff, unsigned long long sector){
	DWORD dwRead;
	HANDLE hDisk = CreateFile(disk,GENERIC_READ,FILE_SHARE_VALID_FLAGS,0,OPEN_EXISTING,0,0);
	if(hDisk == INVALID_HANDLE_VALUE){
		CloseHandle(hDisk);
	}

	SetFilePointer(hDisk, low + default_sector_size * sector,(PLONG) &high, FILE_BEGIN);
	ReadFile(hDisk, buff, bytepercluster, &dwRead, NULL);
	CloseHandle(hDisk);
}

void print_sector(char*& buff){
	for (int i = 0; i < default_sector_size; ++i){
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

class EntryNTFS {
	protected:
		string name;
		vector< pair<unsigned long long, unsigned long long> > clusters;
	public:
		EntryNTFS() {}
		void print_info() {}
};

class FileNTFS : protected EntryNTFS{
	private:
	public:
		FileNTFS() : EntryNTFS() {}
		void print_info() {}
};

class FolderNTFS : protected EntryNTFS{
	private:
		vector<EntryNTFS*> entries;
	public:
		FolderNTFS() : EntryNTFS() {}
		void print_info() {
			for (EntryNTFS* entry : this->entries){
				entry->print_info();
			}
		}
};

class NTFS {
	private:
		const char* disk = NULL;
		unsigned long long first_cluster = 0;
		int Sc = 0;	// sector per cluster - unit: sector
		unsigned long long Sv = 0;			// size of volume - sector
		unsigned long long mft_cluster = 0; // cluster bat dau
		int size_mft_entry = 0;	//size of MFT entry - byte

		FolderNTFS* root;

	public:
		NTFS(const char* disk) {
			this->disk = disk;
		}

		bool read_pbs(char* buff) {
			if((unsigned char)buff[510] == 0x55 && (unsigned char)buff[511] == 0xaa){
				print_sector(buff);
				default_sector_size = little_edian_char(buff, 11, 2);
				Sc = buff[13];
				mft_cluster = little_edian_char(buff, 48, 8);
				unsigned long long bytepercluster = Sc * default_sector_size;
				MFTLocation.QuadPart = mft_cluster * bytepercluster;
				int i = 10;
				while (i != 18){
					char* buff = new char[bytepercluster];
					ReadSector_mft(MFTLocation.LowPart, MFTLocation.HighPart, bytepercluster, disk, buff, 2 * i);
					print_sector(buff);
					cout << endl;
					++i;
				}
				cout << MFTLocation.QuadPart << endl;
				size_mft_entry = little_edian_char(buff, 64, 1);
				Sv = little_edian_char(buff, 40, 8);
				return true;
			}else{
				return false;
			}
		}

		unsigned long long find_sector_attribute(string data)
		{
			unsigned long long i = 2;
			char* buff = new char[512];
			while (true)
			{
				bool check = 0;
				ReadSector(disk, buff, 2*Sc + i);

				for (int j=0; j<512; j++){
					stringstream ss;

					ss << ((buff[j] & 0xF0) >> 4);
					ss << ((buff[j] & 0x0F) >> 0);

					string str_ss = hex_to_val(ss.str());

					if(str_ss[0] == data[0]){
						for(int z=1; z<data.length(); z++){
							stringstream sss;

							sss << ((buff[j+z*2] & 0xF0) >> 4);
							sss << ((buff[j+z*2] & 0x0F) >> 0);

							string str_sss = hex_to_val(ss.str());

							if(str_sss[0] != data[z]){
								check = 0;
								break;
							}else{
								check = 1;
							}
						}
						if(check == 1) return 2*Sc + i;
					}
				}
				i += 1;

				if(2*Sc + i > (Sv / 512)) return 0;
			}

		}

		void read_mft() {
			char* buff = new char[512];

			if (find_sector_attribute("folder1") != 0){
				ReadSector(disk, buff,find_sector_attribute("folder1"));
				print_sector(buff);
			}

			unsigned long long  offset_to_start;
			offset_to_start = little_edian_char(buff, 20, 2);
			int size_10 = buff[offset_to_start + 4];

			int offset_30 = offset_to_start + size_10;
			int size_30 = buff[offset_30 + 4];

			int offset_80 = offset_to_start + size_10 + size_30;
			int size_80 = buff[offset_80 + 4];

			unsigned long long start_runlist = offset_80 + little_edian_char(buff, offset_80 + 32, 1);
			unsigned long long check_runlist = buff[start_runlist];

			vector< pair<unsigned long long, unsigned long long> > clusters;	// cluster bat dau - size
			unsigned long long size_fragment;
			signed long long location_fragment = 0; //clusters

			char* p;
			while(check_runlist != 0){
				stringstream ss1, ss2;
				ss1 << ((buff[start_runlist] & 0xF0) >> 4);
				ss2 << ((buff[start_runlist] & 0x0F) >> 0);

				//cout << "1: " << ((buff[start_runlist] & 0xF0) >> 4) << endl;
				//cout << "2: " << ((buff[start_runlist] & 0x0F) >> 0) << endl;
				//cout << check_runlist << endl;

				unsigned long long s1 = strtol(ss1.str().c_str(), &p, 16);
				unsigned long long s2 = strtol(ss2.str().c_str(), &p, 16);

				size_fragment = little_edian_char(buff, start_runlist + 1, s2);
				size_fragment *= 4096; // clusters to bytes
				//cout << "s: " << size_fragment << endl;
				//location_fragment = location_fragment + little_edian_char(buff, start_runlist + 1 + s2, s1);
				//location âm => chuyển về dec > 2^(n-1) => số âm => bù 2 để chuyển về số âm
				unsigned long long dec_compare = little_edian_char(buff, start_runlist + 1 + s2, s1);
				if ( dec_compare > pow(2,s1*8-1)){
					location_fragment = TwoElement(dec_compare);

				}else{
					location_fragment = dec_compare;
				}
				//cout << "l: " << location_fragment << endl;

				clusters.push_back(make_pair(location_fragment, size_fragment));

				start_runlist += 1 + s1 + s2;
				check_runlist = buff[start_runlist];
				cout << check_runlist << endl;
			}

			for (int i=1; i<clusters.size(); i++){
				clusters[i].first += clusters[i-1].first;	//clusters
			}

		}
};

signed long long TwoElement(unsigned long long num){

    // Find binary vector for number
    vector<int> bit_vector;

    while (num >  0)
    {
        bit_vector.push_back(num%2);
        num = num/2;
    }reverse(bit_vector.begin(), bit_vector.end());

    // substract -1 from binary
    bool borrow_bit = 0;
    if (bit_vector.back() == 0){
        borrow_bit =1;
        bit_vector.back() = 1;
    }else{
        bit_vector.back() = 0;
    }

    int i = bit_vector.size() - 2;

    while (borrow_bit != 0)
    {
        if(bit_vector[i] == 0){
            borrow_bit =1;
            bit_vector[i] = 1;
            i = i-1;
        }else{
            borrow_bit = 0;
            bit_vector[i] = 0;
        }
    }

    cout << endl;
    for(int j : bit_vector){
        cout << j;
    }
    cout << endl;

    // XOR
    vector<int> vt;
    for(int j = 0; j<bit_vector.size(); j++){
        if(bit_vector[j] == 1) vt.push_back(0);
        else vt.push_back(1);
    }

    //cout << "size: " << vt.size() << endl;
    for(int k : vt){
        cout << k;
    }
    cout << endl;

    signed long long result = 0;
    int m = 0;
    for(int i=vt.size()-1; i>=0; i--){
        result += vt[i] * pow(2, m);
        m++;
    }

    return -1*result;

}
string hex_to_val(string str)
{
    string res;
    res.reserve(str.size() / 2);
    for (int i = 0; i < str.size(); i += 2)
    {
        std::istringstream iss(str.substr(i, 2));
        int temp;
        iss >> std::hex >> temp;
        res += static_cast<char>(temp);
    }
    return res;
}

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
				ntfs.read_mft();
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
