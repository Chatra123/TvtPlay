#include <Windows.h>
#include <WindowsX.h>
#include <Shlwapi.h>
#include <map>
#include "Util.h"
#include "StatusView.h"
#include "ChapterMap.h"
/*Snow*/
#include "Sn_ChapterOnSeekBar.h"
#include "Sn_DrawOnSeekBar.h"
#include "Sn_PopupOnSeekBar.h"
#include "TvtPlayUtil.h"
using namespace Snow;




CSeekStatusItem::CSeekStatusItem(CStatusView *pStatus, ITvtPlayController *pPlugin, bool fDrawOfs, bool fDrawTot, int width, int seekMode)
	: CStatusItem(pStatus, STATUS_ITEM_SEEK, max(width, 64))
	, m_pPlugin(pPlugin)
	, m_fDrawOfs(fDrawOfs)
	, m_fDrawTot(fDrawTot)
	, m_seekMode(seekMode == 1 ? 1 : seekMode == 2 ? 2 : 0)
{
	m_MinWidth = m_Width;
	SetMousePos(-1, -1);
	m_spChp = std::make_unique<Snow::CChapterOnSeekBar>();
	m_drawer = std::make_unique<Snow::CDrawOnSeekBar>();
	m_popup = std::make_unique<Snow::CPopupOnSeekBar>();
}

void CSeekStatusItem::Draw(HDC hdc, const RECT *pRect)
{
	/*
	描画命令はTvtpステータスバー左上が(0,0)
	m_mousePosはシークバーパネル左上が(0,0)
	違いに注意。
	*/
	/*
	スキップ▽：　中▽　外枠：濃色　　内側：淡色
	その他▽　：　大▽　　　　淡色
	選択中▽　：　　　　　　　濃色
	*/
	bool fIsPaused = m_pPlugin->IsPaused();
	bool fMouseOn = m_pStatus->GetCurItem() == m_ID;/*マウスがステータスアイテム上にあるか？*/
	POINT mousePos = m_mousePos;
	RECT clientRect;
	GetClientRect(&clientRect);
	m_drawer->SetSize(clientRect);
	/*video*/
	int duration = m_pPlugin->GetDuration();
	int playTime = m_pPlugin->GetApparentPosition();
	double playTimeRatio = 1.0 * playTime / duration;
	playTimeRatio = std::clamp(playTimeRatio, 0.0, 1.0);
	const std::map<int, CChapterMap::CHAPTER> &chMap = m_pPlugin->GetChapter().Get();
	if (fMouseOn == false) {
		m_spChp->ClearSelect();
		m_spChp->ClearSelectRange();
	}

	/*Color*/
	COLORREF crFore = ::GetTextColor(hdc);
	COLORREF crBack = ::GetBkColor(hdc);
	COLORREF crBar = crFore;
	COLORREF crBarFrame = crFore;
	COLORREF crChpM1 = crFore;/*外枠*/
	COLORREF crChpM2 = MixColor(crFore, crBack, 128);/*内側*/
	COLORREF crChpL = MixColor(crFore, crBack, 128);
	COLORREF crSelChp = crFore;/*selected chapter*/
	/*タイムパネルが見やすいように表示中はバー、フレーム、▽を淡色に*/
	if (fMouseOn) {
		BYTE a = 160;/*= 160/255 = 62.5%*/
		crBar = MixColor(crBar, crBack, a);
		crBarFrame = MixColor(crBarFrame, crBack, a);
		crChpM1 = MixColor(crChpM1, crBack, a);
		crChpM2 = MixColor(crChpM2, crBack, a);
		crChpL = MixColor(crChpL, crBack, a);
	}
	crBar = fIsPaused ? MixColor(crBar, crBack, 128) : crBar;

	/*▽*/
	for (auto ch = chMap.begin(); ch != chMap.end(); ++ch) {
		double ratio = 1.0 * ch->first / duration;
		ratio = std::clamp(ratio, 0.0, 1.0);
		TriType type = m_spChp->GetTriType(ch);
		bool isSel = m_spChp->IsSelect(ch->first);
		bool isSelRange = m_spChp->IsSelectRange(ch->first);
		bool isSkip = m_spChp->IsSkipChapter(chMap, ch->first);
		if (isSkip) {
			COLORREF cr1 = isSel || isSelRange ? crSelChp : crChpM1;
			COLORREF cr2 = isSel || isSelRange ? crSelChp : crChpM2;
			m_drawer->DrawTriM(hdc, cr1, cr2, ratio, type);
		}
		else {
			COLORREF cr1 = isSel || isSelRange ? crSelChp : crChpL;
			m_drawer->DrawTriLL(hdc, crBack, cr1, ratio, type);
		}
	}
	/*▽間の線*/
	const auto range = m_spChp->CollectSkipRange(chMap);
	for (const auto &r : range)
	{
		int begin = r.first;
		int end = r.second;
		bool isSelRange = m_spChp->IsSelectRange(begin, end);
		COLORREF cr = isSelRange ? crSelChp : crChpM1;
		double ratio0 = 1.0 * begin / duration;
		double ratio1 = 1.0 * end / duration;
		ratio0 = std::clamp(ratio0, 0.0, 1.0);
		ratio1 = std::clamp(ratio1, 0.0, 1.0);
		m_drawer->DrawSkipLine(hdc, cr, ratio0, ratio1, isSelRange);
	}
	/*▽　Slide中*/
	if (m_spChp->HasSlide()) {
		int t = m_spChp->GetSlide();
		double ratio = 1.0 * t / duration;
		ratio = std::clamp(ratio, 0.0, 1.0);
		TriType type = m_spChp->GetSlideChpType();
		m_drawer->DrawTriLL(hdc, crSelChp, crSelChp, ratio, type);
	}

	/*SeekBar*/
	m_drawer->DrawSeekBar(hdc, crBack, crBar, crBarFrame, playTimeRatio);

	/*タイムパネル*/
	if (fMouseOn) {
		/*	slide			1:02:03 (-0:04:05)
			chapter			1:02:03 +0:04:05
			mouse			1:02:03
		*/
		double mouRatio = m_drawer->PosToBarRatio(mousePos.x);
		int mouTime = static_cast<int>(mouRatio * duration);
		std::wstring text = m_spChp->GetTimePanelText(playTime, mouTime, duration);
		m_drawer->DrawTimePanel(hdc, crBack, mouRatio, text);
	}


	/*開発用*/
	const bool DevelopDraw = false;/* true  false */
	/*各位置に｜表示*/
	if (DevelopDraw) {
		/*マウス位置*/
		RECT stItemRect;
		GetRect(&stItemRect);
		m_drawer->DrawVLine(hdc, crFore, stItemRect.left + mousePos.x, -1);
		COLORREF cr = GetSysColor(COLOR_SCROLLBAR);
		/*▽選択範囲*/
		for (auto ch = chMap.begin(); ch != chMap.end(); ++ch) {
			TriType type = m_spChp->GetTriType(ch);
			double triW = m_drawer->GetTriWRatio();
			double r = 1.0 * ch->first / duration;
			r = std::clamp(r, 0.0, 1.0);
			double rL = r - triW / 2;
			double rR = r + triW / 2;
			double m = triW;
			if (type == TriType::L) {
				m_drawer->DrawVLine(hdc, cr, 0, rL - m);
				m_drawer->DrawVLine(hdc, cr, 0, r + m);
			}
			else if (type == TriType::LR) {
				m_drawer->DrawVLine(hdc, cr, 0, rL - m);
				m_drawer->DrawVLine(hdc, cr, 0, rR + m);
			}
			else if (type == TriType::R) {
				m_drawer->DrawVLine(hdc, cr, 0, r - m);
				m_drawer->DrawVLine(hdc, cr, 0, rR + m);
			}
		}
	}/*DrawDevMode*/

}

void CSeekStatusItem::ProcessSeek(int x)
{
	int dur = m_pPlugin->GetDuration();
	double ratio = m_drawer->PosToBarRatio(x);
	int time = static_cast<int>(ratio * dur) - 1000;
	m_pPlugin->SeekAbsolute(time);
}

void CSeekStatusItem::OnLButtonDown(int x, int y)
{
	CChapterMap &Chapter = m_pPlugin->GetChapter();
	const std::map<int, CChapterMap::CHAPTER> &chMap = m_pPlugin->GetChapter().Get();


	/*スライド　確定、終了*/
	if (m_spChp->HasSlide()) {
		/*chapter移動*/
		int from = m_spChp->GetBeforeSlide();
		int to = m_spChp->GetSlide();
		//m_pPlugin->GetChapter().Move(from, to);
		m_pPlugin->Chapter_Move(from, to);
		m_spChp->ClearSlide();
		return;
	}
	else if (m_spChp->HasSelect()) {
		/*▽選択　シーク*/
		auto chp = chMap.find(m_spChp->GetSelect());
		if (chp != chMap.end())
			m_pPlugin->SeekAbsolute(chp->first);
	}





	/*シーク*/
	int dur = m_pPlugin->GetDuration();
	RECT rc, rcc;
	GetRect(&rc);
	GetClientRect(&rcc);
	if (m_seekMode == 0) {
		ProcessSeek(x);
	}
	else if (m_seekMode == 1) {
		::SetCapture(m_pStatus->GetHandle());
		int pos = ConvUnit(x - (rcc.left - rc.left) - 2, dur, rcc.right - rcc.left - 4);
		/*test*/
		m_pPlugin->SeekAndPreview(max(pos, 0));
		//m_pPlugin->SeekAbsoluteApparently(max(pos - 1000, 0));
		m_pStatus->Invalidate();
	}
	else if (m_seekMode == 2) {
		::SetCapture(m_pStatus->GetHandle());
		ProcessSeek(x);
	}
}

void CSeekStatusItem::OnLButtonUp(int x, int y)
{
	if (m_seekMode == 1 && ::GetCapture() == m_pStatus->GetHandle()) {
		m_pPlugin->SeekAbsoluteApparently(-1);
		ProcessSeek(x);
	}
}


void CSeekStatusItem::OnRButtonDown(int x, int y)
{
	POINT pt;
	UINT flags;
	bool hasMenuPos = GetMenuPos(&pt, &flags);
	if (hasMenuPos)
		pt.x += x;
	const std::map<int, CChapterMap::CHAPTER> &chMap = m_pPlugin->GetChapter().Get();
	double mouPosRatio = m_drawer->PosToBarRatio(m_mousePos.x);
	int	dur = m_pPlugin->GetDuration();
	int mouTime = static_cast<int>(mouPosRatio * dur);

	if (m_spChp->HasSlide() == false) {
		/*新規▽*/
		if (m_spChp->HasSelect() == false && m_spChp->HasSelectRange() == false) {
			int pos0 = mouTime;
			int pos1 = m_spChp->MakeSkipRange(chMap, mouTime);
			if (hasMenuPos)
				m_popup->NewChapter(m_pPlugin, pt, flags, pos0, pos1);
			return;
		}
		/*▽範囲編集*/
		else if (m_spChp->HasSelectRange()) {
			int pos0 = m_spChp->GetSelectRange()[0];
			int pos1 = m_spChp->GetSelectRange()[1];
			std::vector<int> skip = m_spChp->MakeSkipRange2(chMap, pos0, pos1);
			if (hasMenuPos)
				//m_popup->EditRange(m_pPlugin, pt, flags, pos0, pos1);
				m_popup->EditRange2(m_pPlugin, pt, flags, pos0, pos1, skip);
			return;
		}
		/*▽編集*/
		else if (m_spChp->HasSelect()) {
			bool startSlide = false;
			int pos = m_spChp->GetSelect();
			auto ch = chMap.find(pos);
			if (ch != chMap.end() && hasMenuPos)
				m_popup->EditChapter(m_pPlugin, pt, flags, ch, pos, startSlide);
			if (startSlide) {
				auto ch = chMap.find(pos);
				if (ch != chMap.end())
					m_spChp->StartSlide(pos, m_spChp->GetTriType(ch));
			}
			return;
		}

	}
	/*スライド中止*/
	else {
		if (hasMenuPos) {
			bool abort = m_popup->AbortSlide(m_pPlugin, pt, flags);
			if (abort)
				m_spChp->ClearSlide();
		}
		return;
	}
}


void CSeekStatusItem::OnMouseMove(int x, int y)
{
	if (m_drawer->IsInitialized() == false)
		return;

	if (m_mousePos.x != x || m_mousePos.y != y) {
		SetMousePos(x, y);
		if (m_seekMode == 1 && ::GetCapture() == m_pStatus->GetHandle()) {
			int dur = m_pPlugin->GetDuration();
			RECT rc, rcc;
			GetRect(&rc);
			GetClientRect(&rcc);
			int pos = ConvUnit(x - (rcc.left - rc.left) - 2, dur, rcc.right - rcc.left - 4);
			m_pPlugin->SeekAbsoluteApparently(max(pos - 1000, 0));
			m_pStatus->Invalidate();
		}
		else if (m_seekMode == 2 && ::GetCapture() == m_pStatus->GetHandle()) {
			ProcessSeek(x);
		}
		else {
			Update();
		}


		const std::map<int, CChapterMap::CHAPTER> &chMap = m_pPlugin->GetChapter().Get();
		double mouPosRatio = m_drawer->PosToBarRatio(m_mousePos.x);
		int	dur = m_pPlugin->GetDuration();
		double triWRatio = m_drawer->GetTriWRatio();

		/*▽選択*/
		m_spChp->ClearSelect();
		m_spChp->ClearSelectRange();
		if (m_spChp->HasSlide() == false) {
			if (m_drawer->IsInChpArea(m_mousePos.y))
				m_spChp->UpdateSelect(chMap, mouPosRatio, dur, triWRatio);
		}
		/*スライド*/
		else if (m_spChp->HasSlide()) {
			m_spChp->UpdateSlide(chMap, mouPosRatio, dur, triWRatio);
		}
	}

}


CPositionStatusItem::CPositionStatusItem(CStatusView *pStatus, ITvtPlayController *pPlugin)
	: CStatusItem(pStatus, STATUS_ITEM_POSITION, 64)
	, m_pPlugin(pPlugin)
{
	m_MinWidth = 0;
}

void CPositionStatusItem::Draw(HDC hdc, const RECT *pRect)
{
	TCHAR szText[128], szTotText[64];
	int posSec = m_pPlugin->GetApparentPosition() / 1000;
	int durSec = m_pPlugin->GetDuration() / 1000;
	int extMode = m_pPlugin->IsExtending();

	szTotText[0] = 0;
	if (m_pPlugin->IsPosDrawTotEnabled()) {
		int tot = m_pPlugin->GetTotTime();
		int totSec = tot / 1000 + posSec;
		if (tot < 0) ::lstrcpy(szTotText, TEXT(" (不明)"));
		else ::wsprintf(szTotText, TEXT(" (%d:%02d:%02d)"), totSec / 60 / 60 % 24, totSec / 60 % 60, totSec % 60);
	}

	if (durSec < 60 * 60 && posSec < 60 * 60) {
		::wsprintf(szText, TEXT("%02d:%02d/%02d:%02d%s%s"),
			posSec / 60 % 60, posSec % 60,
			durSec / 60 % 60, durSec % 60,
			extMode == 2 ? TEXT("*") : extMode ? TEXT("+") : TEXT(""), szTotText);
	}
	else {
		::wsprintf(szText, TEXT("%d:%02d:%02d/%d:%02d:%02d%s%s"),
			posSec / 60 / 60, posSec / 60 % 60, posSec % 60,
			durSec / 60 / 60, durSec / 60 % 60, durSec % 60,
			extMode == 2 ? TEXT("*") : extMode ? TEXT("+") : TEXT(""), szTotText);
	}
	::DrawText(hdc, szText, -1, const_cast<LPRECT>(pRect),
		DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
}

// 表示に適したアイテム幅を算出する
int CPositionStatusItem::CalcSuitableWidth(HWND hwnd, const LOGFONT &logFont)
{
	int rv = -1;
	{
		HFONT hfont = ::CreateFontIndirect(&logFont);
		if (hfont) {
			HDC hdc = ::GetDC(hwnd);
			if (hdc) {
				// 表示に適したアイテム幅を算出
				TCHAR szText[128];
				::lstrcpy(szText, TEXT("00:00:00/00:00:00+"));
				if (m_pPlugin->IsPosDrawTotEnabled()) ::lstrcat(szText, TEXT(" (00:00:00)"));
				HFONT hfontOld = SelectFont(hdc, hfont);
				RECT rc;
				::SetRectEmpty(&rc);
				if (::DrawText(hdc, szText, -1, &rc,
					DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_CALCRECT))
				{
					rv = rc.right - rc.left;
				}
				SelectFont(hdc, hfontOld);
				::ReleaseDC(hwnd, hdc);
			}
			::DeleteObject(hfont);
		}
	}
	return rv;
}

void CPositionStatusItem::OnRButtonDown(int x, int y)
{
	POINT pt;
	UINT flags;
	if (GetMenuPos(&pt, &flags)) {
		m_pPlugin->SetupWithPopup(pt, flags);
	}
}

CButtonStatusItem::CButtonStatusItem(CStatusView *pStatus, ITvtPlayController *pPlugin, int id, int subID, int width, const DrawUtil::CBitmap &icon)
	: CStatusItem(pStatus, id, max(width, 0))
	, m_pPlugin(pPlugin)
	, m_subID(subID)
	, m_icon(icon)
{
	m_MinWidth = m_Width;
}

void CButtonStatusItem::Draw(HDC hdc, const RECT *pRect)
{
	int cmdID = m_ID - STATUS_ITEM_BUTTON;
	int subCmdID = m_subID - STATUS_ITEM_BUTTON;
	int iconPos = 0;

	if (cmdID == ID_COMMAND_REPEAT_CHAPTER) {
		iconPos = m_pPlugin->IsRepeatChapterEnabled() ? 1 : 0;
	}
	if (cmdID == ID_COMMAND_SKIP_X_CHAPTER) {
		iconPos = m_pPlugin->IsSkipXChapterEnabled() ? 1 : 0;
	}
	else if (cmdID == ID_COMMAND_LOOP) {
		iconPos = m_pPlugin->IsSingleRepeat() ? 2 : m_pPlugin->IsAllRepeat() ? 1 : 0;
	}
	else if (cmdID == ID_COMMAND_PAUSE) {
		iconPos = !m_pPlugin->IsOpen() || m_pPlugin->IsPaused() ? 1 : 0;
	}
	else if (cmdID == ID_COMMAND_STRETCH || cmdID == ID_COMMAND_STRETCH_RE || cmdID == ID_COMMAND_STRETCH_POPUP) {
		int stid = m_pPlugin->GetStretchID();
		iconPos = stid < 0 ? 0 : stid + 1;
	}
	else if (ID_COMMAND_STRETCH_A <= cmdID && cmdID < ID_COMMAND_STRETCH_A + COMMAND_S_MAX) {
		int stid = m_pPlugin->GetStretchID();
		iconPos = stid == cmdID - ID_COMMAND_STRETCH_A ? 1 :
			/* サブコマンドもStretchなら3つ目も使う */
			(ID_COMMAND_STRETCH_A <= subCmdID && subCmdID < ID_COMMAND_STRETCH_A + COMMAND_S_MAX) &&
			stid == subCmdID - ID_COMMAND_STRETCH_A ? 2 : 0;
	}
	DrawIcon(hdc, pRect, m_icon.GetHandle(), ICON_SIZE * iconPos);
	//int h = pRect->bottom - pRect->top;
	//DrawIcon(hdc, pRect, m_icon.GetHandle(), h * iconPos);
}

void CButtonStatusItem::OnLButtonDown(int x, int y)
{
	POINT pt;
	UINT flags;
	if (GetMenuPos(&pt, &flags)) {
		m_pPlugin->OnCommand(m_ID - STATUS_ITEM_BUTTON, &pt, flags);
	}
}

void CButtonStatusItem::OnRButtonDown(int x, int y)
{
	POINT pt;
	UINT flags;
	if (GetMenuPos(&pt, &flags)) {
		m_pPlugin->OnCommand(m_subID - STATUS_ITEM_BUTTON, &pt, flags);
	}
}