// calc.cpp : Defines the entry point for the application.
//


// TODO:
// - подсветка парных скобок под курсором
// - прыгать по словам с ctrl <- ->
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

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

std::atomic<signed_t> numthreads = 0;
ptr::shared_ptr<MainView> mainview;
DWORD mainthread;
volatile bool globalstop = false;

void do_some_tests();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    do_some_tests();

    cfg.load();

    // Initialize global strings
    LoadStringW(hInstance, IDS_INPUTFONT, szFontFamilyNormal, 32);
    LoadStringW(hInstance, IDS_INPUTFONTHEX, szFontFamilyHex, 32);
    MyRegisterClass(hInstance);

    hInst = hInstance; // Store instance handle in our global variable
    mainview = new MainView();
    if (!mainview->create(nCmdShow))
        return FALSE;

    mainthread = GetCurrentThreadId();

    //HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CALC));

    MSG msg;

    // Main message loop:
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        if (SystemWindowView::WM_EVENT == msg.message)
        {
            SystemWindowView::handle_event(msg.wParam, (void*)msg.lParam);
            continue;
        }

        //if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        //{
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        //}
    }
	globalstop = true;
    SystemWindowView::call_stop();
	cfg.ondie();

    for (; numthreads;) // wait until all threads stop
        Sleep(100);

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CALC));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = nullptr;
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDS_CALC);
    wcex.lpszClassName  = WND_CLASS_STR;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

HWND SystemWindowView::create_window(SystemWindowView *child)
{
    int4 rect = child->get_position().calc_rect(get_canvas_size());
    return CreateWindowW(WND_CLASS_STR, nullptr, WS_CHILD|WS_VISIBLE, rect.left, rect.top, rect.right-rect.left, rect.bottom - rect.top, hwnd, nullptr, hInst, child);
}

bool MainView::create(int /*nCmdShow*/)
{
    hwnd = CreateWindowW(WND_CLASS_STR, L"oxid-calc", WS_OVERLAPPEDWINDOW, cfg.get_wposx(), cfg.get_wposy(), cfg.get_wwidth(), cfg.get_wheight(), nullptr, nullptr, hInst, this);

    if (!hwnd)
        return false;

    ShowWindow(hwnd, cfg.get_wmax() ? SW_MAXIMIZE : SW_NORMAL);
    UpdateWindow(hwnd);

    SetTimer(hwnd, 3437, 500, nullptr);

    RECT r;
    POINT p = {};
	GetClientRect(hwnd, &r);
	ClientToScreen(hwnd, &p);
	int w = r.right - r.left;
	int h = r.bottom - r.top;

	on_poschange(int4(p.x, p.y, p.x + w, p.y + h));

    return true;
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
		{
			GetClientRect(hwnd, &r);
			POINT p = {};
			ClientToScreen(hwnd, &p);
			int w = r.right - r.left;
			int h = r.bottom - r.top;

			wn->on_poschange(int4(p.x, p.y, p.x + w, p.y + h));
        }


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


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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
            v->created();
        }
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
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
                switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
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
			cfg.set_wmax(wParam == SIZE_MAXIMIZED);
		updcfgs(hWnd);
		break;
	case WM_MOVE:
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
        if (SystemWindowView *w = get_view(hWnd))
        {
            w->on_lbm(int2(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), WM_LBUTTONDOWN == message);
        }
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
                if (dynamic_cast<MainView *>(w) != nullptr)
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
                    SystemWindowView *hoverw = get_view(hover);
                    hoverw->on_mm(int2(p.x, p.y));
                    SetCapture(hover);
                    mousecapture = hover;
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
        // NO BREAK HERE TO CALL DefWindowProc
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
