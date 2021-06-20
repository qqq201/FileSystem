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

class Entry32 {
	protected:
		string dataEntry;
		int numExtraEntry = 0;
		vector<long long> clusters;
		vector<bool> status;
		string name;
		string root;

	public:
		Entry32(string dataEntry, int numExtraEntry, string rootName);

		void readStatus();

		void readClusters();

		void readName();

		virtual void get_directory(int step) = 0;

		virtual void readEntry() = 0;

		virtual void print_info();
		
		virtual void print_content() = 0;

		virtual bool type() = 0;
};

class File32 : public Entry32{
	private:
		string ext = "";
		long long fileSize = 0;

	public:
		File32(string dataEntry, int numExtraEntry, string rootName);

		void readExt();

		void readSize();

		virtual void readEntry();

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
		Folder32(string dataEntry, int numExtraEntry, string rootName);

		void set_as_root(vector<long long>& clusters);

		virtual void readEntry();

		virtual void print_info();

		virtual void print_content();

		virtual bool type();

		virtual void get_directory(int step);
};

class FAT32 {
	private:
		vector<long long> rdet_clusters;
		Folder32* root = NULL;

	public:
		static const char* disk;
		static int Sc;
		static long long Sb;
		static int Nf;
		static long long Sv;
		static long long Sf;

		FAT32(const char* disk);
		
		~FAT32();

		static long long cluster_to_sector(long long cluster);

		bool read_bootsector(char* data);

		void get_bs_info();

		void read_rdet();

		void get_rdet_info();

		void print_tree();

		static vector<long long> read_fat(long long cluster);
};

Entry32* getEntry(string data, int extraEntries, string path);

bool get_logical_disks(string& disks);

bool ReadSector(const char *disk, char*& buff, unsigned int sector);

void print_sector(char*& buff);

long long little_edian_char(char* data, int start, int nbyte);

long long little_edian_string(string data, int start, int nbyte);

//left
static inline void ltrim(string &s);

//right
static inline void rtrim(string &s);

//both sides
static inline void trim(string &s);

string filter_name(string s);

#endif
