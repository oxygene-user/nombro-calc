#pragma once


class InputView : public CustomCursorWindow
{
	DECLARE_EYELET(InputView);

    typedef CustomCursorWindow super;
	enum 
	{
		PADDING = 5
	};

    HFONT font;
    int2 textsize = {};
    std::vector<int> szperchar;
	signed_t prevcursor = 0, cursor = 0, selected = -1;
    bool insert_mode = false;
    bool selected_by_mouse = false;
	bool read_only = false;
	bool hexview = false;

    DWORD cacyclestart = 0;
    int cursor_alpha = 0;

    int text_x = PADDING;
	int canvas_w = 0;
	Color ramka = 0xff000000;
	Color textcol = 0xff000000;

protected:
	std::wstring prevbuffer;
	std::wstring buffer;
	std::wstring placeholder;
	bool quiet = false; // do not notify

	virtual void on_text_changed();
	virtual void on_key_enter() {}

	wchar_t char_to_view(wchar_t) const;
	wchar_t view_to_char(wchar_t) const;
    void start_cursor_animation_cycle();
    void update_cursor_by_mouse(const int2 &mp);
	void copy();
	void paste();

	void init_hex_font();

	bool is_hexview() const
	{
		return hexview;
	}

public:
    InputView(bool read_only);
    virtual ~InputView();
    /*virtual*/ void draw(Canvas &canvas) override;
    /*virtual*/ void created() override;
    /*virtual*/ bool on_key(int vk, bool down) override;
    /*virtual*/ void on_char(wchar_t c) override;
    /*virtual*/ void on_lbm(const int2& p, bool down) override;
    /*virtual*/ void on_mm(const int2&) override;
    /*virtual*/ bool on_mout(const int2&) override;
	/*virtual*/ void activate() override;

    virtual void animation_tick() override;

	void set_text(const std::wstring_view &t, bool selectall);
	void set_cursor_pos(size_t pos);
	void select_all();
	bool is_cursor_at_end() const
	{
		return cursor == (signed_t)buffer.length() && selected < 0;
	}

	void set_ramka_color(Color c)
	{
		ramka = c;
		invalidate();
	}

	void set_text_color(Color c)
	{
		textcol = c;
		invalidate();
	}

	void set_hexview();
	void set_placeholder(const std::wstring& ph) { placeholder = ph; }
};

class CalculatorInputView : public InputView
{
	typedef InputView super;

	bool calculated = false;

	/*virtual*/ void on_text_changed() override;
	/*virtual*/ void on_key_enter() override;

public:
	CalculatorInputView() :InputView(false) {}
	virtual ~CalculatorInputView() {}

	void colorize_ramka(errset e);

};

class ResultView : public InputView
{
	typedef InputView super;
	
	//mutable std::mutex lock;
	//mutable std::condition_variable wait;

	ptr::shared_ptr<calculating_value> result; // currently calculated base result. It can be used in expression of this result view (addition calc)

	signed_t result_fmt_id = 0;
	
	signed_t showradix = 10;
	signed_t showprec = 100;
	//std::wstring showstr;
	//volatile bool working = false;
	//volatile bool stop = false;

	//enum eee
	//{
		//EVT_END_OF_CALC = 1
	//};
protected:
	// /*virtual*/ void on_event(signed_t eventid) override;
	
public:
	ResultView(signed_t fmt_id);
	virtual ~ResultView();


	void colorize_text(errset e);
	//bool match_id(signed_t id) const { return result_fmt_id == id; }

	void set_result_expression(ptr::shared_ptr<calculating_value> rslt);
	bool show();

	
	/*virtual*/ void on_stop() override;

};
