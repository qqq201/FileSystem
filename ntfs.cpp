#include "ntfs.h"

char const hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
WORD default_sector_size = 512;

const char* NTFS::disk = NULL;
int NTFS::Sc = 8;	// sector per cluster - unit: sector
unsigned long long NTFS::Sv = 0;			// size of volume - sector
unsigned long long NTFS::mft_cluster = 0; // cluster bat dau
int NTFS::size_mft_entry = 2;

vector<bool> read(100, false);

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

signed long long TwoElement(unsigned long long num){
  // Find binary vector for number
  vector<int> bit_vector;

  while (num >  0) {
    bit_vector.push_back(num % 2);
    num /= 2;
  }
	reverse(bit_vector.begin(), bit_vector.end());

  // substract -1 from binary
  bool borrow_bit = 0;
  if (bit_vector.back() == 0){
      borrow_bit =1;
      bit_vector.back() = 1;
  }
	else{
      bit_vector.back() = 0;
  }

  int i = bit_vector.size() - 2;

  while (borrow_bit != 0) {
    if(bit_vector[i] == 0){
      borrow_bit = 1;
      bit_vector[i] = 1;
      i = i - 1;
    }
		else{
      borrow_bit = 0;
      bit_vector[i] = 0;
    }
  }

  // XOR
  vector<int> vt;
  for(int j = 0; j < bit_vector.size(); ++j){
      if(bit_vector[j] == 1) vt.push_back(0);
      else vt.push_back(1);
  }

  signed long long result = 0;
  int m = 0;
  for(int i = vt.size() - 1; i >= 0; --i){
    result += vt[i] * (1 << m);
    ++m;
  }

  return -1 * result;
}


EntryNTFS::EntryNTFS(unsigned long long mft_id, EntryNTFS* parent) {
	this->mft_id = mft_id;
	this->parent = parent;
}

void EntryNTFS::read_index_buffer(){
	if (is_nonresident){
		cout << "File in this folder:\n";
		for (auto cluster : clusters){
			//index header 24 bytes
			int ih_start_offset = 0;
			int nh_start_offset = 24;

			// check if this is an index buffer
			char* buff = new char[cluster.second];
			ReadSector(NTFS::disk, buff, cluster.first * NTFS::Sc, cluster.second / default_sector_size);

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

				if (index_length > 16 && !read[mft_id]){
					EntryNTFS* new_file = new EntryNTFS(mft_id, this);
					new_file->read_content();
					childs.push_back(new_file);
				}

				eh_start_offset += index_length;
				index_node_length -= index_length;
			}
			delete[] buff;
		}
	}
}

void EntryNTFS::file_name_attribute(char* buff) {
	DWORD start_offset = attributes[2].first;
	DWORD length = attributes[2].second;

	int offset_start_content_filename = little_edian_char(buff, start_offset + 20, 2) + start_offset;
	int length_filename = little_edian_char(buff, offset_start_content_filename + 64, 1)*2;
	int offset_start_name =  offset_start_content_filename + 66;
	int offset_end_name =  offset_start_name + length_filename;

	for (int i = offset_start_name; i < offset_end_name; i++)
		name.push_back(buff[i]);
}

void EntryNTFS::standard_info_attribute(char* buff){
	DWORD start_offset = attributes[0].first;
	DWORD length = attributes[0].second;

	int offset_creation_time = little_edian_char(buff, start_offset + 20, 2) + start_offset;
	int offset_modified_time = offset_creation_time + 8;

	FILETIME ft = { 0 };
	ft.dwHighDateTime = little_edian_char(buff, offset_modified_time + 4, 4);
	ft.dwLowDateTime = little_edian_char(buff, offset_modified_time, 4);

	FileTimeToSystemTime(&ft, &this->sys);
}

void EntryNTFS::data_attribute(char* buff){
	DWORD start_offset = attributes[7].first;
	DWORD length = attributes[7].second;

	is_nonresident = buff[start_offset + 8];

	if (!is_nonresident){
		int start_data = little_edian_char(buff, start_offset + 20, 4);
		int end_data = buff[start_offset + 4];
		int offset_start_data = start_offset + start_data;
		int offset_end_data = start_offset + end_data - 1; // offset truoc FF
		long long  size_of_data = offset_end_data - offset_start_data + 1;	// size data in resident

		//data start
		file_content.erase();
		for(int z = offset_start_data; z < offset_end_data; ++z)
			file_content.push_back(buff[z]);
	}
	else{
		long long actual_size_data = little_edian_char(buff, start_offset + 48, 8);

		unsigned long long start_runlist = start_offset + little_edian_char(buff, start_offset + 32, 2);
		unsigned long long check_runlist = buff[start_runlist];

		vector<pair<unsigned long long, unsigned long long>> clusters;	// cluster bat dau - size(byte)
		unsigned long long size_fragment;
		signed long long location_fragment = 0; //clusters

		while(check_runlist != 0){
			int s1 = (buff[start_runlist] & 0xF0) >> 4;
			int s2 = (buff[start_runlist] & 0x0F) >> 0;

			size_fragment = little_edian_char(buff, start_runlist + 1, s2) * NTFS::Sc * default_sector_size;

			unsigned long long dec_compare = little_edian_char(buff, start_runlist + 1 + s2, s1);

			if (dec_compare > (1 << s1 * 8 - 1))
				location_fragment = TwoElement(dec_compare);
			else
				location_fragment = dec_compare;

			clusters.push_back(make_pair(location_fragment, size_fragment));

			start_runlist += 1 + s1 + s2;
			check_runlist = buff[start_runlist];
		}

		for (int i = 1; i < clusters.size(); ++i){
			clusters[i].first += clusters[i - 1].first;
		}
	}
}

void EntryNTFS::index_root_attribute(char* buff){
	DWORD start_offset = attributes[8].first;
	DWORD length = attributes[8].second;

	int offset_to_AtrContent = little_edian_char(buff, start_offset + 20, 2) + start_offset;
	int att_type = little_edian_char(buff, offset_to_AtrContent, 4);
	is_nonresident = little_edian_char(buff, offset_to_AtrContent + 28 , 4);

	if(!is_nonresident && attributes[9].first == 0 && att_type == 48){
		int offset_start_node = offset_to_AtrContent + 16 + little_edian_char(buff, offset_to_AtrContent + 16, 4);
		int length_node = little_edian_char(buff, offset_to_AtrContent + 20, 4) - 16;

		while (length_node > 0){
			int mft_id = little_edian_char(buff, offset_start_node, 6);
			int index_length = little_edian_char(buff, offset_start_node + 8, 2);
			if (index_length > 16 && !read[mft_id]){
				EntryNTFS* new_file = new EntryNTFS(mft_id, this);
				new_file->read_content();
				childs.push_back(new_file);
			}
			length_node -= index_length;
			offset_start_node += index_length;
		}
	}
}

void EntryNTFS::index_location_attribute(char* buff){
	DWORD start_offset = attributes[9].first;
	DWORD length = attributes[9].second;

	unsigned long long start_runlist = start_offset + little_edian_char(buff, start_offset + 32, 2);
	unsigned long long check_runlist = buff[start_runlist];

	unsigned long long size_fragment;
	signed long long location_fragment = 0; //clusters

	while(check_runlist != 0){
		int s1 = (buff[start_runlist] & 0xF0) >> 4;
		int s2 = (buff[start_runlist] & 0x0F) >> 0;

		size_fragment = little_edian_char(buff, start_runlist + 1, s2) * NTFS::Sc * default_sector_size;

		unsigned long long dec_compare = little_edian_char(buff, start_runlist + 1 + s2, s1);

		if (dec_compare > (1 << s1 * 8 - 1))
			location_fragment = TwoElement(dec_compare);
		else
			location_fragment = dec_compare;

		clusters.push_back(make_pair(location_fragment, size_fragment));
		start_runlist += 1 + s1 + s2;
		check_runlist = buff[start_runlist];
	}

	for (int i = 1; i < clusters.size(); ++i){
		clusters[i].first += clusters[i - 1].first;	//clusters
	}

	read_index_buffer();
}

void EntryNTFS::attributes_handler(char* buff, int type) {
	if (attributes[type].second == 0)
		return;

	switch(type){
		case 0: //x10
			standard_info_attribute(buff);
			break;
		case 2: //x30
			file_name_attribute(buff);
			break;
		case 7: //x80
			data_attribute(buff);
			break;
		case 8: //x90
			index_root_attribute(buff);
			break;
		case 9: //xA0
			index_location_attribute(buff);
			break;
	}
}

void EntryNTFS::read_content() {
	read[mft_id] = true;

	char* buff = new char[default_sector_size * 2];
	ReadSector(NTFS::disk, buff, mft_id * 2 + NTFS::mft_cluster * NTFS::Sc, 2);

	unsigned long long  offset_to_start;
	offset_to_start = little_edian_char(buff, 20, 2);

	attributes.push_back(make_pair(0, 0));
	attributes[0].first = offset_to_start;
	attributes[0].second = little_edian_char(buff, offset_to_start + 4, 4);

	for (int i = 1; i < 10; ++i){
		attributes.push_back(make_pair(0, 0));

		int location = attributes[0].first;
		for (int j = i - 1; j >= 0; --j)
			location +=	attributes[j].second;

		int type = little_edian_char(buff, location, 4);

		if ((i + 1) * 16 == type){
			attributes[i].first = location;
			attributes[i].second = little_edian_char(buff, attributes[i].first + 4, 4);
		}
		else{
			continue;
		}
	}

	//for (auto att : attributes)
	//	cout << att.first << " " << att.second << endl;

	for(int i = 0; i < 10; ++i){
		attributes_handler(buff, i);
	}
}

string EntryNTFS::read_file(){
	if (is_nonresident){
		string str;
		for (auto cluster : clusters){
			char* buff = new char[cluster.second];
			ReadSector(NTFS::disk, buff, cluster.first * NTFS::Sc, cluster.second / default_sector_size);
			for (int i = 0; i < cluster.second; ++i)
				str.push_back(buff[i]);
			delete[] buff;
		}

		return str;
	}
	else {
		return file_content;
	}
}

bool EntryNTFS::isFolder(){
	return (attributes[8].first > 0 && attributes[8].second > 0);
}

NTFS::NTFS(const char* disk) { this->disk = disk; }

bool NTFS::read_pbs(char* buff) {
	if((unsigned char)buff[510] == 0x55 && (unsigned char)buff[511] == 0xaa){
		print_sector(buff, 1);
		default_sector_size = little_edian_char(buff, 11, 2);
		Sc = buff[13];
		mft_cluster = little_edian_char(buff, 48, 8);
		size_mft_entry = little_edian_char(buff, 64, 1);
		Sv = little_edian_char(buff, 40, 8);
		return true;
	}
	else
		return false;
}

void NTFS::read_mft(){
	root = new EntryNTFS(5, NULL);
	root->read_content();
}
