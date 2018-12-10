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
ファイル選択用ののポップメニュー
*/
namespace CycPop6
{
	namespace fs = std::experimental::filesystem::v1;
	using namespace std;
	const bool Appned_Folder = true;/*ポップアップメニューにフォルダを追加*/
	const bool Append_Mp4 = false;/*ポップアップメニューにMp4を追加*/

	//ポップアップメニュー用ＩＤ
	enum CycID {
		Cancel = 0,
		SwitchFolder = 1,
		NextPage = 2,
		SelectFile = 10,// file offset 10+
		SelectFolder = 20,
	};

	/*
	フォルダ情報
	ファイル一覧を取得し、ポップアップ用のリストを作成
	*/
	class FolderInfo {
	protected:
		fs::path _1stPath;//初期検索フォルダ
		fs::path searchPath;//現在の検索フォルダ
		int pageIdx = 0, pageLastIdx = 0;
		vector<vector<fs::path>> pageList;
		const wstring ToParent = L"..";//parent folderへの移動用
	   /*
	   ポップアップメニューの段数
	   ファイル数の”多い・少ない”でマウスの上下にポップアップ位置が移動しないように段数を固定
	   */
		const int PageRowMax = 8;//１ページ内のメニュー段数
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
	検索フォルダ設定
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
	　ファイルリスト更新
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
		// ページごとに分割し追加
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
		//	files.insert(files.begin(), { ToParent });//parent folderへの移動用。filesの先頭に挿入
		append_to_pageList(files, pageList, PageRowMax);
		//folder
		if (Appned_Folder) {
			vector<fs::path> folders = get_file(searchPath, false, Append_Mp4);
			folders.insert(folders.begin(), { ToParent });//parent folderへの移動用。filesの先頭に挿入
			append_to_pageList(folders, pageList, PageRowMax);
		}
		//validate page no
		pageLastIdx = pageList.size() - 1;
		pageIdx = pageIdx < pageLastIdx ? pageIdx : pageLastIdx;
		pageIdx = 0 <= pageIdx ? pageIdx : 0;
	}

	/*
	メニュー用のテキスト、IDを返す。
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
			name = regex_replace(name, wregex(L"&"), L"_");  // プレフィクス対策
			if (fs::is_directory(p))
				name = L"□:  " + name;
			pageText.emplace_back(name);
			pageID.emplace_back(CycID::SelectFile + id);
			id++;
		}
		//空行で埋める
		size_t space = PageRowMax - pageText.size();
		for (size_t i = 0; i < space; i++) {
			pageText.emplace_back(wstring());
			pageID.emplace_back(CycID::Cancel);
		}
	}
	/*
	選択した項目を返す。
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
	  ポップアップメニューでファイルを表示
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
	　		互換性のためpattern形式ならフォルダパスへ変換する
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
	次のFolderへ
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
	フォルダ情報更新
	*/
	void CyclePopMenu::UpdateFolder(const wstring playing_path) {
		//CurrentPlay folder
		//PopupFolderと同じならCurrentPlayを無効に
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
	ポップアップメニュー作成
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
		for (size_t i = 0; i < folder.size(); i++)/*フォルダ番号*/
		{
			/*folderラストの「CurrentPlayフォルダ」は♪マーク　*/
			wstring mark = i == folder.size() - 1 ? L"♪" : to_wstring(i);
			mark = (int)i == index ? L"[" + mark + L"]" : mark;
			title += mark + L"    ";
		}
		while (title.size() < 80)
			title += L"    ";   //空フォルダ等で横幅が小さくならないようにする
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
		/*デスクトップ下部ではポップアップ位置がボタンと重なるので空行追加*/
		::AppendMenu(hmenu, MF_STRING | MF_GRAYED, CycID::Cancel, L"");
	}

	/*
	メニュー項目の選択
	  - ファイル選択  -->  return path
	  - フォルダ選択  -->  検索フォルダ変更
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
	Folderの次のページへ
	*/
	void CyclePopMenu::NextPage() {
		if (folder.empty() == false)
			folder[index]->NextPage();
	}






}
#endif // INCLUDE_CYCLEPOP_MENU_6_H
