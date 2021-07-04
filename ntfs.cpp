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

bool ReadSector(const char *disk, char* buff, unsigned long long sector, unsigned long long nsector) {
	DWORD dwRead;
	HANDLE hDisk = CreateFile(disk,GENERIC_READ,FILE_SHARE_VALID_FLAGS,0,OPEN_EXISTING,0,0);
	if(hDisk == INVALID_HANDLE_VALUE){
		CloseHandle(hDisk);
		return false;
	}

	ULARGE_INTEGER location;
	location.QuadPart = sector * default_sector_size;
	DWORD low = location.LowPart;
	LONG high = location.HighPart;

	SetFilePointer(hDisk, low, (PLONG) &high, FILE_BEGIN); // which sector to read
	ReadFile(hDisk, buff, default_sector_size * nsector, &dwRead, 0);
	CloseHandle(hDisk);
	return true;
}

void print_sector(char* buff, int n){
	for (int i = 0; i < default_sector_size * n; ++i){
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

		struct Date {
			int year;
			int month;
			int day;
			int hour;
			int minute;
			int second;
		};

	public:
		NTFS(const char* disk) {
			this->disk = disk;
		}

		bool read_pbs(char* buff) {
			if((unsigned char)buff[510] == 0x55 && (unsigned char)buff[511] == 0xaa){
				print_sector(buff, 1);
				default_sector_size = little_edian_char(buff, 11, 2);
				Sc = buff[13];
				mft_cluster = little_edian_char(buff, 48, 8);

				//char* buff = new char[512];
				//ReadSector(disk, buff, mft_cluster * Sc + 2);
				//print_sector(buff);
				size_mft_entry = little_edian_char(buff, 64, 1);
				Sv = little_edian_char(buff, 40, 8);
				return true;
			}else{
				return false;
			}
		}

		unsigned long long find_sector_attribute(string data)
		{
			unsigned long long i = 1;

			while (true)
			{
				bool check = 0;
				char* buff = new char[512];
				ReadSector(disk, buff, mft_cluster*Sc + i*2, 1);

				for (int j=0; j<512; j++){
					if(buff[j] == data[0]){
						for(int z=0; z < data.length(); z++){
							if(buff[j+z*2] != data[z]){
								check = 0;
								break;
							}else{
								check = 1;
							}
						}
						if(check == 1) return mft_cluster*Sc + i*2;
					}
				}
				i += 1;

				if(2*Sc + 2*i > (Sv / 512)) return 0;
			}

		}

		void read_index_buffer(vector<pair<unsigned long long, unsigned long long>> clusters){
			cout << "File in this folder:\n";

			for (auto cluster : clusters){
				//index header 24 bytes
				int ih_start_offset = 0;
				int ih_end_offfset = 23;
				int nh_start_offset = 24;
				int nh_end_offset = 39;

				// check if this is an index buffer
				char* buff = new char[cluster.second];
				ReadSector(disk, buff, cluster.first * Sc, cluster.second / default_sector_size);

				string indx;
				for (int i = ih_start_offset; i < 4; ++i)
					indx.push_back(buff[i]);

				if (indx != "INDX")
					continue;

				DWORD eh_start_offset = little_edian_char(buff, nh_start_offset, 4) + nh_start_offset;
				DWORD index_node_length = little_edian_char(buff, nh_start_offset + 4, 4) - 16 - eh_start_offset + nh_start_offset;
				int offset_attribute_content = 16;

				while (index_node_length > 0){
					unsigned long long mft_id = little_edian_char(buff, eh_start_offset, 6);
					WORD index_length = little_edian_char(buff, eh_start_offset + 8, 2);
					char* test = new char[512];
					ReadSector(disk, test, mft_id * 2 + Sc * mft_cluster, 1);
					print_sector(test, 1);
					eh_start_offset += index_length;
					index_node_length -= index_length;
				}

				delete[] buff;
			}
		}

		void file_name_attribute(char* buff, DWORD start_offset, DWORD length) {

		}

		void standard_info_attribute(char* buff, DWORD start_offset, DWORD length){

		}

		void data_attribute(char* buff, DWORD start_offset, DWORD length){

		}

		void index_root_attribute(char* buff, DWORD start_offset, DWORD length){

		}

		void index_location_attribute(char* buff, DWORD start_offset, DWORD length){

		}


		void attributes_handler(char* buff, int type, DWORD start_offset, DWORD length) {
			if (buff[start_offset] != (type + 1) * 16)
				return;

			switch(type){
				case 0: //x10
					standard_info_attribute(buff, start_offset, length);
					break;
				case 2: //x30
					file_name_attribute(buff, start_offset, length);
					break;
				case 7: //x80
					data_attribute(buff, start_offset, length);
					break;
				case 8: //x90
					index_root_attribute(buff, start_offset, length);
					break;
				case 9: //xA0
					index_location_attribute(buff, start_offset, length);
					break;
			}
		}
		void read_mft_file() {
			char* buff = new char[1024];

			unsigned long long ll = find_sector_attribute("test1.txt");
			if ( ll != 0){
				ReadSector(disk, buff, ll, 1);
				print_sector(buff, 1);
				cout << endl << ll << endl;
			}else return;

			// long long ll=49*2;
			// while (ll!=55*2)
			// {
			// 	ReadSector(disk, buff,mft_cluster*Sc + ll);
			// 	print_sector(buff);
			// 	ll++;
			// }

			//ReadSector(disk, buff,4494*Sc);
			//print_sector(buff);

			unsigned long long  offset_to_start;
			offset_to_start = little_edian_char(buff, 20, 2);

			vector<pair<int, int>> attribute(10, make_pair(0,0));	//  8 attribute dau tien (vitri - size)
			attribute[0].first = offset_to_start;
			attribute[0].second = little_edian_char(buff, offset_to_start + 4, 4);
			for (int i=1; i<10; i++){
				int temp = attribute[0].first;
				for (int j = i-1; j >=0; j--)
				{
					temp +=	attribute[j].second;
				}

				int location = little_edian_char(buff, temp, 4);

				if ((i+1)*16 == location){
					attribute[i].first = temp;
					attribute[i].second = buff[attribute[i].first + 4];
				}else{
					continue;
				}
			}

			for (int i=0; i<10; i++){
				cout << i << ": " << attribute[i].first << " - " << attribute[i].second << endl;
			}

			//switch case

			//offset 10 - last time modified
			int offset_creation_time = little_edian_char(buff, attribute[0].first + 20, 2) + attribute[0].first;
			int offset_modified_time = offset_creation_time + 8;



			//Offset 30 - file name
			int offset_start_content_filename = little_edian_char(buff, attribute[2].first + 20, 2) + attribute[2].first;
			int length_filename = little_edian_char(buff, offset_start_content_filename + 64, 1)*2;
			int offset_start_name =  offset_start_content_filename + 66;
			int offset_end_name =  offset_start_name + length_filename;
			string filename;
			for (int i = offset_start_name; i < offset_end_name; i++)
			{
				filename.push_back(buff[i]);
			}
			cout << filename << endl;


			// Offset 80
			int offset_80 = attribute[7].first;
			int size_80 = attribute[7].second;

			bool is_nonresident = buff[offset_80 + 8];

			if (!is_nonresident){
				int start_data = little_edian_char(buff, offset_80 + 20, 4);
				int end_data = buff[offset_80 + 4];
				int offset_start_data = offset_80 + start_data;
				int offset_end_data = offset_80 + end_data - 1; // offset truoc FF
				long long  size_of_data = offset_end_data - offset_start_data + 1;	// size data in resident
				//data start
				for(int z=offset_start_data; z < offset_end_data; z++){
					cout << buff[z];
				}

				// data end
			}else{
				long long actual_size_data = little_edian_char(buff, offset_80 + 48, 8);

				unsigned long long start_runlist = offset_80 + little_edian_char(buff, offset_80 + 32, 1);
				unsigned long long check_runlist = buff[start_runlist];

				vector< pair<unsigned long long, unsigned long long> > clusters;	// cluster bat dau - size(byte)
				unsigned long long size_fragment;
				signed long long location_fragment = 0; //clusters

				char* p;
				while(check_runlist != 0){
					stringstream ss1, ss2;
					ss1 << ((buff[start_runlist] & 0xF0) >> 4);
					ss2 << ((buff[start_runlist] & 0x0F) >> 0);

					unsigned long long s1 = strtol(ss1.str().c_str(), &p, 16);
					unsigned long long s2 = strtol(ss2.str().c_str(), &p, 16);

					size_fragment = little_edian_char(buff, start_runlist + 1, s2);
					size_fragment *= 4096; // clusters to bytes

					unsigned long long dec_compare = little_edian_char(buff, start_runlist + 1 + s2, s1);
					if ( dec_compare > pow(2,s1*8-1)){
						location_fragment = TwoElement(dec_compare);

					}else{
						location_fragment = dec_compare;
					}

					clusters.push_back(make_pair(location_fragment, size_fragment));

					start_runlist += 1 + s1 + s2;
					check_runlist = buff[start_runlist];
					cout << check_runlist << endl;
				}

				for (int i=1; i<clusters.size(); i++){
					clusters[i].first += clusters[i-1].first;	//clusters
				}

				cout << endl;
				for (int i=0; i<clusters.size(); i++){
					cout << clusters[i].first << " - " << clusters[i].second << endl;
				}
			}


		}

		void read_mft_folder(){
			char* buff = new char[512];

			// int ii=39*2;
			// while (ii != 43*2)
			// {
			// 	ReadSector(disk, buff,mft_cluster*Sc + ii);
			// 	print_sector(buff);
			// 	ii++;
			// }return;

			unsigned long long ll = find_sector_attribute("folder1");
			//cout << "right: " << mft_cluster*Sc + 39*2 << endl;
			if ( ll != 0){
				ReadSector(disk, buff, ll+5, 1);
				print_sector(buff, 1);
				cout << endl << ll << endl;
			}



			return;
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
			if (ReadSector(disk.c_str(), buff, 0, 1)){
				NTFS ntfs(disk.c_str());
				ntfs.read_pbs(buff);
				//ntfs.read_mft_file();
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
