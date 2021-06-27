#include <gdiplus.h>
#include "fat.h"
#define BACKBUTTON1 1
#define BACKBUTTON2 2

string disks;
bool read_folder = true;
string current_disk = "\\\\.\\?:";

HWND main_hwnd, fs_hwnd, directory_hwnd, menu_hwnd;
HINSTANCE ghInstance;

FAT32* fat = NULL;

void filesystem_handle(){
  fat = new FAT32(current_disk.c_str());
  char* buff = new char[512];
  ReadSector(current_disk.c_str(), buff, 0);
  fat->read_bootsector(buff);
  fat->read_rdet();
}

LRESULT CALLBACK main_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK menu_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK fs_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK direc_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

BOOL CALLBACK DestoryChildCallback(HWND hwnd, LPARAM lParam);

void load_menu_window(HWND hwnd);

void load_fs_window(HWND hwnd);

void load_directory_window(HWND hwnd);

void load_disks(HWND hwnd);

void load_directory(HWND hwnd);

void main_to_fs(HWND hwnd);

void draw_button(LPDRAWITEMSTRUCT Item, wchar_t* file, UINT text_align);

void print_fs_info(HDC hdc);

void draw_title_bar(HDC hdc);

void print_file(HDC hdc);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR args, int nCmdShow){
  Gdiplus::GdiplusStartupInput gpStartupInput;
  ULONG_PTR gpToken;

  Gdiplus::GdiplusStartup(&gpToken, &gpStartupInput, NULL);

  WNDCLASSW wc = {0};
  MSG  msg;

  wc.lpszClassName = L"Main";
  wc.hInstance     = hInstance;
  wc.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
  wc.lpfnWndProc   = main_proc;
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

  RegisterClassW(&wc);
  main_hwnd = CreateWindowW(wc.lpszClassName, L"Filesystem reader", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 960, 720, NULL, NULL, hInstance, NULL);

  ghInstance = hInstance;

  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  Gdiplus::GdiplusShutdown(gpToken);
  return (int) msg.wParam;
}

LRESULT CALLBACK main_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch(msg) {
    case WM_CREATE:{
      load_menu_window(hwnd);
      load_fs_window(hwnd);
      ShowWindow(fs_hwnd, SW_HIDE);
      break;
    }
    case WM_DESTROY:{
      PostQuitMessage(0);
      break;
    }
  }

  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK menu_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
  switch(msg) {
    case WM_DRAWITEM:{
      LPDRAWITEMSTRUCT Item = (LPDRAWITEMSTRUCT)lParam;
      for (int i = 0; i < disks.size(); ++i){
          if (Item->CtlID == i){
            draw_button(Item, L"assets/disk.png", DT_CENTER);
          }
      }
      break;
    }
    case WM_COMMAND:{
      int id = LOWORD(wParam);
      if (id < disks.length()){
        current_disk[4] = disks[id];
        main_to_fs(hwnd);
      }
      break;
    }
    case WM_CREATE:{
      load_disks(hwnd);
      break;
    }
    case WM_DESTROY:{
      PostQuitMessage(0);
      break;
    }
  }

  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

HWND path_hwnd;

LRESULT CALLBACK fs_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
  switch(msg) {
    case WM_PAINT: {
      //draw info
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hwnd, &ps);

      print_fs_info(hdc);

      EndPaint(hwnd, &ps);
      ReleaseDC(hwnd, hdc);
      break;
    }
    case WM_DRAWITEM:{
      //draw back button
      LPDRAWITEMSTRUCT Item = (LPDRAWITEMSTRUCT)lParam;
      FillRect(Item->hDC, &Item->rcItem, CreateSolidBrush(0xFFFFFF));
      SelectObject(Item->hDC, CreateSolidBrush(0xFFFFFF));
      SetTextColor(Item->hDC, 0);
      SetBkMode(Item->hDC, TRANSPARENT);
      int len = GetWindowTextLength(Item->hwndItem);
      LPWSTR lpBuff;
      lpBuff = new wchar_t[len + 1];
      GetWindowTextW(Item->hwndItem, lpBuff, len+1);
      DrawTextW(Item->hDC, lpBuff, len, &Item->rcItem, DT_LEFT);
      break;
    }
    case WM_CREATE:{
      load_directory_window(hwnd);
      CreateWindowW(L"Button", L"🡠", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 10, 0, 30, 30, hwnd, (HMENU) BACKBUTTON1, NULL, NULL);
      CreateWindowW(L"Button", L"🡠", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 205, 15, 25, 25, hwnd, (HMENU) BACKBUTTON2, NULL, NULL);
      break;
    }
    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
        case BACKBUTTON1: {
          ShowWindow(menu_hwnd, SW_SHOW);
          ShowWindow(hwnd, SW_HIDE);
          break;
        }
        case BACKBUTTON2: {
          fat->get_current_directory();
          Entry32* cd = fat->back_parent_directory();
          read_folder = cd->type();
          InvalidateRect(directory_hwnd, NULL, TRUE);
          RedrawWindow(directory_hwnd, NULL, NULL, RDW_INTERNALPAINT);
          break;
        }
      }
      break;
    }
    case WM_DESTROY:{
      PostQuitMessage(0);
      break;
    }
  }

  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

BOOL CALLBACK DestoryChildCallback(HWND hwnd, LPARAM lParam){
  if (hwnd != NULL) {
    DestroyWindow(hwnd);
  }
  return TRUE;
}

LRESULT CALLBACK directory_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
  switch(msg) {
    case WM_PAINT: {
      string path = fat->get_cd_path();
      path_hwnd = CreateWindowA("Static", &path[0], WS_VISIBLE | WS_CHILD | SS_OWNERDRAW, 250, 15, 650, 25, fs_hwnd, NULL, NULL, NULL);

      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hwnd, &ps);

      if (read_folder){
        load_directory(directory_hwnd);
        draw_title_bar(hdc);
      }
      else{
        print_file(hdc);
      }
      EndPaint(hwnd, &ps);
      ReleaseDC(hwnd, hdc);
      break;
    }
    case WM_DRAWITEM:{
      Entry32* current_directory = fat->get_current_directory();
      LPDRAWITEMSTRUCT Item = (LPDRAWITEMSTRUCT)lParam;
      if (current_directory->type()){
        vector<Entry32*> content = current_directory->get_directory();

        for (int i = 0; i < content.size(); ++i){
            if (Item->CtlID == i){
              if (content[i]->type())
                draw_button(Item, L"assets/folder-icon.png", DT_EXPANDTABS | DT_TABSTOP | 0x0400);
              else
                draw_button(Item, L"assets/file-icon.png", DT_EXPANDTABS | DT_TABSTOP | 0x0400);
            }
        }
      }
      break;
    }
    case WM_CREATE: {
      break;
    }
    case WM_COMMAND: {
      //InvalidateRect(fs_hwnd, NULL, TRUE);
      //RedrawWindow(fs_hwnd, NULL, NULL, RDW_INTERNALPAINT);
      SetWindowTextA(path_hwnd, fat->get_cd_path().c_str());

      int id = LOWORD(wParam);
      Entry32* cd = fat->change_directory(id);
      read_folder = cd->type();
      EnumChildWindows(hwnd, DestoryChildCallback, 0);
      InvalidateRect(hwnd, NULL, TRUE);
      RedrawWindow(hwnd, NULL, NULL, RDW_INTERNALPAINT);
      break;
    }
    case WM_DESTROY:{
      PostQuitMessage(0);
      break;
    }
  }

  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK file_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
  switch(msg) {
    case WM_PAINT: {
      //draw info
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hwnd, &ps);
      RECT rec = {};
      SetRect(&rec, 5, 5, 200, 500);

      string info = fat->get_current_directory()->get_content();
      DrawText(hdc, &info[0], info.length(), &rec, DT_TOP|DT_LEFT);

      EndPaint(hwnd, &ps);
      ReleaseDC(hwnd, hdc);
      break;
    }
    case WM_DESTROY:{
      PostQuitMessage(0);
      break;
    }
  }

  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void load_menu_window(HWND hwnd){
	WNDCLASSW wc = {0};

  wc.lpszClassName = L"menu";
  wc.hInstance     = ghInstance;
  wc.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
  wc.lpfnWndProc   = menu_proc;
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

  RECT rect;
  int width, height;
  if(GetWindowRect(hwnd, &rect))
  {
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
  }

  int menu_width = 100, menu_height = height;

  int x = width / 2 - menu_width / 2;
  RegisterClassW(&wc);
  menu_hwnd = CreateWindowW(wc.lpszClassName, NULL, WS_VISIBLE | WS_CHILD, x, 0, menu_width, menu_height, hwnd, NULL, ghInstance, NULL);
}

void load_fs_window(HWND hwnd){
	WNDCLASSW wc = {0};

  wc.lpszClassName = L"filesystem";
  wc.hInstance     = ghInstance;
  wc.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
  wc.lpfnWndProc   = fs_proc;
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

  RegisterClassW(&wc);
  fs_hwnd = CreateWindowW(wc.lpszClassName, NULL, WS_VISIBLE | WS_CHILD, 0, 0, 960, 720, hwnd, NULL, ghInstance, NULL);
}

void load_disks(HWND hwnd){
  int width = 0, height = 0, button_width = 90, button_height = 20;
  RECT rect;

  if(GetWindowRect(hwnd, &rect))
  {
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
  }

  int start_x = width / 2 - button_width / 2;
  int start_y = height / 2 - 50;

	if (get_logical_disks(disks)){
    int i = 0;
    for (char disk : disks){
      string fuldisk;
      fuldisk += disk;
      fuldisk += ":\\";
      CreateWindowA("Button", fuldisk.c_str(), WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, start_x, start_y + i * (button_height + 15), button_width, button_height, hwnd, (HMENU) i, NULL, NULL);
      ++i;
    }
  }
}

void load_directory_window(HWND hwnd){
	WNDCLASSW wc = {0};

  wc.lpszClassName = L"Directory";
  wc.hInstance     = ghInstance;
  wc.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
  wc.lpfnWndProc   = directory_proc;
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

  RegisterClassW(&wc);
  directory_hwnd = CreateWindowW(wc.lpszClassName, NULL, WS_VISIBLE | WS_CHILD | WS_BORDER, 220, 50, 740, 670, hwnd, NULL, ghInstance, NULL);
}

void load_directory(HWND hwnd){
  Entry32* current_directory = fat->get_current_directory();
  vector<Entry32*> content = current_directory->get_directory();

  int button_height = 20, button_width = 740, i = 0;
  for (Entry32* entry : content){
    CreateWindowA("Button", ("\t" + entry->get_info()).c_str(), WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 5, 40 + i * (button_height + 15), button_width, button_height, hwnd, (HMENU) i, NULL, NULL);
    ++i;
  }
}

void main_to_fs(HWND hwnd){
  ShowWindow(hwnd, SW_HIDE);
  ShowWindow(fs_hwnd, SW_SHOWNORMAL);
  filesystem_handle();
  //UpdateWindow(disk_hwnd);
}

void draw_button(LPDRAWITEMSTRUCT Item, wchar_t* file, UINT text_align){
  SelectObject(Item->hDC, CreateFont(16, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial"));
  FillRect(Item->hDC, &Item->rcItem, CreateSolidBrush(0xFFFFFF));
  SelectObject(Item->hDC, CreateSolidBrush(0xFFFFFF));

  SetTextColor(Item->hDC, 0);

  SetBkMode(Item->hDC, TRANSPARENT);

  Gdiplus::Graphics graphics(Item->hDC);

  Gdiplus::Image disk_icon(file);
 // Draw the original source image.
  graphics.DrawImage(&disk_icon, 0, 0);

  int len = GetWindowTextLength(Item->hwndItem);
  LPSTR lpBuff;
  lpBuff = new char[len+1];
  GetWindowTextA(Item->hwndItem, lpBuff, len+1);
  DrawTextA(Item->hDC, lpBuff, len, &Item->rcItem, text_align);
}

void print_fs_info(HDC hdc){
  Gdiplus::Graphics graphics(hdc);
  Gdiplus::Pen pen(Gdiplus::Color(255, 57, 57, 57));
  Gdiplus::SolidBrush brush(Gdiplus::Color(255, 0, 0, 0));
  Gdiplus::FontFamily fontFamily(L"Arial");
  Gdiplus::Font font(&fontFamily, 14, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);

  Gdiplus::PointF point1(10, 50);
  Gdiplus::PointF point2(230, 15);

  string info = "FAT32\n\n" + fat->get_bs_info();
  wstringstream wss;
  wss << info.c_str();
  graphics.DrawString(wss.str().c_str(), -1, &font, point1, &brush);
  graphics.DrawRectangle(&pen, 200, 10, 700, 30);
}

void print_file(HDC hdc){
  Entry32* current_directory = fat->get_current_directory();
  Gdiplus::Graphics graphics(hdc);
  Gdiplus::SolidBrush brush(Gdiplus::Color(255, 0, 0, 0));
  Gdiplus::FontFamily fontFamily(L"Arial");
  Gdiplus::Font font(&fontFamily, 14, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
  Gdiplus::PointF point1(5, 5);

  string content = current_directory->get_content();
  wstringstream wss;
  wss << content.c_str();
  graphics.DrawString(wss.str().c_str(), -1, &font, point1, &brush);
}

void draw_title_bar(HDC hdc) {
  Gdiplus::Graphics graphics(hdc);
  Gdiplus::Pen pen(Gdiplus::Color(255, 57, 57, 57));
  graphics.DrawLine(&pen, 300, 0, 300, 25);
  graphics.DrawLine(&pen, 450, 0, 450, 25);
  graphics.DrawLine(&pen, 600, 0, 600, 25);
  Gdiplus::SolidBrush brush(Gdiplus::Color(255, 0, 0, 0));
  Gdiplus::FontFamily fontFamily(L"Arial");
  Gdiplus::Font font(&fontFamily, 14, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
  Gdiplus::PointF point1(15, 5);
  Gdiplus::PointF point2(315, 5);
  Gdiplus::PointF point3(465, 5);
  Gdiplus::PointF point4(615, 5);

  graphics.DrawString(L"Name", -1, &font, point1, &brush);
  graphics.DrawString(L"Date modified", -1, &font, point2, &brush);
  graphics.DrawString(L"Type", -1, &font, point3, &brush);
  graphics.DrawString(L"Size", -1, &font, point4, &brush);
}
