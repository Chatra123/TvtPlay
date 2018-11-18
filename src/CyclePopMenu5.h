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
  �t�H���_���@��{�N���X
    �y�[�W����A�v���p�e�B�̎���
  */
  class FolderInfoBase {
  public:
    bool Enable = true;
  protected:
    int PageNo = 0;
    int PageMax = 0;
    size_t RowSizeMax = 8;//�P�y�[�W���̃��j���[�i��
    wstring Title;
    fs::path Folder;//�Ō�́��͖���
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
  //���̃y�[�W��
  void FolderInfoBase::NextPage() {
    if (PageMax <= 1)
      return;
    PageNo++;
    PageNo = PageNo < PageMax ? PageNo : 0;
  }
  //�t�@�C�����X�g���y�[�W���Ƃɐ���
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
  �t�H���_���
    �t�H���_���̃t�@�C�������W
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
    // �p�^�[���`���̂݁A�t�H���_�������ł̓_��
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
  //�t�@�C�����W�A�y�[�W�X�V
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
  //��t�H���_�̕\�����擾
  vector<wstring> FolderInfo::GetEmptyPage() {
    vector<wstring> text;
    text.push_back(Pattern);
    text.push_back(L"    no file  ");
    return text;
  }



  /*
  �t�H���_���
    FolderInfo�Ɂu�t�H���_�̕ύX�v�@�\��ǉ�
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
  �t�H���_���
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
  �u�ŋߎg�p�����t�@�C���v�̕ێ�����
  */
  class RecentList
  {
  private:
    //  �V����  front  List[0] ... List[last] back  �Â�
    deque<wstring> List;
    bool isChanged = false;
  public:
    const size_t Max = 8;
    void push_front(const wstring path);
    deque<wstring> GetList() { return List; }
    bool IsChanged() { return isChanged; }
    void ClearChanged() { isChanged = false; }
  };
  //�t�@�C���ǉ�
  //�d���Asize()�𐧌�
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



  //�|�b�v�A�b�v���j���[�p�h�c
  enum CycID {
    Cancel = 0,     //cancel menu
    NextFolder = 1, //next folder
    NextPage = 2,   //next folder page
    FileOffset = 10,//select file      10+
  };

  /*
    �|�b�v�A�b�v���j���[�Ńt�@�C����\��
    �t�H���_���̊Ǘ��A���j���[�쐬
  */
  class CyclePopMenu
  {
  private:

    int Index = 0;//Folder[Index]
    vector<shared_ptr<FolderInfo>> Folder;

    bool EnableCurrent, EnableRecent;
    shared_ptr<FolderInfo_RecentPlay> Folder_RecentPlay;
    shared_ptr<FolderInfo_CurrentPlay> Folder_CurPlay;
    vector<fs::path> Page;//�|�b�v�A�b�v���j���[�̃t�@�C���p�X���ꎞ�ۑ�
    /*
    MenuRowMax
      �|�b�v�A�b�v���j���[�̒i��
      �t�@�C�����́h�����E���Ȃ��h�Ń}�E�X�̏㉺�Ƀ|�b�v�A�b�v�ʒu���ړ����Ȃ��悤��
      �i�������Ȃ��Ƃ��͋󔒂Ŗ��߂�B
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
  //����Folder��
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
  //Folder�̎��̃y�[�W��
  void CyclePopMenu::NextFolderPage() {
    if (Folder.empty() == false)
      Folder[Index]->NextPage();
  }


  //�t�H���_���̍X�V
  void CyclePopMenu::UpdateFolder(const wstring playing_path, const deque<wstring> recent_list) {
    //CurrentPlay
    if (EnableCurrent) {
      fs::path folder(playing_path);
      if (fs::is_directory(folder) == false) {
        folder.assign(folder.parent_path());
      }
      Folder_CurPlay->SetNewFolder(folder);
      //PopupFolder�Ɠ����Ȃ疳����
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


  //�|�b�v�A�b�v���j���[�쐬
  void CyclePopMenu::CreateMenu(HMENU &hmenu, const wstring playing_path) {
    hmenu = ::CreatePopupMenu();
    if (!hmenu) return;
    if (Folder.empty()) {
      ::AppendMenu(hmenu, MF_STRING | MF_GRAYED, CycID::Cancel, TEXT("[ no folder ]"));
      return;
    }

    auto fi = Folder[Index];//folder info
    int rows = 0;
    //�擪�̃t�H���_��
    wstring title = wstring(L"    [ ") + fi->GetTitle() + L" ]";
    while (title.size() < 60)
      title += L"     ";   //��t�H���_���ŉ������������Ȃ�Ȃ��悤�ɂ���
    ::AppendMenu(hmenu, MF_STRING, 1, title.c_str());
    if (fi->IsEmpty()) {
      //��t�H���_
      vector<wstring> page = fi->GetEmptyPage();
      rows += page.size();
      for (wstring line : page)
        ::AppendMenu(hmenu, MF_STRING | MF_GRAYED, CycID::Cancel, line.c_str());
    }
    else {
      //�t�@�C�����j���[
      Page = fi->GetPageFile();
      rows += Page.size();
      for (size_t i = 0; i < Page.size(); i++) {
        bool play_now = playing_path == Page[i].wstring();
        wstring line = Page[i].filename();
        if (60 < line.length())
          line = line.substr(0, 60) + L"...";
        line = regex_replace(line, wregex(L"&"), L"_");  // �v���t�B�N�X�΍�
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
    //��s�Ŗ��߂�
    size_t space = MenuRowMax - rows;
    for (size_t i = 0; i < space; i++)
      ::AppendMenu(hmenu, MF_DISABLED, CycID::Cancel, L"");
    //���y�[�W��
    if (fi->Has2ndPage()) {
      wstring pageNo = L"        " + fi->GetPageNo() + L"  ...";
      ::AppendMenu(hmenu, MF_STRING, CycID::NextPage, pageNo.c_str());
    }
    else
      ::AppendMenu(hmenu, MF_DISABLED, CycID::Cancel, L"");
  }

  //id����t�@�C���p�X�擾
  wstring CyclePopMenu::GetSelectedFile(size_t id) const {
    id -= CycID::FileOffset;
    if (Folder.empty() || Page.empty())
      return wstring();
    else if (id < 0 || Page.size() <= id)
      return wstring();

    return Page[id];
  }

}
