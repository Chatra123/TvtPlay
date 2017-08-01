/* mod */
/*
TvtPlay.cpp
  30
  770
  1044
*/
#pragma once
#include <Windows.h>
#include <Shlwapi.h>
#include <vector>
#include <deque>
#include <regex>
#include <filesystem>


namespace CycPop2
{
  namespace fs = std::experimental::filesystem::v1;
  using namespace std;

  /*
  CyclePopMenuのフォルダ情報
  - ファイル収集、メニューテキストの作成を行う
  */
  class PopFolderInfo
  {
  private:

    int Page = 0;//cur page index
    int PageMax = 0;
    int RowSize = 8;//１ページ中のメニュー段数
    wstring Pattern;
    fs::path Folder;//最後の￥は無し
    deque<wstring> Filenames;//１ページ内のファイル名を一時保存

  public:
    bool Init(const wstring pattern, const int rowSize);
    bool Has2ndPage() const;
    wstring GetPattern() const;
    wstring GetFolderName() const;
    wstring GetPageCount() const;
    vector<wstring> CreatePageText(const wstring playing_path, int &playing_idx);
    bool Next();
    wstring GetFullPath(const size_t id) const;
  };

  //Init
  bool PopFolderInfo::Init(const wstring pattern, const int rowSize) {
    // パターン形式ではなくフォルダ名ならスキップ
    // *.tsは付加しない。
    if (fs::is_directory(pattern)) return false;

    fs::path folder(pattern);
    folder.assign(folder.parent_path());
    if (fs::exists(folder) == false) return false;
    if (fs::is_directory(folder) == false) return false;
    Pattern = pattern;
    Folder = folder;
    RowSize = rowSize;
    return true;
  }

  //Has2ndPage
  bool PopFolderInfo::Has2ndPage() const {
    return 2 <= PageMax;
  }

  //GetPattern
  wstring PopFolderInfo::GetPattern() const {
    return Pattern;
  }

  //GetFolderName
  wstring PopFolderInfo::GetFolderName() const {
    return Folder.filename();
  }

  //GetPageCount
  wstring PopFolderInfo::GetPageCount() const {
    return to_wstring(Page + 1) + L"/" + to_wstring(PageMax);
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
    std::sort(list.begin(), list.end(),
      [](std::wstring a, std::wstring b) { return ::StrCmpLogicalW(a.c_str(), b.c_str()) < 0; });

    //Page
    //validate Page
    PageMax = static_cast<int>(ceil(1.0 * list.size() / RowSize));
    Page = Page < PageMax ? Page : PageMax - 1;
    Page = 0 < Page ? Page : 0;

    //trim to Page
    size_t st_idx = Page * RowSize;
    size_t ed_idx = (Page + 1) * RowSize - 1;
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
    if (PageMax <= 1)
      return false;
    Page++;
    Page = Page < PageMax ? Page : 0;
    return true;
  }

  //フルパス取得
  wstring PopFolderInfo::GetFullPath(const size_t id) const {
    if (id < 0 || Filenames.size() <= id)
      return wstring();
    fs::path fullpath(Folder);
    fullpath.append(Filenames[id]);
    return wstring(fullpath);
  }



  /*
  CycID = 0      cancel menu
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
  class CyclePopMenu
  {
  private:
    int Index = 0;//Folder番号
    vector<PopFolderInfo> Folder;
    /*
    MenuRowSize
      ポップアップメニューの段数
      ファイル数の”多い・少ない”でマウスの上下にポップアップ位置が移動しないように
      段数が少ないときは空白で埋める。
    */
    const size_t MenuRowMax = 8;
    size_t MenuRowSize = 4;

  public:
    void Init(vector<wstring> pattern);
    int NextFolder(int next);
    void NextFolderPage();
    void CreateMenu(HMENU &hmenu, const wstring playing_path);
    wstring GetSelectedFile(size_t id) const;
  };


  //Init
  void CyclePopMenu::Init(vector<wstring> pattern) {
    const int PtnMax = 3;
    while (PtnMax < pattern.size())
      pattern.pop_back();

    for (wstring ptn : pattern) {
      PopFolderInfo fi;
      if (fi.Init(ptn, MenuRowMax)) {
        Folder.emplace_back(fi);
      }
    }
  }

  //次のFolderへ
  int CyclePopMenu::NextFolder(int next = -1) {
    if (Folder.empty() == false) {
      Index = 0 <= next ? next : Index + 1;
      Index = Index < (int)Folder.size() ? Index : 0;
    }
    return Index;
  }

  //Folderの次のページへ
  void CyclePopMenu::NextFolderPage() {
    if (Folder.empty() == false)
      Folder[Index].Next();
  }


  //
  //ポップアップメニュー作成
  //
  void CyclePopMenu::CreateMenu(HMENU &hmenu, const wstring playing_path) {
    hmenu = ::CreatePopupMenu();
    if (!hmenu) return;
    if (Folder.empty()) {
      ::AppendMenu(hmenu, MF_STRING | MF_GRAYED, CycID::Cancel, TEXT("[ no folder ]"));
      return;
    }

    // ファイル名取得
    PopFolderInfo *fi = &Folder[Index];
    int playing_idx;
    auto pageText = fi->CreatePageText(playing_path, playing_idx);

    // メニュー生成
    hmenu = ::CreatePopupMenu();
    //先頭のフォルダ名
    wstring label = wstring(L"[ ") + wstring(fi->GetFolderName()) + L" ]";
    ::AppendMenu(hmenu, MF_STRING, 1, label.c_str());
    if (pageText.empty()) {
      ::AppendMenu(hmenu, MF_STRING | MF_GRAYED, CycID::Cancel, fi->GetPattern().c_str());
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
    //MenuRowSizeより小さいなら空行で埋める
    MenuRowSize = pageText.size() < MenuRowSize ? MenuRowSize : pageText.size();
    size_t space = pageText.empty() ? MenuRowSize - 2 : MenuRowSize - pageText.size();
    for (size_t i = 0; i < space; i++)
      ::AppendMenu(hmenu, MF_DISABLED, CycID::Cancel, L"");
    //次ページへ
    if (fi->Has2ndPage()) {
      wstring pageNo = L"    ...  " + fi->GetPageCount();
      ::AppendMenu(hmenu, MF_STRING, CycID::NextPage, pageNo.c_str());
    }
    else
      ::AppendMenu(hmenu, MF_DISABLED, CycID::Cancel, L"");
  }


  //idからフルパス取得
  wstring CyclePopMenu::GetSelectedFile(size_t id) const {
    id -= CycID::FileOffset;
    if (id < 0) return wstring();
    if (Folder.empty()) return wstring();

    return Folder[Index].GetFullPath(id);
  }

}
