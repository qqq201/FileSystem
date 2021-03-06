#include "filesystem.h"
#include <gdiplus.h>
#define BACKBUTTON1 1
#define BACKBUTTON2 2
#define SHOWDELETED 3

bool read_folder = true, drawn = false, show_deleted = true, fs_type = false; //FAT32: true, NTFS: false
string current_disk = "\\\\.\\?:";
string disks;

HWND main_hwnd, fs_hwnd, directory_hwnd, menu_hwnd;
HINSTANCE ghInstance;

string file_content;
vector<Entry32*> entries;

RECT path_rect = {230, 15, 720, 40};

FAT32* fat = NULL;
NTFS* ntfs = NULL;

int nline = 0, s_prevx = 1, s_prevy = 1;

void filesystem_handler(){
  char* buff = new char[512];
  ReadSector(current_disk.c_str(), buff, 0, 1);
  string fat32_str, ntfs_str;

  for (int i = 3; i < 7; ++i){
    ntfs_str.push_back(buff[i]);
  }

  for (int i = 82; i < 87; ++i)
    fat32_str.push_back(buff[i]);

  if (fat32_str.compare("FAT32") == 0){
    fs_type = true;
    fat = new FAT32(current_disk.c_str());
    if (fat->read_bootsector(buff))
      fat->read_rdet();
    else{
      delete fat;
      fat = NULL;
    }
  }
  else if (ntfs_str.compare("NTFS") == 0){
    fs_type = false;
    ntfs = new NTFS(current_disk.c_str());
    if (ntfs->read_pbs(buff))
      ntfs->read_mft();
    else {
      delete ntfs;
      ntfs = NULL;
    }
  }
}

int max(int a, int b){
  return (a > b)? a : b;
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

void draw_entry_button(LPDRAWITEMSTRUCT Item, vector<string> info, bool is_folder);

void print_fs_info(HDC hdc);

void draw_title_bar(HDC hdc, int pos);

void load_file(){
  if (fs_type){
    if (fat){
      Entry32* entry = fat->get_current_directory();
      if (entry)
        file_content = entry->get_content();
    }
  }
  else {
    if (ntfs){
      EntryNTFS* entry = ntfs->get_current_directory();
      if (entry)
        file_content = entry->get_content();
    }
  }
}

void print_file(HDC hdc, int pos);

void SD_OnVScroll(HWND hwnd, UINT code);

void SD_ScrollClient(HWND hwnd, int bar, int pos);

int SD_GetScrollPos(HWND hwnd, int bar, UINT code);

void reset_scroll(HWND hwnd);

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
      EnumChildWindows(hwnd, DestoryChildCallback, 0);
      PostQuitMessage(0);
      break;
    }
  }

  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void reset_scroll(HWND hwnd){
  SCROLLINFO si = {};
  si.cbSize = sizeof(SCROLLINFO);
  si.fMask = SIF_ALL;
  GetScrollInfo(hwnd, SB_VERT, &si);
  si.nPos = 0;
  s_prevx = 1;
  s_prevy = 1;
  si.nTrackPos = 0;
  si.nMax = max(nline * 630 / 37, 1);
  SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
}

LRESULT CALLBACK menu_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
  switch(msg) {
    case WM_DRAWITEM:{
      LPDRAWITEMSTRUCT Item = (LPDRAWITEMSTRUCT)lParam;
      for (int i = 0; i < disks.size(); ++i){
        if (Item->CtlID == i){
          draw_button(Item, (LPWSTR)L"assets/disk.png", DT_CENTER);
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
      load_directory(directory_hwnd);
      reset_scroll(directory_hwnd);
      break;
    }
    case WM_CREATE:{
      load_disks(hwnd);
      break;
    }
    case WM_DESTROY:{
      EnumChildWindows(hwnd, DestoryChildCallback, 0);
      PostQuitMessage(0);
      break;
    }
  }

  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

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
      CreateWindowW(L"Button", L"????", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 10, 0, 30, 30, hwnd, (HMENU) BACKBUTTON1, NULL, NULL);
      CreateWindowW(L"Button", L"????", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 205, 15, 25, 25, hwnd, (HMENU) BACKBUTTON2, NULL, NULL);
      if (fs_type)
        CreateWindowW(L"button", L"Show deleted files/folders", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 10, 300, 200, 20, hwnd, (HMENU) SHOWDELETED, NULL, NULL);
      CheckDlgButton(hwnd, SHOWDELETED, BST_CHECKED);
      break;
    }
    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
        case BACKBUTTON1: {
          if (fs_type){
            delete fat;
            fat = NULL;
          }
          else {
            delete ntfs;
            ntfs = NULL;
          }
          reset_scroll(directory_hwnd);
          EnumChildWindows(directory_hwnd, DestoryChildCallback, 0);
          file_content.clear();
          read_folder = true;
          InvalidateRect(directory_hwnd, NULL, TRUE);

          ShowWindow(menu_hwnd, SW_SHOW);
          ShowWindow(hwnd, SW_HIDE);
          break;
        }
        case BACKBUTTON2: {
          if (fs_type)
            fat->back_parent_directory();
          else
            ntfs->back_parent_directory();

          read_folder = true;
          EnumChildWindows(directory_hwnd, DestoryChildCallback, 0);
          InvalidateRect(directory_hwnd, NULL, TRUE);
          RedrawWindow(directory_hwnd, NULL, NULL, RDW_INTERNALPAINT);
          InvalidateRect(fs_hwnd, &path_rect, TRUE);
          RedrawWindow(fs_hwnd, &path_rect, NULL, RDW_INTERNALPAINT);
          load_directory(directory_hwnd);
          reset_scroll(directory_hwnd);
          break;
        }
        case SHOWDELETED: {
          show_deleted = !show_deleted;
          if (read_folder){
            drawn = false;
            EnumChildWindows(directory_hwnd, DestoryChildCallback, 0);
            InvalidateRect(directory_hwnd, NULL, TRUE);
            RedrawWindow(directory_hwnd, NULL, NULL, RDW_INTERNALPAINT);
            load_directory(directory_hwnd);
            reset_scroll(directory_hwnd);
          }
          break;
        }
      }
      break;
    }
    case WM_DESTROY:{
      EnumChildWindows(hwnd, DestoryChildCallback, 0);
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

void SD_OnVScroll(HWND hwnd, UINT code) {
    const int scrollPos = SD_GetScrollPos(hwnd, SB_VERT, code);

    if(scrollPos == -1)
        return;

    SetScrollPos(hwnd, SB_VERT, scrollPos, TRUE);
    SD_ScrollClient(hwnd, SB_VERT, scrollPos);
}

void SD_ScrollClient(HWND hwnd, int bar, int pos) {
  int cx = 0;
  int cy = 0;

  int& delta = cy;
  int& prev = s_prevy;

  delta = prev - pos;
  prev = pos;

  if(cx || cy) {
    ScrollWindow(hwnd, cx, cy, NULL, NULL);
  }
}

int SD_GetScrollPos(HWND hwnd, int bar, UINT code) {
    SCROLLINFO si = {};
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;
    GetScrollInfo(hwnd, bar, &si);

    const int minPos = si.nMin;
    const int maxPos = si.nMax - (si.nPage - 1);

    int result = -1;

    switch(code) {
    case SB_LINEUP:
        result = max(si.nPos - 1, minPos);
        break;

    case SB_LINEDOWN:
        result = min(si.nPos + 1, maxPos);
        break;

    case SB_PAGEUP:
        result = max(si.nPos - (int)si.nPage, minPos);
        break;

    case SB_PAGEDOWN:
        result = min(si.nPos + (int)si.nPage, maxPos);
        break;

    case SB_THUMBTRACK:
        result = si.nTrackPos;
        break;

    case SB_TOP:
        result = minPos;
        break;

    case SB_BOTTOM:
        result = maxPos;
        break;
    }

    return result;
}

LRESULT CALLBACK directory_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
  SCROLLINFO si;

  switch(msg) {
    case WM_VSCROLL:{
      SD_OnVScroll(hwnd, LOWORD(wParam));
      break;
    }
    case WM_PAINT: {
      SCROLLINFO si = {};
      si.cbSize = sizeof(SCROLLINFO);
      si.fMask = SIF_ALL;
      GetScrollInfo(hwnd, SB_VERT, &si);

      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hwnd, &ps);
      if (!drawn){
        drawn = true;
        if(read_folder)
          draw_title_bar(hdc, si.nPos);
        else{
          print_file(hdc, si.nPos);
        }
      }
      else if (!read_folder){
        print_file(hdc, si.nPos);
      }
      else {
        draw_title_bar(hdc, si.nPos);
      }
      EndPaint(hwnd, &ps);
      ReleaseDC(hwnd, hdc);

      break;
    }
    case WM_DRAWITEM:{
      LPDRAWITEMSTRUCT Item = (LPDRAWITEMSTRUCT)lParam;
      if (fs_type){
        if (fat){
          Entry32* current_directory = fat->get_current_directory();
          if (current_directory){
            Entry32* entry = current_directory->get_entry(Item->CtlID);
            draw_entry_button(Item, entry->get_info(), entry->type());
          }
        }
      }
      else {
        if (ntfs){
          EntryNTFS* current_directory = ntfs->get_current_directory();
          if (current_directory){
            EntryNTFS* entry = current_directory->get_entry(Item->CtlID);
            draw_entry_button(Item, entry->get_info(), entry->type());
          }
        }
      }
      break;
    }
    case WM_COMMAND: {
      drawn = false;
      int id = LOWORD(wParam);
      if (fs_type){
        if(fat){
          Entry32* cd = fat->change_directory(id);
          if (cd)
            read_folder = cd->type();
        }
      }
      else{
        if (ntfs){
          EntryNTFS* cd = ntfs->change_directory(id);
          if (cd)
            read_folder = cd->type();
        }
      }

      EnumChildWindows(hwnd, DestoryChildCallback, 0);
      InvalidateRect(hwnd, NULL, TRUE);
      RedrawWindow(hwnd, NULL, NULL, RDW_INTERNALPAINT);
      InvalidateRect(fs_hwnd, &path_rect, TRUE);
      RedrawWindow(fs_hwnd, &path_rect, NULL, RDW_INTERNALPAINT);


      if (!read_folder){
        load_file();
        nline = file_content.length() / 82;
      }
      else{
        drawn = false;
        load_directory(hwnd);
      }

      reset_scroll(hwnd);
      break;
    }
    case WM_DESTROY:{
      EnumChildWindows(hwnd, DestoryChildCallback, 0);
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
  directory_hwnd = CreateWindowW(wc.lpszClassName, NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL, 220, 50, 720, 630, hwnd, NULL, ghInstance, NULL);
}

void load_directory(HWND hwnd){
  int button_width = 700, button_height = 25, i = 0;

  if (fs_type){
    if (fat){
      Entry32* current_directory = fat->get_current_directory();
      if (current_directory){
        vector<Entry32*> content = current_directory->get_directory();

        for (auto entry : content){
          if (entry->isDeleted){
            if (show_deleted){
              CreateWindowA("Button", "", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 5, 40 + i * (button_height + 15), button_width, button_height, hwnd, (HMENU) i, NULL, NULL);
              ++i;
            }
          }
          else{
            CreateWindowA("Button", "", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 5, 40 + i * (button_height + 15), button_width, button_height, hwnd, (HMENU) i, NULL, NULL);
            ++i;
          }
        }
      }
    }
  }
  else {
    if (ntfs){
      EntryNTFS* current_directory = ntfs->get_current_directory();
      if (current_directory){
        vector<EntryNTFS*> content = current_directory->get_directory();

        for (auto entry : content){
          CreateWindowA("Button", "", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 5, 40 + i * (button_height + 15), button_width, button_height, hwnd, (HMENU) i, NULL, NULL);
          ++i;
        }
      }
    }
  }

  nline = 2 * i;
}

void main_to_fs(HWND hwnd){
  ShowWindow(hwnd, SW_HIDE);
  ShowWindow(fs_hwnd, SW_SHOWNORMAL);
  filesystem_handler();
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

void draw_entry_button(LPDRAWITEMSTRUCT Item, vector<string> info, bool is_folder){
  SelectObject(Item->hDC, CreateFont(16, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial"));
  FillRect(Item->hDC, &Item->rcItem, CreateSolidBrush(0xFFFFFF));
  SelectObject(Item->hDC, CreateSolidBrush(0xFFFFFF));
  SetBkMode(Item->hDC, TRANSPARENT);


  Gdiplus::Graphics graphics(Item->hDC);
  RECT name_rect = {30, 0, 300, 20};
  RECT date_rect = {310, 0, 450, 20};
  RECT type_rect = {460, 0, 580, 20};
  RECT size_rect = {590, 0, 680, 20};

  if (is_folder){
    Gdiplus::Image file_icon(L"assets/folder-icon.png");
    DrawTextA(Item->hDC, info[0].c_str(), info[0].length(), &name_rect, DT_LEFT);
    graphics.DrawImage(&file_icon, 0, 0);
  }
  else{
    Gdiplus::Image folder_icon(L"assets/file-icon.png");
    string name;
  	if (info[0][0] == '$')
      name = info[0];
    else
      name = info[0] + "." + info[2];

    DrawTextA(Item->hDC, name.c_str(), name.length(), &name_rect, DT_LEFT);
    graphics.DrawImage(&folder_icon, 0, 0);
  }

  DrawTextA(Item->hDC, info[1].c_str(), info[1].length(), &date_rect, DT_LEFT);
  DrawTextA(Item->hDC, info[2].c_str(), info[2].length(), &type_rect, DT_LEFT);
  DrawTextA(Item->hDC, info[3].c_str(), info[3].length(), &size_rect, DT_RIGHT);
}

void print_fs_info(HDC hdc){
  Gdiplus::Graphics graphics(hdc);
  Gdiplus::Pen pen(Gdiplus::Color(255, 57, 57, 57));
  Gdiplus::SolidBrush brush(Gdiplus::Color(255, 0, 0, 0));
  Gdiplus::FontFamily fontFamily(L"Arial");
  Gdiplus::Font font(&fontFamily, 14, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);

  Gdiplus::PointF point1(10, 50);
  Gdiplus::PointF point2(230, 15);

  if (fs_type){
    if (fat){
      string info = "FAT32\n\n" + fat->get_bs_info();
      wstringstream wss;
      wss << info.c_str();
      graphics.DrawString(wss.str().c_str(), -1, &font, point1, &brush);
      graphics.DrawRectangle(&pen, 200, 10, 740, 30);

      Entry32* cd = fat->get_current_directory();
      if (cd){
        string path = cd->get_path();
        DrawTextA(hdc, path.c_str(), path.length(), &path_rect, DT_LEFT);
      }
    }
  }
  else {
    if (ntfs){
      string info = "NTFS\n\n" + ntfs->get_pbs_info();
      wstringstream wss;
      wss << info.c_str();
      graphics.DrawString(wss.str().c_str(), -1, &font, point1, &brush);
      graphics.DrawRectangle(&pen, 200, 10, 740, 30);

      EntryNTFS* cd = ntfs->get_current_directory();
      if (cd){
        string path = cd->get_path();
        DrawTextA(hdc, path.c_str(), path.length(), &path_rect, DT_LEFT);
      }
    }
  }
}

void print_file(HDC hdc, int pos){
  Gdiplus::Graphics graphics(hdc);
  Gdiplus::SolidBrush brush(Gdiplus::Color(255, 0, 0, 0));
  Gdiplus::FontFamily fontFamily(L"Consolas");
  Gdiplus::Font font(&fontFamily, 11, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);

  wstringstream wss;
  int len = file_content.length();
  Gdiplus::RectF rect(0, 0 - pos, 700, 650 + pos);
  wss << file_content.c_str();
  graphics.DrawString(wss.str().c_str(), len, &font, rect, NULL, &brush);
}

void draw_title_bar(HDC hdc, int pos) {
  Gdiplus::Graphics graphics(hdc);
  Gdiplus::Pen pen(Gdiplus::Color(255, 57, 57, 57));
  graphics.DrawLine(&pen, 300, -pos, 300, 25-pos);
  graphics.DrawLine(&pen, 450, -pos, 450, 25-pos);
  graphics.DrawLine(&pen, 580, -pos, 580, 25-pos);
  Gdiplus::SolidBrush brush(Gdiplus::Color(255, 0, 0, 0));
  Gdiplus::FontFamily fontFamily(L"Arial");
  Gdiplus::Font font(&fontFamily, 14, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
  Gdiplus::PointF point1(15, 5-pos);
  Gdiplus::PointF point2(315, 5-pos);
  Gdiplus::PointF point3(465, 5-pos);
  Gdiplus::PointF point4(615, 5-pos);

  graphics.DrawString(L"Name", -1, &font, point1, &brush);
  graphics.DrawString(L"Date modified", -1, &font, point2, &brush);
  graphics.DrawString(L"Type", -1, &font, point3, &brush);
  graphics.DrawString(L"Size", -1, &font, point4, &brush);
}
