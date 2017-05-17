/* mod */
#pragma once
#include <Windows.h>
#include <vector>
#include <regex>
#include <filesystem>
namespace fs = std::experimental::filesystem::v1;
using namespace std;


class CyclePopMenu
{
  int Page = 0;
  vector<wstring> Patterns;
  vector<fs::path> Folders;//�Ō�́��͖���
  vector<wstring> FileNames;//�t�@�C�����ꗗ���ꎞ�ۑ�

public:
  ::CyclePopMenu() { }

  void Init(vector<wstring> pattern)
  {
    const int PtnsMax = 3;
    while (PtnsMax < pattern.size())
      pattern.pop_back();
    for (wstring ptn : pattern)
    {
      //�p�^�[���ł͂Ȃ��t�H���_���Ȃ�X�L�b�v
      if (fs::is_directory(ptn)) continue;

      fs::path folder(ptn);
      folder.assign(folder.parent_path());
      if (fs::exists(folder) == false) continue;
      if (fs::is_directory(folder) == false) continue;
      Patterns.push_back(ptn);
      Folders.push_back(folder);
    }
  }

  //
  //��
  //
  int Next(int newPaga = -1)
  {
    Page = 0 <= newPaga ? newPaga : Page + 1;
    Page = Page < static_cast<int>(Patterns.size()) ? Page : 0;
    return Page;
  }


  //
  //�|�b�v�A�b�v���j���[�쐬
  //
  /*
    ID = 0      ohter
    ID = 1      folder item
    ID = 10+    file item
  */
  const UINT MenuIDOffset = 10;
  void CreateMenu(HMENU &hmenu, const wstring playing_path)
  {
    hmenu = ::CreatePopupMenu();
    if (!hmenu) return;
    if (Patterns.empty()) {
      ::AppendMenu(hmenu, MF_STRING | MF_GRAYED, 0, TEXT("[ no folder ]"));
      return;
    }

    const int PopMax = 8;
    vector<wstring> names = GetFileNames(Patterns[Page]);
    if(PopMax < names.size())
      names.resize(PopMax);
    FileNames = names;

    const fs::path folderPath = Folders[Page];
    wstring label = wstring(L"[ ") + wstring(folderPath.filename()) + L" ]";

    // ���j���[����
    hmenu = ::CreatePopupMenu();
    //�擪�Ƀt�H���_��
    ::AppendMenu(hmenu, MF_STRING, 1, label.c_str());
    if (FileNames.empty()) {
      ::AppendMenu(hmenu, MF_STRING | MF_GRAYED, 0, Patterns[Page].c_str());
      ::AppendMenu(hmenu, MF_STRING | MF_GRAYED, 0, TEXT("  no file  "));
    }
    else {
      for (UINT i = 0; i < FileNames.size(); i++)
      {
        wstring text = FileNames[i];
        if (60 < text.length())
          text = text.substr(0, 60) + L"...";
        // �v���t�B�N�X�΍�
        text = regex_replace(text, wregex(L"&"), L"_");

        //�Đ����̃t�@�C���ɂ� CHECKED������
        fs::path fullpath(folderPath.c_str());
        fullpath.append(FileNames[i]);
        bool playing_now = fullpath == playing_path;

        TCHAR tc_text[64];
        ::lstrcpyn(tc_text, text.c_str(), 64);
        MENUITEMINFO mi;
        mi.cbSize = sizeof(MENUITEMINFO);
        mi.fMask = MIIM_ID | MIIM_STATE | MIIM_TYPE;
        mi.wID = i + MenuIDOffset;
        mi.fState = playing_now ? MFS_DEFAULT | MFS_CHECKED : 0;
        mi.fType = MFT_STRING | MFT_RADIOCHECK;
        mi.dwTypeData = tc_text;
        ::InsertMenuItem(hmenu, i + 1, TRUE, &mi);
      }
    }
    //��s�}��
    //  - �f�X�N�g�b�v�������ƃ}�E�X�����Ƀ��j���[������B
    //  - �Q��N���b�N�����Ƃ��ɂ����Ƀt�@�C���I�������Ȃ��悤�ɂ���B
    ::AppendMenu(hmenu, MF_STRING, 0, L"");
    return;
  }
  
  //
  //�t�@�C�������擾
  //
  vector<wstring> GetFileNames(const wstring pattern)
  {
    std::vector<wstring> list;
    //search
    HANDLE hFind;
    WIN32_FIND_DATA fd;
    hFind = FindFirstFile(pattern.c_str(), &fd);
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
    return list;
  }


  //
  //id����t���p�X�擾
  //
  wstring GetSelectedFile(UINT id)
  {
    if (id < MenuIDOffset)
      return wstring();
    id -= MenuIDOffset;
    if (id < 0 || FileNames.size() <= id)
      return wstring();

    fs::path fullpath(Folders[Page]);
    fullpath.append(FileNames[id]);
    return wstring(fullpath);
  }
};



