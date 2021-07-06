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
		string ext;
		string root;
		SYSTEMTIME modified = { 0 };
		bool isRoot = false;
		unsigned long long size = 0;
		EntryNTFS* parent = NULL;
    bool is_nonresident = false;
    unsigned long long mft_id;
		string file_content;

    vector<pair<DWORD, DWORD>> attributes;

		vector<pair<unsigned long long, unsigned long long>> clusters;

    vector<EntryNTFS*> entries;

		EntryNTFS(unsigned long long mft_id, string path, EntryNTFS* parent);

		~EntryNTFS();

    void read_index_buffer();

		void attributes_handler(char* buff, int type);

		void file_name_attribute(char* buff);

		void standard_info_attribute(char* buff);

		void data_attribute(char* buff);

		void index_root_attribute(char* buff);

		void index_location_attribute(char* buff);

    void read_content();

    bool type();

    string get_content();

		string get_modified_date();

		string get_size();

		vector<string> get_info();

		vector<EntryNTFS*> get_directory();

		string get_path();

		EntryNTFS* get_entry(int id);
};

class NTFS {
  private:
		EntryNTFS* root = NULL;
    EntryNTFS* current_directory = NULL;

	public:
		static const char* disk;
		static int Sc;	// sector per cluster - unit: sector
		static unsigned long long Sv;			// size of volume - sector
		static unsigned long long mft_cluster; // cluster bat dau
		static int size_mft_entry;	//size of MFT entry - byte

		NTFS(const char* disk);

		~NTFS();

    bool read_pbs(char* buff);

    void read_mft();

		string get_pbs_info();

		EntryNTFS* change_directory(int id);

		EntryNTFS* back_parent_directory();

		EntryNTFS* get_current_directory();

		string get_cd_path();
};

bool get_logical_disks(string& disks);

bool ReadSector(const char *disk, char* buff, unsigned long long sector, unsigned long long nsector);

void print_sector(char* buff, int n);

signed long long TwoElement(unsigned long long num);

unsigned long long little_edian_char(char* data, int start, int nbyte);

#endif
