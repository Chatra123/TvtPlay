/*
						TvtPlay.cpp
	#include
	Init				bool CTvtPlay::InitializePlugin()
	read  ini			void CTvtPlay::LoadSettings()
	write ini			void CTvtPlay::SaveSettings(bool fWriteDefault) const
	OpenWithPopup		bool CTvtPlay::OpenWithPopup(const POINT &pt, UINT flags)
*/
#ifndef INCLUDE_Sn_CyclePopMnu_6_H
#define INCLUDE_Sn_CyclePopMnu_6_H
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
namespace Snow::CycPop6
{
	namespace fs = std::experimental::filesystem::v1;
	using namespace std;

	/*�|�b�v�A�b�v���j���[�p�h�c*/
	enum CycID {
		Cancel = 0,
		SwitchFolder = 1,
		NextPage = 2,
		SelectFile = 10,/* file offset 10+ */
		SelectFolder = 40,
	};

	/*
	�t�H���_���
	�t�@�C���ꗗ���擾���A�|�b�v�A�b�v�p�̃��X�g���쐬
	*/
	class FolderInfo {
	protected:
		fs::path _1stPath;/*���������t�H���_*/
		fs::path searchPath;/*���݂̌����t�H���_*/
		vector<vector<fs::path>> pageList;
		int pageIdx = 0;
		int folderPageIdx;/*�t�H���_�y�[�W�擪*/
		int PageLastIdx() const { return pageList.size() - 1; };
		const wstring ToParent = L"��";/*parent folder�ւ̈ړ��p*/
		/*�|�b�v�A�b�v���j���[�Ƀt�H���_��ǉ�*/
		/*                         Mp4���擾����*/
		bool append_Folder, append_Mp4;
		/*
		�|�b�v�A�b�v���j���[�̒i��
		�t�@�C�����́h�����E���Ȃ��h�Ń}�E�X�̏㉺�Ƀ|�b�v�A�b�v�ʒu���ړ����Ȃ��悤�ɒi�����Œ�
		*/
		const int PageRowMax = 8;/*�P�y�[�W���̃��j���[�i��*/
	public:
		FolderInfo() { }
		virtual ~FolderInfo() { }
		bool Init(const wstring folder);
		void Assign(const wstring path);
		void SetOption(const bool appendFolder, const bool appendMp4);
		bool IsExist() const { return fs::exists(searchPath) || fs::exists(_1stPath); }
		void UpdatePage();
		void NextPage();
		void SelectFolderPage();
		int ValidatePageIndex(int idx);
		wstring GetFolderPath() const { return searchPath; }
		wstring GetFolderName() const {
			fs::path p = searchPath;
			if (p == p.root_path())	return p;
			else if (p.filename() == L".") return p.parent_path().filename();
			else return p.filename();
		}
		void GetPageInfo(vector<wstring> &pageText, vector <int> &pageID) const;
		wstring GetPageNo() const { return to_wstring(pageIdx + 1) + L"/" + to_wstring(PageLastIdx() + 1); }
		fs::path SelectItem(int id);
	};

	/*
	Init
	*/
	bool FolderInfo::Init(const wstring folder)
	{
		Assign(folder);
		_1stPath = searchPath;
		return fs::exists(searchPath);
	};
	/*
	�����t�H���_�ݒ�
	*/
	void FolderInfo::Assign(wstring path)
	{
		/*
		file path --> folder path
		pattern   --> folder path
		*/
		path = path.back() == L'\\' ? path.erase(path.length() - 1) : path;/*������ \ �폜*/
		fs::path p = fs::path(path);
		if (fs::is_directory(p) == false)
			p.assign(p.parent_path());/*������ \ �͖���*/
		if (searchPath != p) {
			searchPath = p;
			pageIdx = 0;
		}
	};
	void FolderInfo::SetOption(const bool show_folder, const bool show_mp4)
	{
		this->append_Folder = show_folder;
		this->append_Mp4 = show_mp4;
	};

	/*
	pageIdx
	*/
	void FolderInfo::NextPage() {
		pageIdx++;
		pageIdx = ValidatePageIndex(pageIdx);
	}
	void FolderInfo::SelectFolderPage() {
		pageIdx = folderPageIdx;
		pageIdx = ValidatePageIndex(pageIdx);
	}
	int FolderInfo::ValidatePageIndex(int idx) {
		idx = 0 < idx ? idx : 0;
		idx = idx <= PageLastIdx() ? idx : 0;
		return idx;
	}
	/*
	�t�@�C�����X�g�X�V
	*/
	void FolderInfo::UpdatePage() {
		/* get file or folder*/
		auto get_file = [](const wstring search, const bool is_file, const bool mp4) {
			vector<fs::path> founds;
			for (fs::path p : fs::directory_iterator(search)) {
				bool isHidden = (GetFileAttributes(p.c_str()) & FILE_ATTRIBUTE_HIDDEN) == FILE_ATTRIBUTE_HIDDEN;
				if (isHidden)
					continue;
				if (is_file && fs::is_regular_file(p)) {
					bool isTs = std::regex_match(p.extension().c_str(), std::wregex(L"\\.ts", std::wregex::icase));
					bool isMp4 = std::regex_match(p.extension().c_str(), std::wregex(L"\\.mp4", std::wregex::icase));
					if (isTs)
						founds.emplace_back(p);
					if (isMp4 && mp4)
						founds.emplace_back(p);
				}
				else if (is_file == false && fs::is_directory(p))
					founds.emplace_back(p);
			}
			std::sort(founds.begin(), founds.end(),
				[](std::wstring a, std::wstring b) { return ::StrCmpLogicalW(a.c_str(), b.c_str()) < 0; });
			return founds;
		};
		/*�P�y�[�W�W�R���Ƃɕ���*/
		/*vector<> files  -->  vector<vector<>> Pages*/
		auto split_to_page = [](const vector<fs::path> &files, const size_t SizeMax) {
			vector<vector<fs::path>> Pages;
			vector<fs::path> page;
			for (auto f : files) {
				page.emplace_back(f);
				if (SizeMax <= page.size()) {
					Pages.emplace_back(page);
					page.clear();
				}
			}
			if (page.empty() == false)
				Pages.emplace_back(page);
			return Pages;
		};

		/*update page*/
		if (fs::exists(searchPath) == false)
			searchPath = _1stPath;
		pageList.clear();
		/*file*/
		{
			vector<fs::path> files = get_file(searchPath, true, append_Mp4);
			auto pages = split_to_page(files, PageRowMax);
			if (pages.empty() == false)
				pageList.insert(pageList.end(), pages.begin(), pages.end());
			else/*��y�[�W�}��*/
				pageList.emplace_back(vector<fs::path>());
		}
		/*folder*/
		if (append_Folder) {
			folderPageIdx = pageList.size();
			vector<fs::path> folders = get_file(searchPath, false, append_Mp4);
			folders.insert(folders.begin(), { ToParent });/*parent folder�ւ̈ړ��p�Bfiles�̐擪�ɑ}��*/
			auto pages = split_to_page(folders, PageRowMax);
			pageList.insert(pageList.end(), pages.begin(), pages.end());
		}
		pageIdx = ValidatePageIndex(pageIdx);
	}

	/*
	���j���[�p�̃e�L�X�g�AID��Ԃ��B
	*/
	void FolderInfo::GetPageInfo(vector<wstring> &pageText, vector<int> &pageID) const {
		int id = 0;
		for (fs::path p : pageList[pageIdx]) {
			wstring name = p.filename();
			if (name == ToParent || fs::is_directory(p))
				name = L"��:  " + name;
			pageText.emplace_back(name);
			pageID.emplace_back(CycID::SelectFile + id);
			id++;
		}
		/*��s�Ŗ��߂�*/
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
		bool is_file = CycID::SelectFile <= id && id < CycID::SelectFolder;
		bool is_folder = CycID::SelectFolder <= id;
		id = is_file ? id - CycID::SelectFile
			: is_folder ? id - CycID::SelectFolder
			: -1;
		if (id < 0)
			return fs::path();

		fs::path p = pageList[pageIdx][id];
		if (p.wstring() == ToParent) {
			if (searchPath != searchPath.root_path())
				return searchPath.parent_path();
			else
				return fs::path();/*root directory now*/
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
		void Init(vector<wstring> paths, bool folderMove, bool mp4);
		int SwitchFolder(int next);
		void UpdateFolder(const wstring playing_path);
		void CreateMenu(HMENU &hmenu, const wstring playing_path);
		wstring SelectMenu(int id, bool &_continue);
		void NextPage();
		void ReadIni();
	};

	void CyclePopMenu::ReadIni() {


	}

	/*
	Init
	*/
	void CyclePopMenu::Init(vector<wstring> paths, bool showFolder = false, bool showMp4 = false) {
		for (wstring path : paths) {
			fs::path p = fs::path(path);
			/*pattern --> folder path
	�@		�݊����̂���pattern�`���Ȃ�t�H���_�p�X�֕ϊ�����*/
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

		showFolder = true;
		showMp4 = true;
		for (auto fi : folder) {
			fi->SetOption(showFolder, showMp4);
		}
	}

	/*
	����Folder��
	*/
	int CyclePopMenu::SwitchFolder(int next = -1) {
		next = 0 <= next ? next : index + 1;
		for (size_t i = 0; i < folder.size(); i++) {
			index = next;
			index = index < (int)folder.size() ? index : 0;
			auto fi = folder[index];
			if (fi->IsExist())
				break;
			next = index + 1;
		}
		return index;
	}

	/*
	�t�H���_���X�V
	*/
	void CyclePopMenu::UpdateFolder(const wstring playing_path) {
		/*CurrentPlay folder
		PopupFolder�Ɠ����Ȃ�CurrentPlay�𖳌���*/
		curPlay->Assign(playing_path);
		bool has_same = false;
		for (auto fi : folder) {
			if (fi == curPlay)
				continue;
			wstring f = fi->GetFolderPath();
			wstring c = curPlay->GetFolderPath();
			if (fi->GetFolderPath() == curPlay->GetFolderPath())
				has_same = true;
		}
		if (has_same && folder[index] == curPlay)
			SwitchFolder();
		folder[index]->UpdatePage();
	}


	/*
	�|�b�v�A�b�v���j���[�쐬
	*/
	void CyclePopMenu::CreateMenu(HMENU &hmenu, const wstring playing_path) {
		if (hmenu)
			::DestroyMenu(hmenu);
		hmenu = ::CreatePopupMenu();
		if (!hmenu)
			return;
		if (folder.empty()) {
			::AppendMenu(hmenu, MF_STRING | MF_GRAYED, CycID::Cancel, TEXT("[ no folder ]"));
			return;
		}
		auto fi = folder[index];

		/*title*/
		wstring title = L"    [ " + fi->GetFolderName() + L" ]  ";
		while (title.size() < 40)
			title += L"    ";
		/*�t�H���_�ԍ�*/
		for (size_t i = 0; i < folder.size(); i++) {
			/*folder���X�g�́uCurrentPlay�t�H���_�v�́�}�[�N*/
			wstring mark = i == folder.size() - 1 ? L"��" : to_wstring(i);
			mark = (int)i == index ? L"[" + mark + L"]" : mark;
			title += mark + L"    ";
		}
		while (title.size() < 80)
			title += L"    ";   /*��t�H���_���ŉ������������Ȃ�Ȃ��悤�ɂ���*/
		::AppendMenu(hmenu, MF_STRING, CycID::SwitchFolder, title.c_str());

		/*menu*/
		vector<wstring> menuText;
		vector<int> menuID;
		fi->GetPageInfo(menuText, menuID);
		for (size_t i = 0; i < menuText.size(); i++) {
			wstring line = menuText[i];
			if (menuID[i] == CycID::Cancel) {
				::AppendMenu(hmenu, MF_DISABLED, CycID::Cancel, L"");
				continue;
			}
			fs::path play(playing_path);
			bool play_now = play.filename() == line
				&& play.parent_path() == fi->GetFolderPath();
			if (60 < line.length())
				line = line.substr(0, 60) + L"...";
			line = regex_replace(line, wregex(L"&"), L"_");/*�v���t�B�N�X�΍�*/

			MENUITEMINFO mi;
			mi.cbSize = sizeof(MENUITEMINFO);
			mi.fMask = MIIM_ID | MIIM_STATE | MIIM_TYPE;
			mi.wID = menuID[i];
			mi.fState = play_now ? MFS_DEFAULT | MFS_CHECKED : 0;
			mi.fType = MFT_STRING | MFT_RADIOCHECK;
			TCHAR text[128];
			::lstrcpyn(text, line.c_str(), line.length() + 1);
			mi.dwTypeData = text;
			::InsertMenuItem(hmenu, UINT_MAX, TRUE, &mi);
		}
		/*page No*/
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
			_continue = false;
			return path;
		}
		else if (fs::is_directory(path)) {
			/*�R�R��UpdatePage()���ăt�H���_�y�[�W�����擾�A�ݒ�*/
			folder[index]->Assign(path);
			folder[index]->UpdatePage();
			folder[index]->SelectFolderPage();
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
		folder[index]->NextPage();
	}






}
#endif // INCLUDE_Sn_CyclePopMnu_6_H
