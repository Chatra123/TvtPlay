/* mod */
/*
TvtPlay.cpp
  30      #include
  481     read ini
  640     write ini
  770     Init
  1044    OpenWithPopup
*/
#ifndef INCLUDE_CYCLEPOP_MENU_6_H
#define INCLUDE_CYCLEPOP_MENU_6_H
#pragma once
#include <Windows.h>
#include <Shlwapi.h>
#include <vector>
#include <deque>
#include <regex>
#include <filesystem>

/*
�t�@�C���I��p�̂̃|�b�v���j���[
*/
namespace CycPop6
{
	namespace fs = std::experimental::filesystem::v1;
	using namespace std;
	const bool Appned_Folder = true;/*�|�b�v�A�b�v���j���[�Ƀt�H���_��ǉ�*/
	const bool Append_Mp4 = false;/*�|�b�v�A�b�v���j���[��Mp4��ǉ�*/

	//�|�b�v�A�b�v���j���[�p�h�c
	enum CycID {
		Cancel = 0,
		SwitchFolder = 1,
		NextPage = 2,
		SelectFile = 10,// file offset 10+
		SelectFolder = 20,
	};

	/*
	�t�H���_���
	�t�@�C���ꗗ���擾���A�|�b�v�A�b�v�p�̃��X�g���쐬
	*/
	class FolderInfo {
	protected:
		fs::path _1stPath;//���������t�H���_
		fs::path searchPath;//���݂̌����t�H���_
		int pageIdx = 0, pageLastIdx = 0;
		vector<vector<fs::path>> pageList;
		const wstring ToParent = L"..";//parent folder�ւ̈ړ��p
	   /*
	   �|�b�v�A�b�v���j���[�̒i��
	   �t�@�C�����́h�����E���Ȃ��h�Ń}�E�X�̏㉺�Ƀ|�b�v�A�b�v�ʒu���ړ����Ȃ��悤�ɒi�����Œ�
	   */
		const int PageRowMax = 8;//�P�y�[�W���̃��j���[�i��
	public:
		FolderInfo() { }
		virtual ~FolderInfo() { }
		bool Init(const wstring folder = wstring());
		void AssignFolder(wstring path);
		bool IsExist() const { return fs::exists(searchPath) || fs::exists(_1stPath); }
		void UpdatePageList();
		void GetPageInfo(vector<wstring> &pageText, vector <int> &pageID) const;
		void NextPage();
		wstring GetFolderPath() const { return searchPath; }
		wstring GetFolderName() const {
			fs::path p = searchPath;
			return p == p.root_path() ? p : p.filename();
		}
		wstring GetPageNo() const { return to_wstring(pageIdx + 1) + L"/" + to_wstring(pageLastIdx + 1); }
		fs::path SelectItem(int id);
	};

	/*
	Init
	*/
	bool FolderInfo::Init(wstring folder)
	{
		AssignFolder(folder);
		_1stPath = searchPath;
		return fs::exists(searchPath);
	};

	/*
	�����t�H���_�ݒ�
	*/
	void FolderInfo::AssignFolder(wstring folder)
	{
		/*
		file path --> folder path
		pattern   --> folder path
		*/
		fs::path p = fs::path(folder);
		if (fs::is_directory(p) == false)
			p.assign(p.parent_path());
		searchPath = fs::path(p);
		pageIdx = 0;
	};

	/*
	  pageIdx++
	*/
	void FolderInfo::NextPage() {
		if (pageLastIdx == 0)
			return;
		pageIdx++;
		pageIdx = pageIdx <= pageLastIdx ? pageIdx : 0;
	}

	/*
	�@�t�@�C�����X�g�X�V
	*/
	void FolderInfo::UpdatePageList() {
		// get file
		auto get_file = [](wstring search, bool is_file, bool append_mp4) {
			vector<fs::path> founds;
			for (auto& itr : fs::directory_iterator(search)) {
				fs::path p = itr.path();
				bool isHidden = (GetFileAttributes(p.c_str()) & FILE_ATTRIBUTE_HIDDEN) == FILE_ATTRIBUTE_HIDDEN;
				if (isHidden)
					continue;
				if (is_file && fs::is_regular_file(p)) {
					bool isTs = std::regex_match(p.extension().c_str(), std::wregex(L"\\.ts", std::wregex::icase));
					bool isMp4 = std::regex_match(p.extension().c_str(), std::wregex(L"\\.mp4", std::wregex::icase));
					if (isTs)
						founds.emplace_back(p);
					if (isMp4 && append_mp4)
						founds.emplace_back(p);
				}
				else if (is_file == false && fs::is_directory(p))
					founds.emplace_back(p);
			}
			std::sort(founds.begin(), founds.end(),
				[](std::wstring a, std::wstring b) { return ::StrCmpLogicalW(a.c_str(), b.c_str()) < 0; });
			return founds;
		};
		// �y�[�W���Ƃɕ������ǉ�
		// vector<> list  -->  vector<vector<>> pageS
		auto append_to_pageList = [](vector<fs::path> list, vector<vector<fs::path>> &pageS, size_t RowMax) {
			vector<fs::path> page;
			for (auto p : list) {
				page.push_back(p);
				if (RowMax <= page.size()) {
					pageS.emplace_back(page);
					page.clear();
				}
			}
			if (page.empty() == false)
				pageS.emplace_back(page);
			return pageS;
		};

		//update page
		if (fs::exists(searchPath) == false)
			searchPath = _1stPath;
		pageList.clear();
		//file
		vector<fs::path> files = get_file(searchPath, true, Append_Mp4);
		//if (Appned_Folder)
		//	files.insert(files.begin(), { ToParent });//parent folder�ւ̈ړ��p�Bfiles�̐擪�ɑ}��
		append_to_pageList(files, pageList, PageRowMax);
		//folder
		if (Appned_Folder) {
			vector<fs::path> folders = get_file(searchPath, false, Append_Mp4);
			folders.insert(folders.begin(), { ToParent });//parent folder�ւ̈ړ��p�Bfiles�̐擪�ɑ}��
			append_to_pageList(folders, pageList, PageRowMax);
		}
		//validate page no
		pageLastIdx = pageList.size() - 1;
		pageIdx = pageIdx < pageLastIdx ? pageIdx : pageLastIdx;
		pageIdx = 0 <= pageIdx ? pageIdx : 0;
	}

	/*
	���j���[�p�̃e�L�X�g�AID��Ԃ��B
	*/
	void FolderInfo::GetPageInfo(vector<wstring> &pageText, vector<int> &pageID) const {
		int id = 0;
		for (fs::path p : pageList[pageIdx]) {
			if (p.empty()) {
				pageText.emplace_back(p.filename());
				pageID.emplace_back(CycID::Cancel);
				id++;
				continue;
			}
			wstring name = p.filename();
			if (60 < name.length())
				name = name.substr(0, 60) + L"...";
			name = regex_replace(name, wregex(L"&"), L"_");  // �v���t�B�N�X�΍�
			if (fs::is_directory(p))
				name = L"��:  " + name;
			pageText.emplace_back(name);
			pageID.emplace_back(CycID::SelectFile + id);
			id++;
		}
		//��s�Ŗ��߂�
		size_t space = PageRowMax - pageText.size();
		for (size_t i = 0; i < space; i++) {
			pageText.emplace_back(wstring());
			pageID.emplace_back(CycID::Cancel);
		}
	}
	/*
	�I���������ڂ�Ԃ��B
	*/
	fs::path FolderInfo::SelectItem(int id)
	{
		bool select_file = CycID::SelectFile <= id && id < CycID::SelectFolder;
		bool select_folder = CycID::SelectFolder <= id;
		id = select_file ? id - CycID::SelectFile :
			select_folder ? id - CycID::SelectFolder
			: -1;
		if (id < 0)
			return fs::path();

		fs::path p = pageList[pageIdx][id];
		if (p.wstring() == ToParent) {
			if (searchPath != searchPath.root_path())
				return searchPath.parent_path();
			else
				return fs::path();
		}
		else
			return p;
	}


	/*
	  �|�b�v�A�b�v���j���[�Ńt�@�C����\��
	*/
	class CyclePopMenu
	{
	private:
		int index = 0;
		vector<shared_ptr<FolderInfo>> folder;
		shared_ptr<FolderInfo> curPlay;
	public:
		void Init(vector<wstring> pattern);
		int SwitchFolder(int next);
		void UpdateFolder(const wstring playing_path);
		void CreateMenu(HMENU &hmenu, const wstring playing_path);
		wstring SelectMenu(int id, bool &_continue);
		void NextPage();
	};

	/*
	Init
	*/
	void CyclePopMenu::Init(vector<wstring> paths) {
		for (wstring path : paths) {
			fs::path p = fs::path(path);
			/*
			pattern --> folder path
	�@		�݊����̂���pattern�`���Ȃ�t�H���_�p�X�֕ϊ�����
			*/
			if (path.find(L'*') != std::string::npos)
				p.assign(p.parent_path());
			if (fs::is_directory(p) == false)
				continue;
			auto fi = make_shared<FolderInfo>();
			if (fi->Init(p))
				folder.emplace_back(fi);
		}
		curPlay = make_shared<FolderInfo>();
		folder.emplace_back(curPlay);
	}

	/*
	����Folder��
	*/
	int CyclePopMenu::SwitchFolder(int next = -1) {
		if (folder.empty())
			return index;

		next = 0 <= next ? next : index + 1;
		for (size_t i = 0; i < folder.size(); i++) {
			index = next;
			index = index < (int)folder.size() ? index : 0;
			auto fi = folder[index];
			if (fi->IsExist()) {
				fi->UpdatePageList();
				break;
			}
			next = index + 1;
		}
		return index;
	}

	/*
	�t�H���_���X�V
	*/
	void CyclePopMenu::UpdateFolder(const wstring playing_path) {
		//CurrentPlay folder
		//PopupFolder�Ɠ����Ȃ�CurrentPlay�𖳌���
		curPlay->AssignFolder(wstring());
		bool is_same = false;
		for (auto fi : folder) {
			if (fi->GetFolderPath() == fs::path(playing_path).parent_path())
				is_same = true;
		}
		if (is_same) {
			if (folder[index] == curPlay)
				SwitchFolder();
		}
		else
			curPlay->AssignFolder(playing_path);

		folder[index]->UpdatePageList();
	}


	/*
	�|�b�v�A�b�v���j���[�쐬
	*/
	void CyclePopMenu::CreateMenu(HMENU &hmenu, const wstring playing_path) {
		hmenu = ::CreatePopupMenu();
		if (!hmenu)
			return;
		if (folder.empty()) {
			::AppendMenu(hmenu, MF_STRING | MF_GRAYED, CycID::Cancel, TEXT("[ no folder ]"));
			return;
		}
		auto fi = folder[index];

		//title
		wstring title = L"    [ " + fi->GetFolderName() + L" ]  ";
		while (title.size() < 40)
			title += L"    ";
		for (size_t i = 0; i < folder.size(); i++)/*�t�H���_�ԍ�*/
		{
			/*folder���X�g�́uCurrentPlay�t�H���_�v�́�}�[�N�@*/
			wstring mark = i == folder.size() - 1 ? L"��" : to_wstring(i);
			mark = (int)i == index ? L"[" + mark + L"]" : mark;
			title += mark + L"    ";
		}
		while (title.size() < 80)
			title += L"    ";   //��t�H���_���ŉ������������Ȃ�Ȃ��悤�ɂ���
		::AppendMenu(hmenu, MF_STRING, CycID::SwitchFolder, title.c_str());

		//file menu
		vector<wstring> menuText; vector<int> menuID;
		fi->GetPageInfo(menuText, menuID);
		for (size_t i = 0; i < menuText.size(); i++) {
			if (menuID[i] == CycID::Cancel) {
				::AppendMenu(hmenu, MF_DISABLED, CycID::Cancel, L"");
				continue;
			}
			fs::path play(playing_path);
			bool play_now = play.filename() == menuText[i]
				&& play.parent_path() == fi->GetFolderPath();
			MENUITEMINFO mi;
			mi.cbSize = sizeof(MENUITEMINFO);
			mi.fMask = MIIM_ID | MIIM_STATE | MIIM_TYPE;
			mi.wID = menuID[i];
			mi.fState = play_now ? MFS_DEFAULT | MFS_CHECKED : 0;
			mi.fType = MFT_STRING | MFT_RADIOCHECK;
			TCHAR text[128];
			::lstrcpyn(text, menuText[i].c_str(), menuText[i].length() + 1);
			mi.dwTypeData = text;
			::InsertMenuItem(hmenu, UINT_MAX, TRUE, &mi);
		}
		//pageNo
		wstring pageNo = L"     " + fi->GetPageNo();
		::AppendMenu(hmenu, MF_STRING, CycID::NextPage, pageNo.c_str());
		/*�f�X�N�g�b�v�����ł̓|�b�v�A�b�v�ʒu���{�^���Əd�Ȃ�̂ŋ�s�ǉ�*/
		::AppendMenu(hmenu, MF_STRING | MF_GRAYED, CycID::Cancel, L"");
	}

	/*
	���j���[���ڂ̑I��
	  - �t�@�C���I��  -->  return path
	  - �t�H���_�I��  -->  �����t�H���_�ύX
	*/
	wstring CyclePopMenu::SelectMenu(int id, bool &_continue) {
		fs::path path = folder[index]->SelectItem(id);
		if (fs::is_regular_file(path)) {
			return path;
		}
		else if (fs::is_directory(path)) {
			folder[index]->AssignFolder(path);
			folder[index]->UpdatePageList();
			_continue = true;
			return wstring();
		}
		_continue = true;
		return wstring();
	}

	/*
	Folder�̎��̃y�[�W��
	*/
	void CyclePopMenu::NextPage() {
		if (folder.empty() == false)
			folder[index]->NextPage();
	}






}
#endif // INCLUDE_CYCLEPOP_MENU_6_H
