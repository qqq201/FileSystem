#include <cstdio>
#include <windows.h>
using namespace std;

int main() {
	const int MAX_USB_GB = 32;
	printf("Drive letter of USB?: ");
	char path[] = "\\\\.\\?:";
	scanf_s("%c", path + 4, 1);

	// open the volume 
	HANDLE hVol = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	// if handle fails 
	if (INVALID_HANDLE_VALUE == hVol) {
		cout << "Check drive letter.";
		return -1;
	}

	VOLUME_DISK_EXTENTS vde = { 0 };

	DWORD dw;

	DeviceIoControl(hVol, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, (LPVOID)&vde, (DWORD)sizeof(VOLUME_DISK_EXTENTS), &dw, NULL);

	// check disk size 
	if (vde.Extents[0].ExtentLength.QuadPart > LONGLONG(MAX_USB_GB * 1000000000LL))
	{
		cout << "USB too large\n";
		printf("Use < %d GB", MAX_USB_GB);
		CloseHandle(hVol);
		return -2;
	}

	// open the disk now by using 
	// the disk number 
	// path format is \\.\PhysicalDriveN 
	// so a buffer of 20 sufficient 
	char diskPath[20] = { 0 };

	// use sprintf to construct 
	// the path by appending 
	// disk number to \\.\PhysicalDrive 
	sprintf_s(diskPath, sizeof(diskPath), "\\\\.\\PhysicalDrive%d", vde.Extents[0].DiskNumber);

	// open a handle to the disk 
	HANDLE hUsb = CreateFileA(diskPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	// if handle fails 
	if (INVALID_HANDLE_VALUE == hUsb) {
		printf("Run as admn.");
		CloseHandle(hVol);
		return -3;
	}

	BYTE sector[512];
	dw = 0;

	ReadFile(hUsb, sector, sizeof(sector), &dw, NULL);

	cout << "Boot sector: \n";

	for (int i = 0; i < 512; i++){
		if (0 == i % 16) {
			cout << endl;
		}
		BYTE b = sector[i];
		printf("%c ", isascii(b) ? b : '.');
	}

	// release handles 
	CloseHandle(hVol);
	CloseHandle(hUsb);
	return 0;
}
