/* mod */
/*
TvtPlay.cpp
  30
  770
  1044
*/
#pragma once
#include <Windows.h>
#include <vector>
#include <deque>
#include <regex>
#include <filesystem>
namespace fs = std::experimental::filesystem::v1;
using namespace std;


/*
CyclePopMenu2のフォルダ情報
  - ファイル収集、リストテキスト作成を行う
*/
class PopFolderInfo
{
private:
  int Page = 0;// 0 to N  page index
  int PageCount = 0;// number of page
  int PageSize = 8;//１ページのメニュー最大段数
  wstring Pattern;
  fs::path Folder;//最後の￥は無し
  deque<wstring> Filenames;//１ページのファイル名を一時保存

public:
  bool Init(const wstring pattern);
  void SetPageSize(const int size);
  bool Has2ndPage();
  wstring GetPattern();
  wstring GetFolderName();
  wstring GetPageText();
  vector<wstring> CreatePageText(const wstring playing_path, int &playing_idx);
  bool Next();
  wstring GetFullPath(const size_t id);
};

//Init
bool PopFolderInfo::Init(const wstring pattern) {
  // パターン形式ではなくフォルダ名ならスキップ
  // *.tsは付加しない。
  if (fs::is_directory(pattern)) return false;

  fs::path folder(pattern);
  folder.assign(folder.parent_path());
  if (fs::exists(folder) == false) return false;
  if (fs::is_directory(folder) == false) return false;
  Pattern = pattern;
  Folder = folder;
  return true;
}

//SetPageSize
void PopFolderInfo::SetPageSize(const int size) {
  PageSize = size;
}

//Has2ndPage
bool PopFolderInfo::Has2ndPage() {
  return 2 <= PageCount;
}

//GetPattern
wstring PopFolderInfo::GetPattern() {
  return Pattern;
}

//GetFolderName
wstring PopFolderInfo::GetFolderName() {
  return Folder.filename();
}

//GetPageText
wstring PopFolderInfo::GetPageText() {
  return to_wstring(Page + 1) + L"/" + to_wstring(PageCount);
}



//ファイル収集、メニュー用のテキスト作成
vector<wstring> PopFolderInfo::CreatePageText(const wstring playing_path, int &playing_idx) {

  deque<wstring> list;
  //Get file
  HANDLE hFind;
  WIN32_FIND_DATA fd;
  hFind = FindFirstFile(Pattern.c_str(), &fd);
  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        && !(fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) {
        list.emplace_back(fd.cFileName);
      }
    } while (FindNextFile(hFind, &fd));
  }
  //sort
  std::sort(list.begin(), list.end(),
    [](std::wstring a, std::wstring b) { return ::lstrcmpi(a.c_str(), b.c_str()) < 0; });

  //Page
  //validate Page
  PageCount = static_cast<int>(ceil(1.0 * list.size() / PageSize));
  Page = Page < PageCount ? Page : PageCount - 1;

  //trim to PageSize
  size_t st_idx = Page * PageSize;
  size_t ed_idx = (Page + 1) * PageSize - 1;
  ed_idx = ed_idx < list.size() - 1 ? ed_idx : list.size() - 1;
  size_t size = ed_idx - st_idx + 1;
  while (ed_idx < list.size() - 1)
    list.pop_back();
  while (size < list.size())
    list.pop_front();

  //create text
  vector<wstring> pageText;
  playing_idx = -1;
  for (size_t i = 0; i < list.size(); i++) {
    wstring line = list[i];
    if (60 < line.length())
      line = line.substr(0, 60) + L"...";
    // プレフィクス対策
    line = regex_replace(line, wregex(L"&"), L"_");
    pageText.emplace_back(line);
    //play now?
    fs::path fullpath(Folder.string());
    fullpath.append(list[i]);
    if (fullpath == playing_path)
      playing_idx = i;
  }
  Filenames = deque<wstring>(list);
  return pageText;
}

//次のPageへ
bool PopFolderInfo::Next() {
  if (PageCount <= 1)
    return false;
  Page++;
  Page = Page < PageCount ? Page : 0;
  return true;
}

//フルパス取得
wstring PopFolderInfo::GetFullPath(const size_t id) {
  if (id < 0 || Filenames.size() <= id)
    return wstring();
  fs::path fullpath(Folder);
  fullpath.append(Filenames[id]);
  return wstring(fullpath);
}



/*
Menu ID = 0      cancel menu
        = 1      next folder
        = 2      next folder page
        = 10+    select file
*/
enum CycID {
  Cancel = 0,
  NextFolder = 1,
  NextPage = 2,
  FileOffset = 10,
};

/*
//
//PopupMenu
//  ポップアップメニューでフォルダ内を表示
//  複数フォルダに対応
//
*/
class CyclePopMenu2
{
private:
  int Index = 0;//Folder番号
  vector<PopFolderInfo> Folder;
  /*
  ポップアップメニューの段数
    ファイル数の”多い・少ない”でマウスの上下にポップアップ位置が移動しないように
    段数が少ないときは空白で埋める。
  */
  const size_t MenuRowMax = 8;
  size_t MenuRowCount = 4;

public:
  void Init(vector<wstring> pattern);
  int NextFolder(int next);
  void NextFolderPage();
  void CreateMenu(HMENU &hmenu, const wstring playing_path);
  wstring GetSelectedFile(size_t id);
};


//Init
void CyclePopMenu2::Init(vector<wstring> pattern) {
  const int PtnMax = 3;
  while (PtnMax < pattern.size())
    pattern.pop_back();

  for (wstring ptn : pattern) {
    PopFolderInfo fi;
    if (fi.Init(ptn)) {
      fi.SetPageSize(MenuRowMax);
      Folder.emplace_back(fi);
    }
  }
}

//次のFolderへ
int CyclePopMenu2::NextFolder(int next = -1) {
  if (Folder.empty() == false) {
    Index = 0 <= next ? next : Index + 1;
    Index = Index < (int)Folder.size() ? Index : 0;
  }
  return Index;
}

//Folderの次のページへ
void CyclePopMenu2::NextFolderPage() {
  if (Folder.empty() == false)
    Folder[Index].Next();
}


//
//ポップアップメニュー作成
//
void CyclePopMenu2::CreateMenu(HMENU &hmenu, const wstring playing_path) {
  hmenu = ::CreatePopupMenu();
  if (!hmenu) return;
  if (Folder.empty()) {
    ::AppendMenu(hmenu, MF_STRING | MF_GRAYED, CycID::Cancel, TEXT("[ no folder ]"));
    return;
  }

  // ファイル名取得
  PopFolderInfo *folderMenu = &Folder[Index];
  int playing_idx;
  auto pageText = folderMenu->CreatePageText(playing_path, playing_idx);

  // メニュー生成
  hmenu = ::CreatePopupMenu();
  //先頭のフォルダ名
  wstring label = wstring(L"[ ") + wstring(folderMenu->GetFolderName()) + L" ]";
  ::AppendMenu(hmenu, MF_STRING, 1, label.c_str());
  if (pageText.empty()) {
    ::AppendMenu(hmenu, MF_STRING | MF_GRAYED, CycID::Cancel, folderMenu->GetPattern().c_str());
    ::AppendMenu(hmenu, MF_STRING | MF_GRAYED, CycID::Cancel, TEXT("    no file  "));
  }
  else {
    for (size_t i = 0; i < pageText.size(); i++) {
      bool play_now = playing_idx == (int)i;
      TCHAR text[64];
      ::lstrcpyn(text, pageText[i].c_str(), 64);
      MENUITEMINFO mi;
      mi.cbSize = sizeof(MENUITEMINFO);
      mi.fMask = MIIM_ID | MIIM_STATE | MIIM_TYPE;
      mi.wID = i + CycID::FileOffset;
      mi.fState = play_now ? MFS_DEFAULT | MFS_CHECKED : 0;
      mi.fType = MFT_STRING | MFT_RADIOCHECK;
      mi.dwTypeData = text;
      ::InsertMenuItem(hmenu, UINT_MAX, TRUE, &mi);
    }
  }
  //下の空白部
  MenuRowCount = pageText.size() < MenuRowCount ? MenuRowCount : pageText.size();
  size_t space = pageText.empty()
    ? MenuRowCount - 2 
    : MenuRowCount - pageText.size();
  for (size_t i = 0; i < space; i++)
    ::AppendMenu(hmenu, MF_DISABLED, CycID::Cancel, L"");
  //次ページへ
  if (folderMenu->Has2ndPage()) {
    wstring text = L"    ...  " + folderMenu->GetPageText();
    ::AppendMenu(hmenu, MF_STRING, CycID::NextPage, text.c_str());
  }
  else
    ::AppendMenu(hmenu, MF_DISABLED, CycID::Cancel, L"");
}


//idからフルパス取得
wstring CyclePopMenu2::GetSelectedFile(size_t id) {
  id -= CycID::FileOffset;
  if (id < 0) return wstring();
  if (Folder.empty()) return wstring();

  return Folder[Index].GetFullPath(id);
}




