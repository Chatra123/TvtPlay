/* mod */
/*
TvtPlay.cpp
  30      #include
  481     read ini
  640     write ini
  770     Init
  1044    OpenWithPopup
*/
#pragma once
#include <Windows.h>
#include <Shlwapi.h>
#include <vector>
#include <deque>
#include <regex>
#include <filesystem>


namespace CycPop5
{
  namespace fs = std::experimental::filesystem::v1;
  using namespace std;

  /*
  フォルダ情報　基本クラス
    ページ操作、プロパティの実装
  */
  class FolderInfoBase {
  public:
    bool Enable = true;
  protected:
    int PageNo = 0;
    int PageMax = 0;
    size_t RowSizeMax = 8;//１ページ内のメニュー段数
    wstring Title;
    fs::path Folder;//最後の￥は無し
    vector<vector<fs::path>> PageList;
  public:
    FolderInfoBase(int rowMax) { RowSizeMax = rowMax; }
    virtual ~FolderInfoBase() {}
    virtual bool Init() { return true; };
    virtual bool Init(const wstring folder) { return true; };
    virtual void Update() {};
    void UpdatePageList(vector<wstring> list);
    vector<fs::path> GetPageFile() const { return PageList.empty() ? vector<fs::path>() : PageList[PageNo]; };
    virtual vector<wstring> GetEmptyPage() { return vector<wstring>(); };
    void NextPage();

    fs::path GetFolder() const { return Folder; }
    bool IsEmpty() const { return  PageMax == 0; }
    bool Has2ndPage() const { return 2 <= PageMax; }
    wstring GetPageNo() const { return to_wstring(PageNo + 1) + L"/" + to_wstring(PageMax); }
    wstring GetTitle() const { return Title; }
  };
  //次のページへ
  void FolderInfoBase::NextPage() {
    if (PageMax <= 1)
      return;
    PageNo++;
    PageNo = PageNo < PageMax ? PageNo : 0;
  }
  //ファイルリストをページごとに整理
  //  vector<wstring> list  -->  vector<vector<wstring>> PageList
  void FolderInfoBase::UpdatePageList(vector<wstring> list) {
    PageList.clear();
    //page
    vector<fs::path> page;
    for (wstring one : list) {
      page.push_back(fs::path(one));
      if (RowSizeMax <= page.size()) {
        PageList.push_back(page);
        page.clear();
      }
    }
    if (page.empty() == false)
      PageList.push_back(page);
    //page no
    PageMax = static_cast<int>(ceil(1.0 * list.size() / RowSizeMax));
    PageNo = PageNo < PageMax ? PageNo : PageMax - 1;
    PageNo = 0 < PageNo ? PageNo : 0;
  }


  /*
  フォルダ情報
    フォルダ内のファイルを収集
  */
  class FolderInfo : public FolderInfoBase
  {
  protected:
    wstring Pattern;
  public:
    FolderInfo(int rowMax) : FolderInfoBase(rowMax) { };
    bool Init(const wstring pattern) override;
    void Update() override;
    vector<wstring> GetEmptyPage() override;
  };
  //Init
  bool FolderInfo::Init(const wstring pattern) {
    // パターン形式のみ、フォルダ名だけではダメ
    if (fs::is_directory(pattern)) return false;
    fs::path folder(pattern);
    folder.assign(folder.parent_path());
    if (fs::exists(folder) == false) return false;
    if (fs::is_directory(folder) == false) return false;
    Pattern = pattern;
    Folder = folder;
    Title = Folder.filename().wstring();
    return true;
  }
  //ファイル収集、ページ更新
  void FolderInfo::Update() {
    vector<wstring> fnames;
    HANDLE hFind;
    WIN32_FIND_DATA fd;
    hFind = FindFirstFile(Pattern.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
      do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
          && !(fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) {
          fnames.emplace_back(fd.cFileName);
        }
      } while (FindNextFile(hFind, &fd));
    }
    std::sort(fnames.begin(), fnames.end(),
      [](std::wstring a, std::wstring b) { return ::StrCmpLogicalW(a.c_str(), b.c_str()) < 0; });

    //name --> path
    vector<wstring> paths;
    for (wstring name : fnames) {
      fs::path path(Folder);
      path.append(name);
      paths.push_back(path);
    }
    FolderInfoBase::UpdatePageList(paths);
  }
  //空フォルダの表示を取得
  vector<wstring> FolderInfo::GetEmptyPage() {
    vector<wstring> text;
    text.push_back(Pattern);
    text.push_back(L"    no file  ");
    return text;
  }



  /*
  フォルダ情報
    FolderInfoに「フォルダの変更」機能を追加
  */
  class FolderInfo_CurrentPlay : public FolderInfo
  {
  private:
    const wstring Title_head = L"Current :  ";
  public:
    FolderInfo_CurrentPlay(int rowMax) : FolderInfo(rowMax) { }
    bool Init() override;
    void SetNewFolder(const wstring folder);
  };
  //Init
  bool FolderInfo_CurrentPlay::Init() {
    Title = Title_head;
    return true;
  }
  //SetNewFolder
  void FolderInfo_CurrentPlay::SetNewFolder(const wstring path) {
    fs::path folder(path);
    if (fs::is_directory(folder) == false)
      folder.assign(folder.parent_path());
    if (fs::is_directory(folder) == false || fs::exists(folder) == false)
      return;// not folder path
    //set new
    Folder = folder;
    Pattern = fs::path(folder).append(L"*.ts");
    PageNo = 0;
    Title = Title_head + Folder.filename().wstring();
  }




  /*
  フォルダ情報
    RecentPlay
  */
  class FolderInfo_RecentPlay : public FolderInfo
  {
  private:
    deque<wstring> Recent;
  public:
    FolderInfo_RecentPlay(int rowMax) : FolderInfo(rowMax) { }
    bool Init() override;
    void Update() override;
    void SetNewList(const deque<wstring> list) { Recent = list; }
  };
  //Init
  bool FolderInfo_RecentPlay::Init() {
    Title = L"Recent Play";
    return true;
  }
  //Update
  void FolderInfo_RecentPlay::Update() {
    //deque --> vector
    vector<wstring> v;
    for (wstring one : Recent)
      v.push_back(one);
    FolderInfoBase::UpdatePageList(v);
  }


  /*
  「最近使用したファイル」の保持だけ
  */
  class RecentList
  {
  private:
    //  新しい  front  List[0] ... List[last] back  古い
    deque<wstring> List;
    bool isChanged = false;
  public:
    const size_t Max = 8;
    void push_front(const wstring path);
    deque<wstring> GetList() { return List; }
    bool IsChanged() { return isChanged; }
    void ClearChanged() { isChanged = false; }
  };
  //ファイル追加
  //重複、size()を制限
  void RecentList::push_front(const wstring new_file) {
    if (fs::exists(new_file) == false)
      return;
    //distinct
    auto tail_itr = std::remove_if(List.begin(), List.end()
      , [&](wstring file) { return file == new_file; });
    List.erase(tail_itr, List.end());

    List.push_front(new_file);
    while (Max < List.size())
      List.pop_back();
    isChanged = true;
  }



  //ポップアップメニュー用ＩＤ
  enum CycID {
    Cancel = 0,     //cancel menu
    NextFolder = 1, //next folder
    NextPage = 2,   //next folder page
    FileOffset = 10,//select file      10+
  };

  /*
    ポップアップメニューでファイルを表示
    フォルダ情報の管理、メニュー作成
  */
  class CyclePopMenu
  {
  private:

    int Index = 0;//Folder[Index]
    vector<shared_ptr<FolderInfo>> Folder;

    bool EnableCurrent, EnableRecent;
    shared_ptr<FolderInfo_RecentPlay> Folder_RecentPlay;
    shared_ptr<FolderInfo_CurrentPlay> Folder_CurPlay;
    vector<fs::path> Page;//ポップアップメニューのファイルパスを一時保存
    /*
    MenuRowMax
      ポップアップメニューの段数
      ファイル数の”多い・少ない”でマウスの上下にポップアップ位置が移動しないように
      段数が少ないときは空白で埋める。
    */
    const size_t MenuRowMax = 8;
  public:
    void Init(vector<wstring> pattern, bool enableCurrent, bool enableRecent);
    int NextFolder(int next);
    void NextFolderPage();
    void UpdateFolder(const wstring playing_path, const deque<wstring> recent_play);
    void CreateMenu(HMENU &hmenu, const wstring playing_path);
    wstring GetSelectedFile(size_t id) const;
  };


  //Init
  void CyclePopMenu::Init(vector<wstring> pattern, bool enableCurrent, bool enableRecent) {
    const int PtnMax = 3;
    while (PtnMax < pattern.size())
      pattern.pop_back();

    for (wstring ptn : pattern) {
      auto fi = make_shared<FolderInfo>(MenuRowMax);
      if (fi->Init(ptn)) {
        Folder.emplace_back(fi);
      }
    }

    EnableCurrent = enableCurrent;
    EnableRecent = enableRecent;
    if (EnableCurrent) {
      Folder_CurPlay = make_shared<FolderInfo_CurrentPlay>(MenuRowMax);
      Folder_CurPlay->Init();
      Folder.emplace_back(Folder_CurPlay);
    }
    if (EnableRecent) {
      Folder_RecentPlay = make_shared<FolderInfo_RecentPlay>(MenuRowMax);
      Folder_RecentPlay->Init();
      Folder.emplace_back(Folder_RecentPlay);
    }
  }
  //次のFolderへ
  int CyclePopMenu::NextFolder(int next = -1) {
    if (Folder.empty() == false) {
      for (size_t i = 0; i < Folder.size(); i++) {
        Index = 0 <= next ? next : Index + 1;
        Index = Index < (int)Folder.size() ? Index : 0;
        if (Folder[Index]->Enable)
          break;
      }
    }
    return Index;
  }
  //Folderの次のページへ
  void CyclePopMenu::NextFolderPage() {
    if (Folder.empty() == false)
      Folder[Index]->NextPage();
  }


  //フォルダ情報の更新
  void CyclePopMenu::UpdateFolder(const wstring playing_path, const deque<wstring> recent_list) {
    //CurrentPlay
    if (EnableCurrent) {
      fs::path folder(playing_path);
      if (fs::is_directory(folder) == false) {
        folder.assign(folder.parent_path());
      }
      Folder_CurPlay->SetNewFolder(folder);
      //PopupFolderと同じなら無効に
      Folder_CurPlay->Enable = true;
      for (auto fi : Folder) {
        if (fi == Folder_CurPlay)
          continue;
        if (Folder_CurPlay->GetFolder() == fi->GetFolder())
          Folder_CurPlay->Enable = false;
      }
    }
    //RecentPlay
    if (EnableRecent)
      Folder_RecentPlay->SetNewList(recent_list);
    //FolderInfo
    for (auto fi : Folder)
      fi->Update();
  }


  //ポップアップメニュー作成
  void CyclePopMenu::CreateMenu(HMENU &hmenu, const wstring playing_path) {
    hmenu = ::CreatePopupMenu();
    if (!hmenu) return;
    if (Folder.empty()) {
      ::AppendMenu(hmenu, MF_STRING | MF_GRAYED, CycID::Cancel, TEXT("[ no folder ]"));
      return;
    }

    auto fi = Folder[Index];//folder info
    int rows = 0;
    //先頭のフォルダ名
    wstring title = wstring(L"    [ ") + fi->GetTitle() + L" ]";
    while (title.size() < 60)
      title += L"     ";   //空フォルダ等で横幅が小さくならないようにする
    ::AppendMenu(hmenu, MF_STRING, 1, title.c_str());
    if (fi->IsEmpty()) {
      //空フォルダ
      vector<wstring> page = fi->GetEmptyPage();
      rows += page.size();
      for (wstring line : page)
        ::AppendMenu(hmenu, MF_STRING | MF_GRAYED, CycID::Cancel, line.c_str());
    }
    else {
      //ファイルメニュー
      Page = fi->GetPageFile();
      rows += Page.size();
      for (size_t i = 0; i < Page.size(); i++) {
        bool play_now = playing_path == Page[i].wstring();
        wstring line = Page[i].filename();
        if (60 < line.length())
          line = line.substr(0, 60) + L"...";
        line = regex_replace(line, wregex(L"&"), L"_");  // プレフィクス対策
        MENUITEMINFO mi;
        mi.cbSize = sizeof(MENUITEMINFO);
        mi.fMask = MIIM_ID | MIIM_STATE | MIIM_TYPE;
        mi.wID = i + CycID::FileOffset;
        mi.fState = play_now ? MFS_DEFAULT | MFS_CHECKED : 0;
        mi.fType = MFT_STRING | MFT_RADIOCHECK;
        TCHAR text[64];
        ::lstrcpyn(text, line.c_str(), 64);
        mi.dwTypeData = text;
        ::InsertMenuItem(hmenu, UINT_MAX, TRUE, &mi);
      }
    }
    //空行で埋める
    size_t space = MenuRowMax - rows;
    for (size_t i = 0; i < space; i++)
      ::AppendMenu(hmenu, MF_DISABLED, CycID::Cancel, L"");
    //次ページへ
    if (fi->Has2ndPage()) {
      wstring pageNo = L"        " + fi->GetPageNo() + L"  ...";
      ::AppendMenu(hmenu, MF_STRING, CycID::NextPage, pageNo.c_str());
    }
    else
      ::AppendMenu(hmenu, MF_DISABLED, CycID::Cancel, L"");
  }

  //idからファイルパス取得
  wstring CyclePopMenu::GetSelectedFile(size_t id) const {
    id -= CycID::FileOffset;
    if (Folder.empty() || Page.empty())
      return wstring();
    else if (id < 0 || Page.size() <= id)
      return wstring();

    return Page[id];
  }

}
