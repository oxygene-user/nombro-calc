#pragma once

class View : public ptr::shared_object
{
protected:
    size_t id = 0;
public:
    View();
    virtual ~View();

    enum class EventType
    {
        POST,
        CONTROL_CLICK,
    };

    size_t get_id() const
    {
        return id;
    }
	void set_id(size_t _id)
	{
		id = _id;
	}

    class Position;
    const Position& get_position() const
    {
        return *position.get();
    }

    virtual size_t add_view(View *v, Position *pos);

    class PVal : public ptr::shared_object
    {
    public:
        virtual ~PVal() {};
        virtual int calc(int size) const = 0;
    };

    class AbsolutePVal : public PVal
    {
        int val;
    public:
        AbsolutePVal(int val) :val(val) {}
        /*virtual*/ int calc(int /*size*/) const override
        {
            return val;
        }
    };

    class PaddingPVal : public PVal
    {
        int val;
    public:
        PaddingPVal(int val) :val(val) {}
        /*virtual*/ int calc(int size) const override
        {
            return size-val;
        }
    };

    class Position
    {
        ptr::shared_ptr<PVal> left, top, rite, bottom;
    public:
        Position(PVal *l, PVal *t, PVal *r, PVal *b)
        {
            left = l;
            top = t;
            rite = r;
            bottom = b;
        }
        int4 calc_rect(int2 parent_size) const
        {
            int l = this->left->calc(parent_size.width);
            int t = this->top->calc(parent_size.height);
            int r = this->rite->calc(parent_size.width);
            int b = this->bottom->calc(parent_size.height);
            return int4( l, t, r, b );
        }
    };

    typedef Position* PPosition;
    struct PositionBuilder
    {
        int height;

        operator PPosition()
        {

        }
    };

    class Layout
    {
        ptr::shared_ptr<PVal> lefttop;
        ptr::shared_ptr<PVal> rightbottom;
        int currentY = 0;
    public:
        Layout(int defpad = 10)
        {
            lefttop = new AbsolutePVal(defpad);
            rightbottom = new PaddingPVal(defpad);
        }

        int bottom() const
        {
            return currentY + lefttop->calc(0);
        }

        Position* vnext(int height);
    };

protected:
    std::vector<ptr::shared_ptr<View>> children;
    std::unique_ptr<Position> position;

public:

    virtual Position * build_absolute_pos(int x, int y, int w, int h);
    virtual int2 get_canvas_size() const = 0; // actual size of canvas (not including borders or titles)
    virtual void draw(Canvas &/*canvas*/) {};
};

enum class lbmaction
{
    down,
    up,
    dbl,
};

using view_sptr = ptr::shared_ptr<View>;
class SystemWindowView;

class SystemWindowView : public View
{
	static void call_stop();
	static void handle_event(signed_t eventid, void* thisptr);
    static ATOM register_window_class(HINSTANCE hInstance);
    static LRESULT CALLBACK wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    HWND create_window(SystemWindowView *child);

    SystemWindowView* next = nullptr;
    SystemWindowView* prev = nullptr;

protected:
    typedef View super;
    HWND hwnd = nullptr;
    view_sptr parent;
    std::unique_ptr<Surface> backbuffer;
    bool redraw_required = true;
    bool animation_started = false;
    bool desktop_window = false; // independent (like popup) window on desktop
    bool is_created = false;

    void invalidate();
    void run_animation();
    void stop_animation();

    bool create_desktop_window(const wchar_t* caption, const int4& rect, bool showmax);
    bool create_desktop_window(const wchar_t* caption, const int2& sz);
	void create_child_window();

    struct syswindow
    {
        std::wstring text;
        std::wstring cln;
		size_t par;
        DWORD styles;
        DWORD exstyles;
        syswindow(DWORD styles, DWORD exstyles);
    };

    virtual void created() { is_created = true; } // called just after gui created
    virtual void prepare(syswindow& /*swp*/) {}

    void ajust_size(const Layout& l);

public:
    static const int ANIMATION_TIMER = 3434;

	enum wm
	{
		WM_EVENT = WM_USER + 1212,
	};

	static int main(HINSTANCE hInstance);

public:
    SystemWindowView();
    virtual ~SystemWindowView();
    void set_HWND(HWND hwnd_) { hwnd = hwnd_; }
    HWND get_HWND() { return hwnd; }
    bool is_desktop_window() const { return desktop_window; }

    /*virtual*/ void draw(Canvas &/*canvas*/) override {}
    virtual void draw(HDC dc);

    /*virtual*/ size_t add_view(View *v, Position *pos) override;
    /*virtual*/ int2 get_canvas_size() const override;
    virtual void on_stop() {} // called before shutdown
    virtual void on_destroy();
    virtual void on_poschange(const int4 &newposnsize);
	virtual bool on_key(int /*vk*/, bool /*down*/) { return false; }
    virtual void on_char(wchar_t /*char*/, bool /*batch*/) {}
    virtual void animation_tick() {}
    virtual bool on_lbm(const int2&, lbmaction /*ac*/) { return false; }
    virtual void on_mm(const int2&) {}
    virtual bool on_mout(const int2&) { return true; } // return true if release capture, return false to keep capture
    virtual void on_event(EventType /*et*/, signed_t /*eventid*/) {}
    virtual void on_close();

    virtual int4 compute_possize(const int4 &newposnsize, Position &pos);
    void set_height(signed_t height);

	virtual void activate();
	bool is_active();

	void call_onposchange();

	void post(signed_t eventid);
};


class CustomCursorView : public SystemWindowView
{
    typedef SystemWindowView super;

    HCURSOR hc;
    HCURSOR prevc = nullptr;
public:
    CustomCursorView(HCURSOR c):hc(c)
    {
    }
    /*virtual*/ ~CustomCursorView()
    {
    }

    virtual bool hover_test(const int2&) const { return true; }

    /*virtual*/ void on_mm(const int2&) override;
    /*virtual*/ bool on_mout(const int2&) override;
};


class GuiControlView : public CustomCursorView
{
	typedef CustomCursorView super;

public:
    GuiControlView(HCURSOR c) :CustomCursorView(c)
	{
	}
	GuiControlView() :CustomCursorView(nullptr)
	{
	}
    
	/*virtual*/ ~GuiControlView()
	{
	}
};

class ButtonView : public GuiControlView
{
    std::wstring btext;
protected:
    virtual void prepare(syswindow& swp);
public:
    ButtonView(const std::wstring_view& btext) :btext(btext)
    {
    }
    /*virtual*/ ~ButtonView() {}
};
