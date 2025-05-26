// calc.cpp : Defines the entry point for the application.
//


// TODO:
// - подсветка парных скобок под курсором
// - выдавать подсказку по кличеству одинаковых цифр в результате при наведении мыши

#include "pch.h"
#include "calc.h"
#include "windowsx.h"

#pragma comment(lib, "Winmm.lib")

#define MAX_LOADSTRING 100
#define WND_CLASS_STR L"OXID_CALC_WC"

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szFontFamilyNormal[32];
WCHAR szFontFamilyHex[32];

volatile std::atomic<signed_t> numthreads = 0;
ptr::shared_ptr<MainView> mainview;
DWORD mainthread;
volatile bool globalstop = false;

void do_some_tests();

int APIENTRY SystemWindowView::main(_In_ HINSTANCE hInstance)
{
    do_some_tests();

    cfg.load();

    // Initialize global strings
    LoadStringW(hInstance, IDS_INPUTFONT, szFontFamilyNormal, 32);
    LoadStringW(hInstance, IDS_INPUTFONTHEX, szFontFamilyHex, 32);
    register_window_class(hInstance);

    hInst = hInstance; // Store instance handle in our global variable
    mainview = new MainView();
    if (!mainview->create())
        return FALSE;


    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CALC));

    MSG msg;

    // Main message loop:
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        if (WM_EVENT == msg.message)
        {
            handle_event(msg.wParam, (void*)msg.lParam);
            continue;
        }

        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
	globalstop = true;
    call_stop();
	cfg.ondie();

    for (; numthreads;) // wait until all threads stop
        Sleep(100);

    mainview = nullptr;

    return (int) msg.wParam;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPWSTR /*lpCmdLine*/, _In_ int /*nCmdShow*/)
{
	mainthread = GetCurrentThreadId();
    return SystemWindowView::main(hInstance);
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM SystemWindowView::register_window_class(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcex.lpfnWndProc    = wnd_proc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CALC));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = nullptr;
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_CALC);
    wcex.lpszClassName  = WND_CLASS_STR;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

SystemWindowView::syswindow::syswindow(DWORD styles, DWORD exstyles) : cln(WND_CLASS_STR), styles(styles), exstyles(exstyles), par(0)
{
}


HWND SystemWindowView::create_window(SystemWindowView *child)
{
    int4 rect = child->get_position().calc_rect(get_canvas_size());

    syswindow swp(WS_CHILD | WS_VISIBLE, 0);
    child->prepare(swp);

    return CreateWindowExW(swp.exstyles, swp.cln.c_str(), swp.text.empty() ? nullptr : swp.text.c_str(), swp.styles, rect.left, rect.top, rect.right-rect.left, rect.bottom - rect.top, hwnd, (HMENU)swp.par, hInst, child);
}


bool SystemWindowView::create_desktop_window(const wchar_t *caption, const int4& rect, bool showmax)
{
    HWND p = nullptr;
    bool root = true;
    if (mainview && mainview.get() != this)
    {
        p = mainview->get_HWND();
        parent = mainview;
        root = false;
    }
	hwnd = CreateWindowExW(0, WND_CLASS_STR, caption, root ? WS_OVERLAPPEDWINDOW : (WS_BORDER|WS_CAPTION| WS_SYSMENU), rect.left, rect.top, rect.rect_width(), rect.rect_height(), p, nullptr, hInst, this);

    if (hwnd)
    {
        desktop_window = true;
		if (!root)
            SetMenu(hwnd, nullptr);

		ShowWindow(hwnd, showmax ? SW_MAXIMIZE : SW_NORMAL);
		UpdateWindow(hwnd);
        return true;
    }

    return false;
}

bool SystemWindowView::create_desktop_window(const wchar_t* caption, const int2& sz)
{
    int4 mainrect(cfg.get_wposx(), cfg.get_wposy(), cfg.get_wposx() + cfg.get_wwidth(), cfg.get_wposy() + cfg.get_wheight());
	HMONITOR m = MonitorFromRect(&ref_cast<RECT>(mainrect), MONITOR_DEFAULTTONEAREST);
	MONITORINFOEXW minf;
	minf.cbSize = sizeof(MONITORINFOEXW);
	GetMonitorInfo(m, &minf);
    int wx = minf.rcWork.left + ((minf.rcWork.right - minf.rcWork.left) - sz.x) / 2;
    int wy = minf.rcWork.top + ((minf.rcWork.bottom - minf.rcWork.top) - sz.y) / 2;
    return create_desktop_window(caption, int4(wx, wy, wx+sz.x, wy + sz.y), false);
}

bool MainView::create()
{
    if (!create_desktop_window(L"oxid-calc", int4(cfg.get_wposx(), cfg.get_wposy(), cfg.get_wposx()+ cfg.get_wwidth(), cfg.get_wposy()+ cfg.get_wheight()), cfg.get_wmax()))
        return false;

    SetTimer(hwnd, 3437, 500, nullptr);

    call_onposchange();
    return true;
}

void SystemWindowView::call_onposchange()
{
	RECT r;
	POINT p = {};
	GetClientRect(hwnd, &r);
	ClientToScreen(hwnd, &p);
	int w = r.right - r.left;
	int h = r.bottom - r.top;

	on_poschange(int4(p.x, p.y, p.x + w, p.y + h));
}

static SystemWindowView * get_view(HWND wnd)
{
    return (SystemWindowView *)GetWindowLongPtrW(wnd, GWLP_USERDATA);
}

static bool looks_like_windows_pos_changed_normally(const int4& oldrect, const int4& newrect)
{
    if (oldrect.lt() == newrect.lt())
        return true; // allow change rb, keeping lt
	if (oldrect.rb() == newrect.rb())
		return true; // allow change lt, keeping rb
	if (oldrect.lb() == newrect.lb())
		return true; // allow change rt, keeping lb
	if (oldrect.rt() == newrect.rt())
		return true; // allow change lb, keeping rt
    if (oldrect.rect_size() == newrect.rect_size())
        return true; // allow move, keeping size

    return false;
}

static void updcfgs(HWND hwnd)
{

	RECT r;
	if (!cfg.get_wmax())
	{
		
        if (SystemWindowView* wn = get_view(hwnd))
            wn->call_onposchange();


		int4 oldr(cfg.get_wposx(), cfg.get_wposy(), cfg.get_wposx() + cfg.get_wwidth(), cfg.get_wposy() + cfg.get_wheight());

        if (mainview != nullptr && mainview->get_HWND() == hwnd)
		{
			GetWindowRect(hwnd, &r);
			if (looks_like_windows_pos_changed_normally(oldr, ref_cast<int4>(r)))
			{
				cfg.set_wposx(r.left);
				cfg.set_wposy(r.top);
				cfg.set_wwidth(r.right - r.left);
				cfg.set_wheight(r.bottom - r.top);
			}
        }
	}

}


LRESULT CALLBACK SystemWindowView::wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND mousecapture = nullptr;

    switch (message)
    {
    case WM_CREATE:
        {
            CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
            SystemWindowView *v = (SystemWindowView *)cs->lpCreateParams;
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, PTR_TO_UNSIGNED(v)); //-V221
            v->set_HWND(hWnd);
            //v->created();
        }
        break;
    case WM_CLOSE:
        if (SystemWindowView* w = get_view(hWnd))
            w->on_close();
        break;
    case WM_TIMER:
        if (SystemWindowView::ANIMATION_TIMER == wParam)
        {
            if (SystemWindowView* w = get_view(hWnd))
                w->animation_tick();
        }
        else if (3437 == wParam)
            cfg.tick();
            
        break;
    case WM_NOTIFY:
        break;
    case WM_COMMAND:
        {
		    int wmId = LOWORD(wParam);
		    int nid = HIWORD(wParam);
            if (wmId >= 100 && nid == BN_CLICKED)
            {
                if (SystemWindowView* w = get_view(hWnd))
                {
                    w->on_event(View::EventType::CONTROL_CLICK, wmId);
                    return 0;
                }
            }


            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                new AboutView(); // not memory leak
                break;
            case IDM_EXIT:
                if (mainview)
                    mainview->on_close();
                else
                    PostQuitMessage(0);
                break;
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            if (SystemWindowView *w = get_view(hWnd))
                w->draw(hdc);
            EndPaint(hWnd, &ps);
        }
        break;
	case WM_SIZE:
        if (mainview != nullptr && mainview->get_HWND() == hWnd)
        {
            cfg.set_wmax(wParam == SIZE_MAXIMIZED);
            updcfgs(hWnd);
        }
        if (SystemWindowView* w = get_view(hWnd))
            if (!w->is_created)
            {
                w->is_created = true;
                w->created();
            }
		break;
	case WM_MOVE:
        if (mainview != nullptr && mainview->get_HWND() == hWnd)
		    updcfgs(hWnd);
		break;
        /*
    case WM_WINDOWPOSCHANGED:
        if (SystemWindowView *w = get_view(hWnd))
            RECT r;
            POINT p = {};
            GetClientRect(hWnd, &r);
            ClientToScreen(hWnd, &p);

            w->on_poschange(int4(p.x, p.y, p.x+r.right-r.left, p.y + r.bottom-r.top));

            if (dynamic_cast<MainView*>(w) != nullptr)
            {
                GetWindowRect(hWnd, &r);

                int4 oldr(cfg.get_wposx(), cfg.get_wposy(), cfg.get_wposx() + cfg.get_wwidth(), cfg.get_wposy() + cfg.get_wheight());

                if (looks_like_windows_pos_changed_normally(oldr, ref_cast<int4>(r)))
				{
					cfg.set_wposx(r.left);
					cfg.set_wposy(r.top);
					cfg.set_wwidth(r.right - r.left);
					cfg.set_wheight(r.bottom - r.top);
                }
            }
        }
        break;
        */
    case WM_DESTROY:
        if (SystemWindowView *w = get_view(hWnd))
            w->on_destroy();
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_CHAR:
        if (SystemWindowView *w = get_view(hWnd))
            w->on_char((wchar_t)wParam, false);
        break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        if (SystemWindowView* w = get_view(hWnd))
        {
            if (w->is_desktop_window())
                BringWindowToTop(w->get_HWND());
            if (w->on_lbm(int2(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), WM_LBUTTONDOWN == message ? lbmaction::down : lbmaction::up))
                return 0;
        }
        break;
    case WM_LBUTTONDBLCLK:
        if (SystemWindowView* w = get_view(hWnd))
        {
            if (w->is_desktop_window())
                BringWindowToTop(w->get_HWND());
            if (w->on_lbm(int2(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), lbmaction::dbl))
                return 0;
        }
        break;
        break;
	case WM_ACTIVATE:
	case WM_ACTIVATEAPP:
		if (SystemWindowView *w = get_view(hWnd))
			w->activate();
		break;
    case WM_MOUSEMOVE:
        if (SystemWindowView *w = get_view(hWnd))
        {
            POINT p;
            p.x = GET_X_LPARAM(lParam);
            p.y = GET_Y_LPARAM(lParam);
            ClientToScreen(hWnd, &p);

            HWND hover = WindowFromPoint(p);
            if (hover == hWnd)
            {
                // just mouse move
                int2 wp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
                w->on_mm(wp);
                if (hover != mousecapture)
                {
                    SetCapture(hover);
                    mousecapture = hover;
                }
                if (w->is_desktop_window())
                {
                    if (wp.x < 0 || wp.y < 0)
                        ReleaseCapture();
                    else {
                        RECT r;
                        GetClientRect(hWnd, &r);
                        if (wp.x >= r.right || wp.y >= r.bottom)
                            ReleaseCapture();
                    }
                    
                }
                break;
            }

            if (hover != nullptr)
            {
                DWORD threadid = GetWindowThreadProcessId(hover, nullptr);
                if (threadid != mainthread)
                    hover = nullptr;
            }
            bool rc = w->on_mout(int2(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
            if (hover != nullptr)
            {
                if (rc)
                {
                    ScreenToClient(hover, &p);
                    if (SystemWindowView* hoverw = get_view(hover))
                    {
                        hoverw->on_mm(int2(p.x, p.y));
                        SetCapture(hover);
                        mousecapture = hover;
                    }
                    else
                    {
                        ReleaseCapture();
                        mousecapture = nullptr;
                    }
                }
            }
            else
            {
                if (rc)
                {
                    ReleaseCapture();
                    mousecapture = nullptr;
                }
            }
        }
        break;
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
		if (SystemWindowView *w = get_view(hWnd))
			if (w->on_key((int)wParam, message == WM_KEYDOWN || message == WM_SYSKEYDOWN))
				return 0; // do not pass handled key to DefWindowProc
    }
	return DefWindowProc(hWnd, message, wParam, lParam);
}

