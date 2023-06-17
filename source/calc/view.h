#pragma once

class View : public ptr::shared_object
{

public:
    View();
    virtual ~View();

    class Position;
    const Position& get_position() const
    {
        return *position.get();
    }

    virtual void created() {} // called just after gui created
    virtual void add_view(View *v, Position *pos);

    class PVal
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
        std::unique_ptr<PVal> left, top, rite, bottom;
    public:
        Position(PVal *l, PVal *t, PVal *r, PVal *b)
        {
            left.reset(l);
            top.reset(t);
            rite.reset(r);
            bottom.reset(b);
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

protected:
    std::vector<ptr::shared_ptr<View>> children;
    std::unique_ptr<Position> position;

public:

    virtual Position * build_absolute_pos(int x, int y, int w, int h);
    virtual int2 get_canvas_size() const = 0; // actual size of canvas (not including borders or titles)
    virtual void draw(Canvas &/*canvas*/) {};
};

using view_sptr = ptr::shared_ptr<View>;
class SystemWindowView;

class SystemWindowView : public View
{
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

    void invalidate();
    void run_animation();
    void stop_animation();

public:
    static const int ANIMATION_TIMER = 3434;

	enum wm
	{
		WM_EVENT = WM_USER + 1212,
	};

public:
    SystemWindowView(HWND hwnd);
    virtual ~SystemWindowView();
    void set_HWND(HWND hwnd_) { hwnd = hwnd_; }
    HWND get_HWND() { return hwnd; }

    void create_system_window();
    /*virtual*/ void draw(Canvas &/*canvas*/) override {}
    virtual void draw(HDC dc);

    /*virtual*/ void add_view(View *v, Position *pos) override;
    /*virtual*/ int2 get_canvas_size() const override;
    virtual void on_stop() {} // called before shutdown
    virtual void on_destroy();
    virtual void on_poschange(const int4 &newposnsize);
	virtual bool on_key(int /*vk*/, bool /*down*/) { return false; }
    virtual void on_char(wchar_t /*char*/, bool /*batch*/) {}
    virtual void animation_tick() {}
    virtual void on_lbm(const int2&, bool /*down*/) {}
    virtual void on_mm(const int2&) {}
    virtual bool on_mout(const int2&) { return true; } // return true if release capture, return false to keep capture
    virtual void on_event(signed_t /*eventid*/) {}

    virtual int4 compute_possize(const int4 &newposnsize, Position &pos);
    void set_height(signed_t height);

	virtual void activate();
	bool is_active();

	void post(signed_t eventid);
    static void handle_event(signed_t eventid, void* thisptr);
    static void call_stop();
};


class CustomCursorWindow : public SystemWindowView
{
    typedef SystemWindowView super;

    HCURSOR hc;
    HCURSOR prevc = nullptr;
public:
    CustomCursorWindow(HWND hwnd, HCURSOR c):hc(c), SystemWindowView(hwnd)
    {
    }
    /*virtual*/ ~CustomCursorWindow()
    {
    }
    /*virtual*/ void on_mm(const int2&) override;
    /*virtual*/ bool on_mout(const int2&) override;
};

