#include "pch.h"
#include <shellapi.h>

void LabelView::init_default_font()
{
	DeleteObject(font);

	HFONT dgf = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	LOGFONTW f = {};
	GetObjectW(dgf, sizeof(f), &f);

	/*
	f.lfHeight = 20;
	f.lfCharSet = DEFAULT_CHARSET;
	f.lfOutPrecision = OUT_DEFAULT_PRECIS;
	f.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	f.lfQuality = DEFAULT_QUALITY;
	f.lfPitchAndFamily = DEFAULT_PITCH;
	memcpy(f.lfFaceName, L"Times", sizeof(f.lfFaceName));
	*/

	font = CreateFontIndirectW(&f);

}

LabelView::~LabelView()
{
	DeleteObject(font);
}

/*virtual*/ void LabelView::draw(Canvas& canvas)
{
	if (!text.empty() && textsize == int2(0))
	{
		HDC dc = GetDC(hwnd);
		HGDIOBJ of = SelectObject(dc, font);
		SIZE sz = {};
		GetTextExtentPoint32W(dc, text.c_str(), (int)text.length(), &sz);
		textsize.x = sz.cx;
		textsize.y = sz.cy;
		SelectObject(dc, of);
		ReleaseDC(hwnd, dc);
	}
	canvas.fill(GetSysColor(15));

	lasttextrect.left = (canvas.size().x - textsize.x) / 2;
	lasttextrect.top = (canvas.size().y - textsize.y) / 2;
	lasttextrect.right = lasttextrect.left + textsize.x;
	lasttextrect.bottom = lasttextrect.top + textsize.y;
	canvas.draw_text(lasttextrect.left, lasttextrect.top, text, font, textcolor);
}

/*virtual*/ LinkLabelView::~LinkLabelView()
{
	DeleteObject(normfont);
	DeleteObject(overfont);
	font = nullptr;
}

void LinkLabelView::init_over_font()
{
	HFONT dgf = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	LOGFONTW f = {};
	GetObjectW(dgf, sizeof(f), &f);

	f.lfUnderline = 1;

	/*
	f.lfHeight = 20;
	f.lfCharSet = DEFAULT_CHARSET;
	f.lfOutPrecision = OUT_DEFAULT_PRECIS;
	f.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	f.lfQuality = DEFAULT_QUALITY;
	f.lfPitchAndFamily = DEFAULT_PITCH;
	memcpy(f.lfFaceName, L"Times", sizeof(f.lfFaceName));
	*/

	overfont = CreateFontIndirectW(&f);

}

/*virtual*/ void LinkLabelView::on_mm(const int2& p)
{
	bool intext = tools::inrect(lasttextrect, p);
	if (font != overfont && intext)
	{
		font = overfont;
		invalidate();
	} else if (font != normfont && !intext)
	{
		font = normfont;
		invalidate();
	}
	super::on_mm(p);
}
/*virtual*/ bool LinkLabelView::on_mout(const int2& p)
{
	if (font != normfont)
	{
		font = normfont;
		invalidate();
	}

	return super::on_mout(p);
}

/*virtual*/ bool LinkLabelView::on_lbm(const int2& p, lbmaction ac)
{
	if (ac==lbmaction::down && tools::inrect(lasttextrect, p))
	{
		ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
		return true;
	}

	return false;
}
