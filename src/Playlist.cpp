#include <Windows.h>
#include <Shlwapi.h>
#include <algorithm>
#include <vector>
#include "Util.h"
#include "Playlist.h"

CPlaylist::CPlaylist()
    : m_pos(0)
{
}

// プレイリストファイルまたはTSファイルを再生リストに加える
// 成功: 加えられた位置, 失敗: 負
int CPlaylist::PushBackListOrFile(LPCTSTR path, bool fMovePos)
{
    // カレントからの絶対パスに変換
    TCHAR fullPath[MAX_PATH];
    DWORD rv = ::GetFullPathName(path, _countof(fullPath), fullPath, NULL);
    if (rv == 0 || rv >= MAX_PATH) return -1;

    int pos = -1;
    if (IsPlayListFile(path)) {
        // プレイリストファイルとして処理
        pos = PushBackList(fullPath);
    }
    else {
        // TSファイルとして処理
        PLAY_INFO pi;
        ::lstrcpyn(pi.path, fullPath, _countof(pi.path));
        pos = static_cast<int>(m_list.size());
        m_list.push_back(pi);
    }
    if (fMovePos && pos >= 0) m_pos = pos;
    return pos;
}


// プレイリストファイルを再生リストに加える
int CPlaylist::PushBackList(LPCTSTR fullPath)
{
    int pos = -1;
    std::vector<WCHAR> ret = ReadUtfFileToEnd(fullPath, FILE_SHARE_READ);
    if (!ret.empty()) {
        // 相対パスとの結合用
        TCHAR dirName[MAX_PATH];
        ::lstrcpyn(dirName, fullPath, _countof(dirName));
        ::PathRemoveFileSpec(dirName);

        for (TCHAR *p = &ret.front(); *p;) {
            // 1行取得してpを進める
            TCHAR line[512];
            int len = ::StrCSpn(p, TEXT("\r\n"));
            ::lstrcpyn(line, p, min(len+1, _countof(line)));
            p += len;
            if (*p == TEXT('\r') && *(p+1) == TEXT('\n')) ++p;
            if (*p) ++p;

            // 左右の空白文字を取り除く
            ::StrTrim(line, TEXT(" \t"));
            if (line[0] != TEXT('#')) {
                // 左右に"の対応があれば取り除く
                if (line[0] == TEXT('"') && line[::lstrlen(line)-1] == TEXT('"')) ::StrTrim(line, TEXT("\""));
                // スラッシュ→バックスラッシュ
                for (TCHAR *q=line; *q; ++q) if (*q==TEXT('/')) *q=TEXT('\\');
                if (line[0]) {
                    PLAY_INFO pi;
                    if (::PathIsRelative(line)) {
                        // 相対パス
                        if (!::PathCombine(pi.path, dirName, line)) pi.path[0] = 0;
                    }
                    else {
                        // 絶対パス
                        ::lstrcpyn(pi.path, line, _countof(pi.path));
                    }
                    if (::PathFileExists(pi.path)) {
                        if (pos < 0) pos = static_cast<int>(m_list.size());
                        m_list.push_back(pi);
                    }
                }
            }
        }
    }
    return pos;
}








//mod
#include <TCHAR.H>//::_tcsicmp(...);
#include <string>
#include <deque>
#include <regex>
/*
//
// AutoPlay
//   フォルダ内のファイルをプレイリストに追加
//   利用することが無いのでOpenDialogからは呼んでいない。
//
*/
int CPlaylist::PushBackListOrFile_AutoPlay(LPCTSTR path, bool fMovePos, bool fAutoCorrect)
{
  // カレントからの絶対パスに変換
  TCHAR fullpath[MAX_PATH];
  DWORD rv = ::GetFullPathName(path, _countof(fullpath), fullpath, NULL);
  if (rv == 0 || rv >= MAX_PATH)
    return -1;
  bool isPattern = _tcsstr(fullpath, TEXT("*")) != NULL;
  if (PathFileExists(fullpath) == false && isPattern == false)
    return -1;  // not found

  const size_t ListMax = 3;//-1で制限しない
  int pos = -1;
  if (isPattern && fAutoCorrect) {
    //pattern
    pos = PushBackCollectedFiles(fullpath, ListMax);
  }
  else if (IsMediaFile(fullpath)) {
    //file path
    if (fAutoCorrect)
      pos = PushBackCollectedFiles(fullpath, ListMax);
    else // add 1 file
      pos = PushBackListOrFile(fullpath, fMovePos);
  }
  else if (IsPlayListFile(path)) {
    //tslist
    pos = PushBackListOrFile(fullpath, fMovePos);
  }
  else if(PathIsDirectory(fullpath) && fAutoCorrect){
    //directory
    pos = PushBackCollectedFiles(fullpath, ListMax);
  }

  if (fMovePos && pos >= 0) m_pos = pos;
  return pos;
}


//
//ファイルを収集しプレイリストに追加
//
int CPlaylist::PushBackCollectedFiles(const LPCTSTR path, const size_t ListMax)
{
  std::vector<std::wstring> list;
  int pos = CollectFiles(list , path, ListMax);
  if (list.empty()) return -1;

  ClearWithoutCurrent();
  EraseCurrent();
  for (std::wstring tspath : list) {
    PLAY_INFO pi;
    ::lstrcpyn(pi.path, tspath.c_str(), _countof(pi.path));
    m_list.emplace_back(pi);
  }
  return pos;
}


//
//ファイルを収集
//
int CPlaylist::CollectFiles(std::vector<std::wstring> &list, const LPCTSTR path, const size_t ListMax)
{
  /*
  file path 
  directory path   -->  pattern
  pattern 
  */
  TCHAR dir[MAX_PATH] = {};
  TCHAR name[MAX_PATH] = {};
  TCHAR pattern[MAX_PATH] = {};
  if (_tcsstr(path, TEXT("*")) != NULL) {
    //is pattern
    ::lstrcpyn(dir, path, _countof(dir));
    ::PathRemoveFileSpec(dir);
    ::lstrcpyn(pattern, path, _countof(pattern));
  }
  else if (PathFileExists(path) && !PathIsDirectory(path)) {
    //is file path
    ::lstrcpyn(dir, path, _countof(dir));
    ::PathRemoveFileSpec(dir);
    ::PathAddBackslash(dir);
    ::lstrcpyn(name, PathFindFileName(path), _countof(name));
    ::lstrcpyn(pattern, dir, _countof(pattern));
    ::PathCombine(pattern, dir, TEXT("*.ts"));
  }
  else if (PathFileExists(path)) {
    //is dir path
    ::lstrcpyn(dir, path, _countof(dir));
    ::PathAddBackslash(dir);
    ::lstrcpyn(pattern, dir, _countof(pattern));
    ::PathCombine(pattern, dir, TEXT("*.ts"));
  }
  else
    return -1;  // not found

  //search files
  HANDLE hFind;
  WIN32_FIND_DATA fd;
  hFind = FindFirstFile(pattern, &fd);
  if (hFind == INVALID_HANDLE_VALUE)
    return -1;  // fail
  list.clear();
  do {
    if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      && !(fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) {
      list.emplace_back(fd.cFileName);
    }
  } while (FindNextFile(hFind, &fd));

  /*
    windows explorerのようにひら・カナを無視して並べかえる
  */
  //std::sort(list.begin(), list.end(), 
  //  [](const wstring a, const wstring b) { return ::lstrcmpi(a.c_str(), b.c_str()) < 0; });
  /* natural sort */
  std::sort(list.begin(), list.end(),
    [](std::wstring a, std::wstring b) { return ::StrCmpLogicalW(a.c_str(), b.c_str()) < 0; });


  //index at list
  size_t path_idx = 0;
  for (size_t i = 0; i < list.size(); i++) {
    if (::_tcsicmp(list[i].c_str(), name) == 0)
      path_idx = i;
  }
  //trim list
  std::deque<std::wstring> que;
  for (size_t i = path_idx; i < list.size(); i++) {
    if (que.size() < ListMax)
      que.push_back(list[i].c_str());
  }
  for (int i = (int)path_idx - 1; 0 <= i; i--) {
    if (que.size() < ListMax)
      que.push_front(list[i].c_str());
  }
  //index at que
  path_idx = 0;
  for (size_t i = 0; i < que.size(); i++) {
    if (::_tcsicmp(que[i].c_str(), name) == 0)
      path_idx = i;
  }
  list.clear();
  for (std::wstring fname : que) {
    TCHAR fullpath[MAX_PATH] = {};
    PathCombine(fullpath, dir, fname.c_str());
    list.push_back(fullpath);
  }
  return static_cast<int>(path_idx);
}








// 現在位置のPLAY_INFOを前に移動する
bool CPlaylist::MoveCurrentToPrev()
{
    if (m_pos != 0 && m_pos < m_list.size()) {
        std::swap(m_list[m_pos], m_list[m_pos-1]);
        --m_pos;
        return true;
    }
    return false;
}

// 現在位置のPLAY_INFOを次に移動する
bool CPlaylist::MoveCurrentToNext()
{
    if (m_pos+1 < m_list.size()) {
        std::swap(m_list[m_pos], m_list[m_pos+1]);
        ++m_pos;
        return true;
    }
    return false;
}

// ソートまたはシャッフルする
void CPlaylist::Sort(SORT_MODE mode)
{
    std::vector<PLAY_INFO> swapList = m_list;
    std::vector<const PLAY_INFO*> sortList;
    for (size_t i = 0; i < swapList.size(); ++i) {
        sortList.push_back(&swapList[i]);
    }
    if (mode == SORT_ASC || mode == SORT_DESC) {
        //std::sort(sortList.begin(), sortList.end(), [](const PLAY_INFO *a, const PLAY_INFO *b) { return ::lstrcmpi(a->path, b->path) < 0; });
        std::sort(sortList.begin(), sortList.end(), [](const PLAY_INFO *a, const PLAY_INFO *b) { return ::StrCmpLogicalW(a->path, b->path) < 0; });
    }
    else if (mode == SORT_SHUFFLE) {
        std::srand(::GetTickCount());
        std::random_shuffle(sortList.begin(), sortList.end());
    }
    m_list.clear();
    size_t pos = m_pos;
    for (size_t i = 0; i < sortList.size(); ++i) {
        size_t j = mode == SORT_DESC ? sortList.size() - 1 - i : i;
        m_list.push_back(*sortList[j]);
        if (sortList[j] == &swapList[pos]) {
            m_pos = i;
        }
    }
}

// 現在位置のPLAY_INFOを削除する
void CPlaylist::EraseCurrent()
{
    if (!m_list.empty()) {
        m_list.erase(m_list.begin() + m_pos);
        // 現在位置はまず次のPLAY_INFOに移すが、なければ前のPLAY_INFOに移す
        if (m_pos != 0 && m_pos >= m_list.size()) --m_pos;
    }
}

// 現在位置のPLAY_INFO以外を削除する
void CPlaylist::ClearWithoutCurrent()
{
    if (!m_list.empty()) {
        m_list.erase(m_list.begin() + m_pos + 1, m_list.end());
        m_list.erase(m_list.begin(), m_list.begin() + m_pos);
        m_pos = 0;
    }
}

//// 現在位置を前に移動する
//// 移動できなければfalseを返す
//bool CPlaylist::Prev(bool fLoop)
//{
//    if (fLoop && !m_list.empty() && m_pos == 0) {
//        m_pos = m_list.size() - 1;
//        return true;
//    }
//    else if (m_pos != 0) {
//        --m_pos;
//        return true;
//    }
//    return false;
//}
//
//// 現在位置を次に移動する
//// 移動できなければfalseを返す
//bool CPlaylist::Next(bool fLoop)
//{
//  if (fLoop && !m_list.empty() && m_pos + 1 >= m_list.size()) {
//    m_pos = 0;
//    return true;
//  }
//  else if (m_pos+1 < m_list.size()) {
//      ++m_pos;
//      return true;
//  }
//  return false;
//}


//mod
// 現在位置を前に移動する
// 移動できなければfalseを返す
//mod変更点
// ファイルチェックを追加
bool CPlaylist::Prev(bool fLoop)
{
  if (fLoop && !m_list.empty() && m_pos == 0) {
    m_pos = m_list.size() - 1;
    return true;
  }

  while (m_pos != 0)
  {
    --m_pos;
    if (PathFileExists(m_list[m_pos].path))
      return true;
  }
  return false;
}

//mod
// 現在位置を次に移動する
// 移動できなければfalseを返す
//mod変更点
// ファイルチェックを追加
bool CPlaylist::Next(bool fLoop)
{
  if (fLoop && !m_list.empty() && m_pos + 1 >= m_list.size()) {
    m_pos = 0;
    return true;
  }

  while (m_pos + 1 < m_list.size())
  {
    ++m_pos;
    if (PathFileExists(m_list[m_pos].path))
      return true;
  }
  return false;
}







// 文字列として出力する
int CPlaylist::ToString(TCHAR *pStr, int max, bool fFileNameOnly) const
{
    if (!pStr) {
        // 出力に必要な要素数を算出
        int strSize = 1;
        for (std::vector<PLAY_INFO>::const_iterator it = m_list.begin(); it != m_list.end(); ++it) {
            LPCTSTR path = fFileNameOnly ? ::PathFindFileName((*it).path) : (*it).path;
            strSize += ::lstrlen(path) + 2;
        }
        return strSize;
    }
    if (max >= 1) {
        // 出力
        int strPos = 0;
        pStr[0] = 0;
        for (std::vector<PLAY_INFO>::const_iterator it = m_list.begin(); max-strPos-2 > 0 && it != m_list.end(); ++it) {
            LPCTSTR path = fFileNameOnly ? ::PathFindFileName((*it).path) : (*it).path;
            ::lstrcpyn(pStr + strPos, path, max-strPos-2);
            ::lstrcat(pStr + strPos, TEXT("\r\n"));
            strPos += ::lstrlen(pStr + strPos);
        }
        return strPos + 1;
    }
    return 0;
}

bool CPlaylist::IsPlayListFile(LPCTSTR path)
{
    LPCTSTR ext = ::PathFindExtension(path);
    return !::lstrcmpi(ext, TEXT(".m3u")) || !::lstrcmpi(ext, TEXT(".tslist"));
}

bool CPlaylist::IsMediaFile(LPCTSTR path)
{
    LPCTSTR ext = ::PathFindExtension(path);
    return !::lstrcmpi(ext, TEXT(".ts")) || !::lstrcmpi(ext, TEXT(".m2t")) || !::lstrcmpi(ext, TEXT(".m2ts")) || !::lstrcmpi(ext, TEXT(".mp4"));
}
