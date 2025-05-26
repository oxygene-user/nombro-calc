#pragma once


class LabelView : public GuiControlView
{
	typedef GuiControlView super;
protected:
	HFONT font = nullptr;
	std::wstring text;
	int2 textsize;
	int4 lasttextrect = {}; // initialized in draw
	Color textcolor = tools::ARGB(0,0,0);
	void init_default_font();
public:
	LabelView(const std::wstring_view& text, HCURSOR c = nullptr) :text(text), GuiControlView(c) { init_default_font(); }
	virtual ~LabelView();

	/*virtual*/ void draw(Canvas& canvas) override;

};

class LinkLabelView : public LabelView
{
	typedef LabelView super;
	std::wstring url;
	HFONT normfont;
	HFONT overfont;
	void init_over_font();
public:
	LinkLabelView(const std::wstring_view &url, const std::wstring_view &text):url(url), LabelView(text, LoadCursorW(nullptr, IDC_HAND)) {
		textcolor = tools::ARGB(0,0,255);
		normfont = font;
		init_over_font();
	}
	/*virtual*/ ~LinkLabelView();

	/*virtual*/ bool hover_test(const int2& p) const override
	{
		return tools::inrect(lasttextrect, p);
	}
	/*virtual*/ void on_mm(const int2&) override;
	/*virtual*/ bool on_mout(const int2&) override;
	/*virtual*/ bool on_lbm(const int2& p, lbmaction ac) override;

};
