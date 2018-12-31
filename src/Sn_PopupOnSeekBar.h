#ifndef INCLUDE_Sn_PopupOnSeekBar_H
#define INCLUDE_Sn_PopupOnSeekBar_H
#pragma once
#include "TvtPlayUtil.h"
#include "resource.h"
#include <string>
namespace Snow
{
	/*
	シークバーでのポップアップメニュー
	ここでのCChapterMapは読み込みのみ。チャプター操作はCTvtPlayに任せる。
	*/
	class CPopupOnSeekBar
	{
	public:
		/*▽作成*/
		void NewChapter(ITvtPlayController *tvtp, const POINT &pt, UINT flags,
			int pos0, int pos1);
		/*▽範囲編集*/
		void EditRange(ITvtPlayController *tvtp, const POINT &pt, UINT flags,
			int pos0, int pos1);
		void EditRange2(ITvtPlayController *tvtp, const POINT &pt, UINT flags,
			int pos0, int pos1, std::vector<int> skip);
		/*▽編集*/
		int stepIndex = 0;
		void EditChapter(ITvtPlayController *tvtp, const POINT &pt, UINT flags,
			std::map<int, CChapterMap::CHAPTER>::const_iterator cit,
			int &pos, bool &startSlide);
		/*スライド中止*/
		bool AbortSlide(ITvtPlayController *tvtp, const POINT &pt, UINT flags);
	};


	/*▽作成*/
	void CPopupOnSeekBar::NewChapter(ITvtPlayController *tvtp, const POINT &pt, UINT flags,
		int pos0, int pos1)
	{
		HMENU hmenu = ::CreatePopupMenu();
		if (!hmenu)
			return;

		UINT uFlags = 0 < pos1 ? MF_STRING : MF_STRING | MF_GRAYED;
		::AppendMenu(hmenu, MF_STRING | MF_GRAYED, 0, util::TimeToString(pos0).c_str());
		::AppendMenu(hmenu, MF_SEPARATOR, 0, 0);
		::AppendMenu(hmenu, MF_STRING, 1, L"New ▽");
		::AppendMenu(hmenu, uFlags, 2, L"New skip");
		int id = tvtp->TrackPopup(hmenu, pt, flags);
		::DestroyMenu(hmenu);

		if (id == 1) {
			std::pair<int, CChapterMap::CHAPTER> ch0;
			ch0.first = pos0;
			tvtp->Chapter_Insert(ch0, -1);
		}
		else if (id == 2) {
			std::pair<int, CChapterMap::CHAPTER> ch0, ch1;
			ch0.first = pos0;
			ch1.first = pos1;
			ch0.second.SetSkipBegin(true);
			ch1.second.SetSkipEnd(true);
			tvtp->Chapter_Insert(ch0, -1);
			tvtp->Chapter_Insert(ch1, -1);
		}
		tvtp->BeginWatchingNextChapter(false);
	}


	/*▽範囲編集*/
	void CPopupOnSeekBar::EditRange(ITvtPlayController *tvtp, const POINT &pt, UINT flags,
		int pos0, int pos1)
	{
		HMENU hmenu = ::CreatePopupMenu();
		if (!hmenu)
			return;

		TCHAR text[32];
		std::wstring time0 = util::TimeToString(pos0);
		int len = (pos1 - pos0) / 1000;
		::wsprintf(text, TEXT("▽ %s  [%ds] ▽"), time0.c_str(), len);
		::AppendMenu(hmenu, MF_STRING | MF_GRAYED, 0, text);
		::AppendMenu(hmenu, MF_SEPARATOR, 0, 0);

		const int len30 = 30;
		const int len60 = 60;
		const int len90 = 90;
		MENUITEMINFO mii;
		{
			HMENU hsub = CreatePopupMenu();
			mii = { sizeof(MENUITEMINFO), MIIM_ID | MIIM_TYPE | MIIM_SUBMENU,
					 MFT_STRING, 0, 0, hsub };
			mii.dwTypeData = L"Set Range";
			::AppendMenu(hsub, MF_STRING, len30, TEXT("  30s"));
			::AppendMenu(hsub, MF_STRING, len60, TEXT("  60s"));
			::AppendMenu(hsub, MF_STRING, len90, TEXT("  90s"));
		}

		InsertMenuItem(hmenu, UINT32_MAX, TRUE, &mii);
		::AppendMenu(hmenu, MF_STRING, 1, L"Remove");
		int id = tvtp->TrackPopup(hmenu, pt, flags);
		::DestroyMenu(hmenu);
		if (id == 1) {
			tvtp->Chapter_Erase(pos0);
			tvtp->Chapter_Erase(pos1);
		}
		tvtp->BeginWatchingNextChapter(false);
	}


	/*▽範囲編集*/
	void CPopupOnSeekBar::EditRange2(ITvtPlayController *tvtp, const POINT &pt, UINT flags,
		int pos0, int pos1, std::vector<int> canSkip)
	{
		HMENU hmenu = ::CreatePopupMenu();
		if (!hmenu)
			return;

		TCHAR text[32];
		std::wstring time0 = util::TimeToString(pos0);
		int len = (pos1 - pos0) / 1000;
		::wsprintf(text, TEXT("▽ %s  [%ds] ▽"), time0.c_str(), len);
		::AppendMenu(hmenu, MF_STRING | MF_GRAYED, 0, text);
		::AppendMenu(hmenu, MF_SEPARATOR, 0, 0);

		//const std::vector<int> Len{ 30, 60, 90, 120 };
		const std::vector<int> Len{ 3, 6, 9, 12, 20, 22 };
		MENUITEMINFO mii;
		{
			HMENU sub = CreatePopupMenu();
			mii = { sizeof(MENUITEMINFO), MIIM_ID | MIIM_TYPE | MIIM_SUBMENU,
					 MFT_STRING, 0, 0, sub };
			mii.dwTypeData = L"Set New Length";
			for (int len : Len) {
				bool found = std::count(canSkip.begin(), canSkip.end(), len);
				UINT uFlags = found ? MF_STRING : MF_DISABLED;
				std::wstring text = L"  " + std::to_wstring(len) + L"s";
				::AppendMenu(sub, uFlags, len, text.c_str());
			}
		}
		InsertMenuItem(hmenu, UINT32_MAX, TRUE, &mii);
		::AppendMenu(hmenu, MF_STRING, 1, L"Remove");
		int id = tvtp->TrackPopup(hmenu, pt, flags);
		::DestroyMenu(hmenu);

		//if (id == 1) {
		//	tvtp->Chpater_Erase(pos0);
		//	tvtp->Chpater_Erase(pos1);
		//}
		tvtp->BeginWatchingNextChapter(false);
	}


















	/*▽編集*/
	void CPopupOnSeekBar::EditChapter(ITvtPlayController *tvtp, const POINT &pt, UINT flags,
		std::map<int, CChapterMap::CHAPTER>::const_iterator cit,
		int &pos, bool &startSlide)
	{
		const std::vector<int> step = { 200, 1 * 1000, 15 * 1000 };
		const std::vector<std::wstring> stepText = {
			L"＋  0.2s", L"－  0.2s",
			L"＋  1s",   L"－  1s",
			L"＋ 15s",   L"－ 15s"
		};

		std::pair<int, CChapterMap::CHAPTER> ch = *cit;
		HMENU hmenu = ::CreatePopupMenu();
		if (!hmenu)
			return;
		const UINT IDM_CHAPTER_TITLE = 40100;
		const UINT IDM_CHAPTER_NEXTSTEP = 40200;
		::AppendMenu(hmenu, MF_STRING | MF_GRAYED, IDM_CHAPTER_TITLE, TEXT(""));
		::AppendMenu(hmenu, MF_SEPARATOR, 0, 0);
		::AppendMenu(hmenu, MF_STRING, IDM_CHAPTER_FORWARD, TEXT("＋"));
		::AppendMenu(hmenu, MF_STRING, IDM_CHAPTER_BACKWARD, TEXT("－"));
		::AppendMenu(hmenu, MF_STRING, IDM_CHAPTER_NEXTSTEP, TEXT("            >>"));
		::AppendMenu(hmenu, MF_STRING, IDM_CHAPTER_EDIT, TEXT("Slide"));
		::AppendMenu(hmenu, MF_SEPARATOR, 0, 0);
		::AppendMenu(hmenu, MF_STRING, IDM_CHAPTER_X_IN, TEXT("Skip Begin"));
		::AppendMenu(hmenu, MF_STRING, IDM_CHAPTER_X_OUT, TEXT("Skip End"));
		::AppendMenu(hmenu, MF_SEPARATOR, 0, 0);
		::AppendMenu(hmenu, MF_STRING, IDM_CHAPTER_ERASE, TEXT("Remove"));

		/*＋－とチェックボタンは連続で操作できるようにする*/
		bool save = false, erase = false;
		while (true) {
			/*▽Time*/
			{
				MENUITEMINFO mi;
				mi.cbSize = sizeof(MENUITEMINFO);
				mi.fMask = MIIM_STRING;
				TCHAR text[32];
				std::wstring time = util::TimeToString(ch.first, false, true);
				::wsprintf(text, TEXT("▽ %s"), time.c_str());
				mi.dwTypeData = text;
				::SetMenuItemInfo(hmenu, IDM_CHAPTER_TITLE, FALSE, &mi);
			}
			/*FORWARD, BACKWARD*/
			{
				MENUITEMINFO fw, bw;
				fw.cbSize = sizeof(MENUITEMINFO);
				bw.cbSize = sizeof(MENUITEMINFO);
				fw.fMask = MIIM_STRING;
				bw.fMask = MIIM_STRING;
				int i = stepIndex * (step.size() - 1);
				TCHAR cfw[32], cbw[32];
				::wsprintf(cfw, TEXT("%s"), stepText[i].c_str());
				::wsprintf(cbw, TEXT("%s"), stepText[i + 1].c_str());
				fw.dwTypeData = cfw;
				bw.dwTypeData = cbw;
				::SetMenuItemInfo(hmenu, IDM_CHAPTER_FORWARD, FALSE, &fw);
				::SetMenuItemInfo(hmenu, IDM_CHAPTER_BACKWARD, FALSE, &bw);
			}
			/*Skip Begin, End*/
			{
				MENUITEMINFO begin, end;
				begin.cbSize = sizeof(MENUITEMINFO);
				end.cbSize = sizeof(MENUITEMINFO);
				begin.fMask = MIIM_STATE;
				end.fMask = MIIM_STATE;
				begin.fState = ch.second.IsSkipBegin() ? MF_CHECKED : MF_UNCHECKED;
				end.fState = ch.second.IsSkipEnd() ? MF_CHECKED : MF_UNCHECKED;
				::SetMenuItemInfo(hmenu, IDM_CHAPTER_X_IN, FALSE, &begin);
				::SetMenuItemInfo(hmenu, IDM_CHAPTER_X_OUT, FALSE, &end);
			}

			int	id = tvtp->TrackPopup(hmenu, pt, flags);
			switch (id)
			{
			case IDM_CHAPTER_EDIT:
				startSlide = true;
				break;
			case IDM_CHAPTER_FORWARD:
			{
				int i = stepIndex;
				ch.first += step[i];
				CChapterMap::Clamp(ch);
				save = true;
				//tvtp->Chapter_Insert(ch, pos);
				tvtp->Chapter_Move(pos, ch.first);
				pos = ch.first;
				tvtp->Seek(ch.first);
				continue;/*while*/
			}
			case IDM_CHAPTER_BACKWARD:
			{
				int i = stepIndex;
				ch.first -= step[i];
				CChapterMap::Clamp(ch);
				save = true;
				continue;/*while*/
			}
			case IDM_CHAPTER_NEXTSTEP:
				stepIndex++;
				stepIndex = stepIndex < (int)step.size() ? stepIndex : 0;
				continue;/*while*/
			case IDM_CHAPTER_X_IN:
			{
				bool bg = ch.second.IsSkipBegin();
				ch.second.SetSkipBegin(!bg);
				save = true;
				continue;/*while*/
			}
			case IDM_CHAPTER_X_OUT:
			{
				bool ed = ch.second.IsSkipEnd();
				ch.second.SetSkipEnd(!ed);
				save = true;
				continue;/*while*/
			}
			case IDM_CHAPTER_ERASE:
				erase = true;
				break;
			}/*switch*/
			break;/*while*/
		}/*while*/
		::DestroyMenu(hmenu);

		if (erase)
			tvtp->Chapter_Erase(pos);
		else if (save)
			tvtp->Chapter_Insert(ch, pos);
		if (startSlide)
			pos = ch.first;
		tvtp->BeginWatchingNextChapter(false);
	}


	/*スライド中止*/
	bool CPopupOnSeekBar::AbortSlide(ITvtPlayController *tvtp, const POINT &pt, UINT flags)
	{
		HMENU hmenu = ::CreatePopupMenu();
		if (!hmenu)
			return true;
		::AppendMenu(hmenu, MF_STRING, 1, L"Abort slide");
		int id = tvtp->TrackPopup(hmenu, pt, flags);
		::DestroyMenu(hmenu);
		return id == 1;
	}

}
#endif // INCLUDE_Sn_PopupOnSeekBar_H



