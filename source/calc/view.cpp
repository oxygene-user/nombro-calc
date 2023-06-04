#include "pch.h"

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

void View::add_view(View *v, Position *pos)
{
    for (auto &ptr : children)
    {
        if (&*ptr == v)
            __debugbreak();
    }

    v->position.reset(pos);
    children.emplace_back(v);
}


static SystemWindowView* first = nullptr;
static SystemWindowView* last = nullptr;

SystemWindowView::SystemWindowView(HWND hwnd):hwnd(hwnd)
{
    LIST_ADD(this, first, last, prev, next);
}

SystemWindowView::~SystemWindowView()
{
    LIST_DEL(this, first, last, prev, next);
}

void SystemWindowView::handle_event(signed_t eventid, void* thisptr)
{
    for (SystemWindowView* w = first; w; w = w->next)
        if (w == thisptr)
        {
            w->on_event(eventid);
            break;
        }
}

void SystemWindowView::call_stop()
{
    for (SystemWindowView* w = first; w; w = w->next)
        w->on_stop();
}

void SystemWindowView::create_system_window()
{
    hwnd = ((SystemWindowView *)parent.get())->create_window(this);
}

void SystemWindowView::draw(HDC dc)
{
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

/*virtual*/ void SystemWindowView::add_view(View *v, Position *pos)
{
    super::add_view(v, pos);
    
    if (SystemWindowView *sw = dynamic_cast<SystemWindowView *>(v))
        sw->parent = this;

    if (hwnd != nullptr)
    {
        SystemWindowView *sw = dynamic_cast<SystemWindowView *>(v);
        if (sw != nullptr)
        {
            if (sw->hwnd != nullptr)
                __debugbreak();

            sw->create_system_window();

        }
    }
}

void SystemWindowView::on_destroy()
{
    parent = nullptr;
    hwnd = nullptr;
    for (auto &v : children)
        if (SystemWindowView *sw = dynamic_cast<SystemWindowView *>(&*v))
            sw->on_destroy();
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


/*virtual*/ void CustomCursorWindow::on_mm(const int2&p)
{
    if (prevc == nullptr)
        prevc = SetCursor(hc);
    super::on_mm(p);
}
/*virtual*/ bool CustomCursorWindow::on_mout(const int2&p)
{
    bool rc = super::on_mout(p);
    if (rc)
    {
        SetCursor(prevc);
        prevc = nullptr;
    }
    return rc;
}
