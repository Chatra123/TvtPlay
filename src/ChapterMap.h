#ifndef INCLUDE_CHAPTER_MAP_H
#define INCLUDE_CHAPTER_MAP_H
class CChapterMap
{
	static const int RETRY_LIMIT = 3;
public:
	static const int CHAPTER_POS_MAX = 99 * 3600000 + 59 * 60000 + 59 * 1000 + 999;
	struct CHAPTER {
	public:
		// 最終要素のみにNULを必ず格納する
		std::vector<TCHAR> name;
		CHAPTER(LPCTSTR name_ = TEXT("")) : name(name_, name_ + ::lstrlen(name_) + 1) {}
		bool IsIn() const { return !::ChrCmpI(name[0], TEXT('i')); }
		bool IsOut() const { return !::ChrCmpI(name[0], TEXT('o')); }
		bool IsX() const { return !::ChrCmpI(name[IsIn() || IsOut() ? 1 : 0], TEXT('x')); }
		void SetIn(bool f) { if (f && !IsIn()) name.insert(name.begin(), TEXT('i')); else if (!f && IsIn()) name.erase(name.begin()); }
		void SetOut(bool f) { if (f && !IsOut()) name.insert(name.begin(), TEXT('o')); else if (!f && IsOut()) name.erase(name.begin()); }
		void SetX(bool f) {
			if (f && !IsX()) name.insert(name.begin() + (IsIn() || IsOut() ? 1 : 0), TEXT('x'));
			else if (!f && IsX()) name.erase(name.begin() + (IsIn() || IsOut() ? 1 : 0));
		}
		/*Snow*/
		static constexpr const wchar_t *IX = L"ix", *OX = L"ox";
		bool IsSkipBegin() const { return !::ChrCmpI(name[0], TEXT('i')) && !::ChrCmpI(name[1], TEXT('x')); }
		bool IsSkipEnd() const { return !::ChrCmpI(name[0], TEXT('o')) && !::ChrCmpI(name[1], TEXT('x')); }
		void SetSkipBegin(bool f) { SetOut(false); SetIn(f);  SetX(f); }
		void SetSkipEnd(bool f) { SetIn(false); SetOut(f); SetX(f); }
		void SetNormal() { SetIn(false); SetOut(false); SetX(false); }
	};

	CChapterMap();
	~CChapterMap();
	bool Open(LPCTSTR path, LPCTSTR subDirName);
	void Close();
	bool Sync();
	bool Insert(std::pair<int, CHAPTER>& ch, int erase_pos = -1);
	bool Insert_obs(const std::pair<int, CHAPTER> &ch, int pos = -1);
	bool Move(int from, int to);
	bool Erase(int pos);
	void ShiftAll(int offset);
	const std::map<int, CHAPTER>& Get() const { return m_map; }
	bool IsOpen() const { return m_path[0] != 0; }
	bool NeedToSync() const { return m_hDir != INVALID_HANDLE_VALUE; }
	static int Clamp(int pos);
	static void Clamp(std::pair<int, CHAPTER> &ch);

private:
	bool Save() const;
	bool InsertCommand(LPCTSTR p);
	bool InsertOgmStyleCommand(LPCTSTR p);
	bool InsertFrameStyleCommand(LPCTSTR p);

	std::map<int, CHAPTER> m_map;
	TCHAR m_path[MAX_PATH];
	HANDLE m_hDir, m_hEvent;
	bool m_fWritable;
	int m_retryCount;
	OVERLAPPED m_ol;
	BYTE m_buf[2048];
};

#endif // INCLUDE_CHAPTER_MAP_H
