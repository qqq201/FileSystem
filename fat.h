#ifndef FAT32_H
#define FAT32_H

#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <cstdlib>
using namespace std;

//extern const char* main_disk;
//extern int default_sector_size;

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
		vector<bool> status;
		string name;
		string root;
		bool isDeleted;
		Date created;
		Date modified;
		unsigned long long size = 0;

	public:
		Entry32(string rootName);

		void readStatus(string dataEntry, int numExtraEntry);

		void readState(string dataEntry, int numExtraEntry);

		void readClusters(string dataEntry, int numExtraEntry);

		void readName(string dataEntry, int numExtraEntry);

		void readDate(string dataEntry, int numExtraEntry);

		virtual void get_directory(int step) = 0;

		virtual unsigned long long get_size() = 0;

		virtual void readEntry(string dataEntry, int numExtraEntry) = 0;

		virtual void print_info();

		virtual void print_content() = 0;

		virtual bool type() = 0;
};

class File32 : public Entry32{
	private:
		string ext = "";
		Date created;
		Date modified;
	public:
		File32(string rootName);

		void readExt(string dataEntry, int numExtraEntry);

		void readSize(string dataEntry);



		virtual unsigned long long get_size();

		virtual void readEntry(string dataEntry, int numExtraEntry);

		virtual void print_info();

		virtual void print_content();

		virtual bool type();

		virtual void get_directory(int step);
};

class Folder32 : public Entry32 {
	private:
		vector<Entry32*> entries;
		bool isRoot = false;
	public:
		Folder32(string rootName);

		void set_as_root(vector<DWORD>& clusters);

		virtual unsigned long long get_size();

		virtual void readEntry(string dataEntry, int numExtraEntry);

		virtual void print_info();

		virtual void print_content();

		virtual bool type();

		virtual void get_directory(int step);
};

class FAT32 {
	private:
		vector<DWORD> rdet_clusters;
		Folder32* root = NULL;

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

		void get_bs_info();

		void read_rdet();

		void get_rdet_info();

		void print_tree();

		static vector<DWORD> read_fat(DWORD cluster);
};

Entry32* getEntry(string data, int extraEntries, string path);

bool get_logical_disks(string& disks);

bool ReadSector(const char *disk, char*& buff, DWORD sector);

void print_sector(char*& buff);

unsigned long long little_edian_char(char* data, int start, int nbyte);

unsigned long long little_edian_string(string data, int start, int nbyte);

string filter_name(string s, bool subentry);

#endif
