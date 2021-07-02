#ifndef FAT32_H
#define FAT32_H

#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iomanip>
using namespace std;

struct Date {
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int second;
};

class Entry32 {
	protected:
		vector<DWORD> clusters;

	public:
		vector<bool> status;
		string name;
		string ext;
		string root;
		bool isRoot = false;
		bool isDeleted;
		Date created;
		Date modified;
		unsigned long long size = 0;
		Entry32* parent = NULL;

		Entry32(string rootName, Entry32* parent);
		~Entry32();

		void readStatus(string dataEntry, int numExtraEntry);

		void readState(string dataEntry, int numExtraEntry);

		void readClusters(string dataEntry, int numExtraEntry);

		void readName(string dataEntry, int numExtraEntry);

		void readDate(string dataEntry, int numExtraEntry);

		string get_modified_date();

		string get_size();

		virtual void clear() = 0;

		virtual string get_path() = 0;

		virtual vector<Entry32*> get_directory() = 0;

		virtual string get_content() = 0;

		virtual Entry32* get_entry(int id) = 0;

		virtual void readEntry(string dataEntry, int numExtraEntry) = 0;

		virtual bool type() = 0; //file: false, folder: true
};

class File32 : public Entry32{
	public:
		File32(string rootName, Entry32* parent);
		~File32();

		void readExt(string dataEntry, int numExtraEntry);

		void readSize(string dataEntry);

		virtual void clear();

		virtual vector<Entry32*> get_directory();

		virtual string get_content();

		virtual void readEntry(string dataEntry, int numExtraEntry);

		virtual string get_path();

		virtual bool type();

		virtual Entry32* get_entry(int id);
};

class Folder32 : public Entry32 {
	public:
		vector<Entry32*> entries;

		Folder32(string rootName, Entry32* parent);
		~Folder32();

		void set_as_root(vector<DWORD>& clusters);

		virtual void clear();

		virtual vector<Entry32*> get_directory();

		virtual string get_content();

		virtual void readEntry(string dataEntry, int numExtraEntry);

		virtual string get_path();

		virtual bool type();

		virtual Entry32* get_entry(int id);
};

class FAT32 {
	private:
		vector<DWORD> rdet_clusters;
		Folder32* root = NULL;
		Entry32* current_directory = NULL;
	public:
		static const char* disk;
		static int Sc;
		static WORD Sb;
		static int Nf;
		static DWORD Sv;
		static DWORD Sf;

		FAT32(const char* disk);

		~FAT32();

		static DWORD cluster_to_sector(DWORD cluster);

		bool read_bootsector(char* data);

		string get_bs_info();

		void read_rdet();

		string get_path();

		static vector<DWORD> read_fat(DWORD cluster);

		Entry32* change_directory(int id);

		Entry32* back_parent_directory();

		Entry32* get_current_directory();

		string get_cd_path();
};

Entry32* getEntry(string data, int extraEntries, string path);

bool get_logical_disks(string& disks);

bool ReadSector(const char *disk, char*& buff, unsigned long long sector);

void print_sector(char*& buff);

unsigned long long little_edian_char(char* data, int start, int nbyte);

unsigned long long little_edian_string(string data, int start, int nbyte);

string filter_name(string s, bool subentry);

#endif
