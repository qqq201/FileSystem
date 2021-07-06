#ifndef NTFS_H
#define NTFS_H

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

class EntryNTFS {
	public:
		string name;
		SYSTEMTIME sys = { 0 };
		bool isRoot = false;
		unsigned long long size = 0;
		EntryNTFS* parent = NULL;
    bool is_nonresident = false;
    unsigned long long mft_id;
		string file_content;

    vector<pair<DWORD, DWORD>> attributes;

		vector<pair<unsigned long long, unsigned long long>> clusters;

    vector<EntryNTFS*> childs;

		EntryNTFS(unsigned long long mft_id, EntryNTFS* parent);

    void read_index_buffer();

		void attributes_handler(char* buff, int type);

		void file_name_attribute(char* buff);

		void standard_info_attribute(char* buff);

		void data_attribute(char* buff);

		void index_root_attribute(char* buff);

		void index_location_attribute(char* buff);

    void read_content();

    bool isFolder();

    string read_file();
};

class NTFS {
  private:
		EntryNTFS* root;
    EntryNTFS* current_directory;

	public:
		static const char* disk;
		static int Sc;	// sector per cluster - unit: sector
		static unsigned long long Sv;			// size of volume - sector
		static unsigned long long mft_cluster; // cluster bat dau
		static int size_mft_entry;	//size of MFT entry - byte

		NTFS(const char* disk);

    bool read_pbs(char* buff);

    void read_mft();
};

bool get_logical_disks(string& disks);

bool ReadSector(const char *disk, char* buff, unsigned long long sector, unsigned long long nsector);

void print_sector(char* buff, int n);

signed long long TwoElement(unsigned long long num);

unsigned long long little_edian_char(char* data, int start, int nbyte);

#endif
