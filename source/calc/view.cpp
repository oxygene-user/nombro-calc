#include "pch.h"

static size_t idspool = 100; // less values used in menu

View::Position* View::Layout::vnext(int height)
{
    PVal* top = currentY ? new AbsolutePVal(lefttop->calc(0) + currentY) : lefttop.get();
    currentY = top->calc(0) + height;
    return new Position(lefttop.get(), top, rightbottom.get(), new AbsolutePVal(currentY));
}

View::View()
{

}

View::~View()
{

}

/*virtual*/ View::Position * View::build_absolute_pos(int x, int y, int w, int h)
{
    return new Position(new AbsolutePVal(x), new AbsolutePVal(y), new AbsolutePVal(x+w), new AbsolutePVal(y+h));
}

size_t View::add_view(View *v, Position *pos)
{
    for (auto &ptr : children)
    {
        if (&*ptr == v)
            __debugbreak();
    }

    v->position.reset(pos);
    children.emplace_back(v);

    if (v->get_id() == 0)
        v->set_id(idspool++);
    return v->get_id();
}


static SystemWindowView* first = nullptr;
static SystemWindowView* last = nullptr;

SystemWindowView::SystemWindowView()
{
    ASSERT(mainthread == GetCurrentThreadId());
    LIST_ADD(this, first, last, prev, next);
    //OutputDebugStringW((L"new sw: " + std::to_wstring((signed_t)this) + L"\n").c_str());
}

SystemWindowView::~SystemWindowView()
{
    ASSERT(mainthread == GetCurrentThreadId());
	if (hwnd != nullptr)
		DestroyWindow(hwnd);
    LIST_DEL(this, first, last, prev, next);
    //OutputDebugStringW((L"del sw: " + std::to_wstring((signed_t)this) + L"\n").c_str());
}

void SystemWindowView::handle_event(signed_t eventid, void* thisptr)
{
    for (SystemWindowView* w = first; w; w = w->next)
        if (w == thisptr)
        {
            w->on_event(View::EventType::POST, eventid);
            break;
        }
}

void SystemWindowView::call_stop()
{
    for (SystemWindowView* w = first; w; w = w->next)
        w->on_stop();
}

void SystemWindowView::create_child_window()
{
    hwnd = ((SystemWindowView *)parent.get())->create_window(this);
}

void SystemWindowView::draw(HDC dc)
{
    if (desktop_window)
    {
		RECT r;
		GetClientRect(hwnd, &r);
		FillRect(dc, &r, (HBRUSH)(16));
        return;
    }

    int2 mysz = get_canvas_size();
    if (backbuffer == nullptr)
    {
        backbuffer.reset(new Surface(mysz, -1));
    }
    else if (backbuffer->sz != mysz)
    {
        backbuffer->create(mysz, -1);
        redraw_required = true;
    }
    if (redraw_required)
    {
        Canvas c(backbuffer.get());
        draw(c);
        redraw_required = false;
    }

    backbuffer->flush(dc);
}

/*virtual*/ int2 SystemWindowView::get_canvas_size() const
{
    RECT r;
    GetClientRect(hwnd, &r);
    return int2(r.right-r.left, r.bottom-r.top);
}

/*virtual*/ size_t SystemWindowView::add_view(View *v, Position *pos)
{
    size_t cid = super::add_view(v, pos);
    
    if (SystemWindowView *sw = dynamic_cast<SystemWindowView *>(v))
        sw->parent = this;

    if (hwnd != nullptr)
    {
        SystemWindowView *sw = dynamic_cast<SystemWindowView *>(v);
        if (sw != nullptr)
        {
            if (sw->hwnd != nullptr)
                __debugbreak();

            sw->create_child_window();
        }
    }
    return cid;
}

/*virtual*/ void SystemWindowView::on_close()
{
    HWND w2d = hwnd;
    hwnd = nullptr;
	DestroyWindow(w2d);
}

void SystemWindowView::on_destroy()
{
	hwnd = nullptr;
	
    for (auto& v : children)
	{
        if (SystemWindowView* sw = dynamic_cast<SystemWindowView*>(&*v))
        {
            sw->parent = nullptr;
            v = nullptr;
        }
	}

    ptr::shared_ptr<View> me(this); // up ref counter
    if (SystemWindowView *sp = dynamic_cast<SystemWindowView *>(parent.get()))
    {
        for (signed_t i = sp->children.size() - 1; i >= 0; --i)
        {
            if (sp->children[i] == this)
            {
                sp->children.erase(sp->children.begin() + i);
                break;
            }
        }
        
		parent = nullptr;
    }

}

void SystemWindowView::on_poschange(const int4 &newposnsize)
{
    for (auto &v : children)
        if (SystemWindowView *sw = dynamic_cast<SystemWindowView *>(&*v))
        {
            int4 szpos = compute_possize(newposnsize, *sw->position);
            u32 flgs = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_SHOWWINDOW;
            SetWindowPos(sw->hwnd, nullptr, szpos.left, szpos.top, szpos.rect_width(), szpos.rect_height(), flgs);
        }
}

int4 SystemWindowView::compute_possize(const int4 &newposnsize, Position &pos)
{
    return pos.calc_rect(newposnsize.rect_size());
}

void SystemWindowView::ajust_size(const Layout& l)
{
	RECT r;
	GetWindowRect(hwnd, &r);
	int addsz = r.bottom - r.top;
	GetClientRect(hwnd, &r);
	addsz -= (r.bottom - r.top);

	int new_main_window_height = l.bottom() + addsz;
	set_height(new_main_window_height);
}

void SystemWindowView::set_height(signed_t height)
{
    RECT r;
    GetWindowRect(hwnd, &r);
    MoveWindow(hwnd, r.left, r.top, r.right - r.left, (int)height, TRUE);
}

void SystemWindowView::invalidate()
{
    redraw_required = true;
    InvalidateRect(hwnd, nullptr, FALSE);
}

void SystemWindowView::activate()
{
	//SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE);
	SetActiveWindow(hwnd);
	SetFocus(hwnd);
}

bool SystemWindowView::is_active()
{
	return GetFocus() == hwnd;
}

void SystemWindowView::run_animation()
{
    if (animation_started)
        return;
    animation_started = true;
    SetTimer(hwnd, ANIMATION_TIMER, USER_TIMER_MINIMUM, nullptr);
}

void SystemWindowView::stop_animation()
{
    if (!animation_started)
        return;
    animation_started = false;
    KillTimer(hwnd, ANIMATION_TIMER);
}


void SystemWindowView::post(signed_t eventid)
{
	PostThreadMessageW(mainthread, WM_EVENT, eventid, (LPARAM)this);
}


/*virtual*/ void CustomCursorView::on_mm(const int2&p)
{
    bool inr = hover_test(p);
    if (hc != nullptr && prevc == nullptr && inr)
        prevc = SetCursor(hc);
	else if (prevc != nullptr && !inr)
	{
		SetCursor(prevc);
		prevc = nullptr;
	}
    super::on_mm(p);
}
/*virtual*/ bool CustomCursorView::on_mout(const int2&p)
{
    bool rc = super::on_mout(p);
    if (rc && prevc != nullptr)
    {
        SetCursor(prevc);
        prevc = nullptr;
    }
    return rc;
}

/*virtual*/ void ButtonView::prepare(syswindow& swp)
{
    swp.text = btext;
    swp.cln = WSTR("button");
    swp.par = id;
}
