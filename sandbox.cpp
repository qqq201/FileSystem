#include <iostream>
#include <vector>

using namespace std;

int main()
{
	string status[6] = { "Read Only", "Hidden", "System", "Vol Label", "Directory", "Archive" };
	string result = "";
	result += status[1];
	result += status[4];
	cout << result << endl;
	system("pause");
}