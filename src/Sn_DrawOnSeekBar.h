#ifndef INCLUDE_Sn_DrawOnSeekBar_H
#define INCLUDE_Sn_DrawOnSeekBar_H
#pragma once
#include <Shlwapi.h>
#include <memory>
#pragma comment (lib, "msimg32.lib")
/*
#include <string>
std::wstring mes = L"  ";
mes += L"   ";
mes += std::to_wstring();
mes += L"\r\n";
OutputDebugString(mes.c_str());

OutputDebugString(std::to_wstring().c_str());
*/

namespace Snow
{
	/*シークバーの描画*/
	class CDrawOnSeekBar
	{
	private:
		bool Initialized = false;
		RECT Rect, BarRect;
		int RectH, BarRectW;
		int TriW, TriH;
		int TriH_LL, TriW_LL;

	public:
		/*初期設定*/
		void SetSize(RECT rect);
		bool IsInitialized() { return Initialized; }
		/*マウスはチャプターエリア上？*/
		bool IsInChpArea(int posY) { return posY < BarRect.top; }
		bool IsInBarArea(int posY) { return BarRect.top < posY; }
		/*pos x --> bar ratio*/
		double PosToBarRatio(int posX);
		int BarRatioToPos(double ratio);
		/*バー全長に対する▽の大きさ、比率*/
		double GetTriWRatio() { return 1.0 * TriW / BarRectW; }

		/*描画  SeekBar*/
		void DrawSeekBar(const HDC &hdc,
			const COLORREF &crBack, const COLORREF &crBar, const COLORREF &crFrame,
			double playTimeRatio);
		/*描画  中▽*/
		void DrawTriM(const HDC &hdc, const COLORREF &cr1, const COLORREF &cr2,
			double posRatio, TriType type);
		/*描画  大▽*/
		void DrawTriLL(const HDC &hdc, const COLORREF &cr1, const COLORREF &cr2,
			double posRatio, TriType type);
		/*描画  ▽*/
		void DrawTri(const HDC &hdc, const COLORREF &cr1, const COLORREF &cr2,
			double posRatio, TriType type, int W, int H);
		/*描画  スキップ▽間の線*/
		void DrawSkipLine(const HDC &hdc, const COLORREF &cr,
			double posRatio0, double posRatio1, bool intensity);
		/*描画  シークバー上のタイムパネル*/
		void DrawTimePanel(const HDC &hdc, const COLORREF &crBack,
			double mousePosRatio, std::wstring text);

		/*develop util*/
		/*描画 | */
		void DrawVLine(const HDC &hdc, const COLORREF &cr, int posX, double barRatio);

	};

	/*初期設定*/
	void CDrawOnSeekBar::SetSize(RECT rect)
	{
		this->Rect = rect;
		this->RectH = Rect.bottom - Rect.top;
		this->BarRect = Rect;
		this->BarRect.left += 10;
		this->BarRect.top = static_cast<int>(RectH * (1 - 0.3));
		this->BarRect.right -= 10;
		this->BarRectW = BarRect.right - BarRect.left;
		int h = static_cast<int>(RectH * 0.4);
		this->TriH = h;
		this->TriW = static_cast<int>(h * 1.4);
		this->TriH_LL = static_cast<int>(h * 1.4);
		this->TriW_LL = static_cast<int>(h * 1.4 * 1.4);
		this->Initialized = true;
	}


	/*pos x --> bar ratio*/
	/*posX: シークバーパネル左上が(0,0)のx*/
	double CDrawOnSeekBar::PosToBarRatio(int posX)
	{
		/*gap:  パネル左端とバー右端のすきま*/
		int gap = BarRect.left - Rect.left;
		double r = 1.0 * (posX - gap) / BarRectW;
		r = std::clamp(r, 0.0, 1.0);
		return r;
	}
	int CDrawOnSeekBar::BarRatioToPos(double ratio)
	{
		int gap = BarRect.left - Rect.left;
		int x = static_cast<int>(ratio * BarRectW);
		x = std::clamp(x, 0, BarRectW);
		return x + gap;
	}


	/*描画 SeekBar*/
	void CDrawOnSeekBar::DrawSeekBar(const HDC &hdc,
		const COLORREF &crBack, const COLORREF &crBar, const COLORREF &crFrame,
		double playTimeRatio)
	{
		RECT rc;
		/*クリア　▽下点がバーに重なるので*/
		rc = Rect;
		rc.top = BarRect.top;
		DrawUtil::Fill(hdc, &rc, crBack);
		/*Bar*/
		/*再生位置のシークバー右端*/
		int barLen = static_cast<int>(playTimeRatio * BarRectW);
		barLen = std::clamp(barLen, 0, BarRectW);
		rc = BarRect;
		rc.top += 2;
		rc.right = BarRect.left + barLen;
		DrawUtil::Fill(hdc, &rc, crBar);

		/*BarFrame*/
		HPEN hpen = ::CreatePen(PS_SOLID, 1, crFrame);
		if (hpen) {
			HPEN hpenOld = SelectPen(hdc, hpen);
			HBRUSH hbrOld = SelectBrush(hdc, ::GetStockObject(NULL_BRUSH));
			rc = BarRect;
			::Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
			SelectPen(hdc, hpenOld);
			SelectBrush(hdc, hbrOld);
			::DeletePen(hpen);
		}
	}

	/*描画 ▽*/
	void CDrawOnSeekBar::DrawTriM(const HDC &hdc, const COLORREF &cr1, const COLORREF &cr2,
		double posRatio, TriType type)
	{
		DrawTri(hdc, cr1, cr2, posRatio, type, TriW, TriH);
	}
	/*描画 大▽*/
	void CDrawOnSeekBar::DrawTriLL(const HDC &hdc, const COLORREF &cr1, const COLORREF &cr2,
		double posRatio, TriType type)
	{
		DrawTri(hdc, cr1, cr2, posRatio, type, TriW_LL, TriH_LL);
	}
	/*描画 ▽
		cr1:外枠
		cr2:内側
	*/
	void CDrawOnSeekBar::DrawTri(const HDC &hdc, const COLORREF &cr1, const COLORREF &cr2,
		double posRatio, TriType type, int W, int H)
	{
		/*▽Locaiton*/
		int x = BarRect.left + static_cast<int>(posRatio * BarRectW);
		int yTop = BarRect.top - H;/*▽上辺*/
		int yBottom = BarRect.top + 3;/*▽下点　少しバーに重ねる*/
		/*左▽*/
		POINT triL[3] = { x, yBottom,
						 x, yTop ,
						 x - W / 2, yTop };
		/*▽*/
		POINT triC[3] = { x, yBottom ,
						x - W / 2, yTop ,
						x + W / 2, yTop };
		/*右▽*/
		POINT triR[3] = { x, yBottom,
						 x, yTop ,
						 x + W / 2, yTop };

		HPEN hpen = ::CreatePen(PS_SOLID, 1, cr1);
		HBRUSH hbr = ::CreateSolidBrush(cr2);
		if (hpen && hbr) {
			HPEN hpenOld = SelectPen(hdc, hpen);
			HBRUSH hbrOld = SelectBrush(hdc, hbr);
			if (type == TriType::L)
				::Polygon(hdc, triL, 3);
			else if (type == TriType::LR)
				::Polygon(hdc, triC, 3);
			else if (type == TriType::R)
				::Polygon(hdc, triR, 3);
			SelectPen(hdc, hpenOld);
			SelectBrush(hdc, hbrOld);
			::DeleteBrush(hbr);
			::DeletePen(hpen);
		}
	}

	/*描画  スキップ▽間の線*/
	void CDrawOnSeekBar::DrawSkipLine(const HDC &hdc, const COLORREF &cr,
		double posRatio0, double posRatio1,
		bool intensity)
	{
		/*▽size*/
		int bold = static_cast<int>(TriH * 0.15);
		bold = 1 < bold ? bold : 1;
		bold = intensity ? bold * 2 : bold;
		int yTop = BarRect.top - TriH;/*▽上辺*/
		yTop += bold - 1;
		/*ratio --> pos x*/
		int x0 = BarRect.left + static_cast<int>(posRatio0 * BarRectW);
		int x1 = BarRect.left + static_cast<int>(posRatio1 * BarRectW);
		HPEN hpen = ::CreatePen(PS_SOLID, bold, cr);
		if (hpen) {
			HPEN hpenOld = SelectPen(hdc, hpen);
			::MoveToEx(hdc, x0, yTop, NULL);
			::LineTo(hdc, x1, yTop);
			SelectObject(hdc, hpenOld);
			::DeletePen(hpen);
		}
	}


	/*描画  シークバー上のタイムパネル*/
	void CDrawOnSeekBar::DrawTimePanel(const HDC &hdc, const COLORREF &crBack,
		double mousePosRatio, std::wstring text)
	{
		/*テキスト描画の幅を取得*/
		int panelW;
		{
			RECT rc = {};
			if (::DrawText(hdc, text.c_str(), -1, &rc,
				DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_CALCRECT))
				panelW = rc.right - rc.left;
			else
				panelW = 150;
		}
		/*表示位置　マウスの左右に表示*/
		int panelX;
		{
			const int ofs = 20;
			int mx = BarRect.left + static_cast<int>(mousePosRatio * BarRectW);
			int posL = mx - ofs - panelW;
			int posR = mx + ofs;
			bool hasSpaceL = BarRect.left < posL;
			bool hasSpaceR = posR + panelW < BarRect.right;
			panelX = hasSpaceR ? posR
				: hasSpaceL ? posL
				: posR;
		}

		RECT rc;
		int x, y, w, h;
		/*背景色でクリア*/
		x = panelX;
		y = Rect.top;
		w = panelW;
		h = Rect.bottom - Rect.top;
		HDC hbuff = CreateCompatibleDC(hdc);
		HBITMAP hbmp = CreateCompatibleBitmap(hdc, w, h);
		SelectObject(hbuff, hbmp);
		::SetRect(&rc, 0, 0, w, h);
		DrawUtil::Fill(hbuff, &rc, crBack);
		BLENDFUNCTION bl = {};
		bl.SourceConstantAlpha = 180;/*hbuffの不透明度　大→背景色を濃く*/
		AlphaBlend(hdc, x, y, w, h, hbuff, 0, 0, w, h, bl);
		DeleteDC(hbuff);
		DeleteObject(hbmp);
		/*文字、重ねて太字*/
		x = panelX;
		y = Rect.top;
		w = panelX + panelW;
		h = Rect.bottom;
		::SetRect(&rc, x, y, w, h);
		::DrawText(hdc, text.c_str(), -1, &rc, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
		rc.left += 1;
		rc.top += 1;
		::DrawText(hdc, text.c_str(), -1, &rc, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
	}


	/*develop util*/
	/*描画 | */
	/*posX    :  tvtpステータスバー左上(0,0)の座標*/
	/*barRatio:  バー上の比率位置*/
	void CDrawOnSeekBar::DrawVLine(const HDC &hdc, const COLORREF &cr, int posX, double barRatio = -1)
	{
		int x = posX;
		if (barRatio != -1)
			x = BarRect.left + static_cast<int>(barRatio * BarRectW);
		int y0 = Rect.top;
		int y1 = Rect.bottom;

		HPEN hpen = ::CreatePen(PS_SOLID, 1, cr);
		if (hpen) {
			HPEN hpenOld = SelectPen(hdc, hpen);
			::MoveToEx(hdc, x, y0, NULL);
			::LineTo(hdc, x, y1);
			SelectObject(hdc, hpenOld);
			::DeletePen(hpen);
		}
	}

}
#endif // INCLUDE_Sn_DrawOnSeekBar_H



