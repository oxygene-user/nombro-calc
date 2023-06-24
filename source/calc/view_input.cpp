#include "pch.h"

extern WCHAR szFontFamilyNormal[32];
extern WCHAR szFontFamilyHex[32];

#define CURSOR_TIME_FULL 100
#define CURSOR_TIME_FADEOUT 600

InputView::InputView(bool read_only):CustomCursorWindow(nullptr, LoadCursorW(nullptr, IDC_IBEAM)), read_only(read_only)
{
    LOGFONTW f = {};
    f.lfHeight = 30;
    f.lfCharSet = DEFAULT_CHARSET;
    f.lfOutPrecision = OUT_DEFAULT_PRECIS;
    f.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    f.lfQuality = DEFAULT_QUALITY;
    f.lfPitchAndFamily = DEFAULT_PITCH;
    memcpy(f.lfFaceName, szFontFamilyNormal, sizeof(f.lfFaceName));
    
    font = CreateFontIndirectW(&f);

}

InputView::~InputView()
{
    DeleteObject(font);
}

void InputView::init_hex_font()
{
	DeleteObject(font);

	LOGFONTW f = {};
	f.lfHeight = 30;
	f.lfCharSet = DEFAULT_CHARSET;
	f.lfOutPrecision = OUT_DEFAULT_PRECIS;
	f.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	f.lfQuality = DEFAULT_QUALITY;
	f.lfPitchAndFamily = DEFAULT_PITCH;
    f.lfWeight = 600;
	memcpy(f.lfFaceName, szFontFamilyHex, sizeof(f.lfFaceName));

	font = CreateFontIndirectW(&f);

}

void InputView::set_hexview()
{
    if (!hexview)
    {
        hexview = true;
        init_hex_font();
    }
}


void InputView::on_text_changed()
{
	if (!hwnd)
		return;


	if (prevbuffer != buffer)
	{
		if (read_only)
		{
			cursor = prevcursor;
			buffer = prevbuffer;
			return;
		}
	}

	prevcursor = cursor;

    // recalc text size

    szperchar.resize(buffer.length()+1);
    if (szperchar.size() == 1)
    {
        szperchar[0] = 0;

		if (textsize.y == 0)
		{
			HDC dc = GetDC(hwnd);
			HGDIOBJ of = SelectObject(dc, font);
			SIZE sz = {};
			GetTextExtentPoint32W(dc, L" ", 1, &sz);
			textsize.y = sz.cy;
			SelectObject(dc, of);
			ReleaseDC(hwnd, dc);
		}

        return;
    }
    HDC dc = GetDC(hwnd);
    HGDIOBJ of = SelectObject(dc, font);
    SIZE sz = {};
    for (signed_t i = 1; i <= (signed_t)buffer.length(); ++i)
    {
        GetTextExtentPoint32W(dc, buffer.c_str(), (int)i, &sz);
        szperchar[i] = sz.cx;
    }
    SelectObject(dc, of);
    ReleaseDC(hwnd,dc);
    textsize.x = sz.cx;
    textsize.y = sz.cy;
    szperchar[0] = 0;

    if (!hexview)
    {
        std::vector<bool> expandhere;
		auto expand = [&](signed_t z)
		{
            expandhere.resize(szperchar.size());
            expandhere[z] = true;
			//for (; z < (signed_t)szperchar.size(); ++z)
				//szperchar[z] += 3;
		};

		signed_t ne = -1;
        bool like_hex = false;
        wchar_t prevcc = 0;
		for (signed_t i = buffer.length() - 1; i >= 0; --i)
		{
			wchar_t c = buffer[i];
			if (c >= '0' && c <= '9')
			{
                if (prevcc == 'h')
                    like_hex = true;

				if (ne < 0)
					ne = i;
				continue;
			}

            if (ne >= 0)
			{
				if (c == '.')
				{
                    if (!like_hex)
					    for (signed_t x = i + 4; x <= ne; x += 3)
						    expand(x);
					ne = -1;
					continue;
				}

				if (c == 'x' || c == '#' || c == '$' || like_hex)
				{
					for (signed_t x = ne - 1; x > i + 1; x -= 2)
						expand(x);

					ne = -1;
					continue;
				}

				for (signed_t x = ne - 2; x > i + 1; x -= 3)
					expand(x);
				ne = -1;
			}
            prevcc = c;
		}
		if (ne >= 0)
		{
            if (like_hex)
            {
                for (signed_t x = ne - 1; x > 0; x -= 2)
                    expand(x);
            } else {
				for (signed_t x = ne - 2; x > 0; x -= 3)
					expand(x);
            }
		}

        if (expandhere.size() > 0)
        {
			int acc = 0;
			for (signed_t z = 0; z < (signed_t)szperchar.size(); ++z)
			{
				if (expandhere[z])
					acc += 3;
				szperchar[z] += acc;
			}
        }

    }
}

/*virtual*/ void InputView::created()
{
    on_text_changed();
    run_animation();
    super::created();
}

/*virtual*/ void InputView::draw(Canvas &canvas)
{
	canvas_w = canvas.size().x;
    canvas.fill_rect(int4(1,canvas.size()-int2(1)), 0xffffffff);

    int text_y = (canvas.size().height - textsize.height) / 2;

	int cx = text_x + szperchar[cursor] - 1;

    if (selected >= 0 && selected != cursor)
    {
        int x0 = szperchar[min(selected, cursor)];
        int x1 = szperchar[max(selected, cursor)];
        canvas.fill_rect(int4(text_x+x0, text_y, text_x+x1, text_y + textsize.height), tools::ARGB(200,200,255));
    }

    if (buffer.empty())
    {
        if (!placeholder.empty())
        {
            canvas.draw_text(text_x, text_y, placeholder, font, tools::ARGB(220, 220, 220));
        }
    }
    else
	{
		canvas.draw_text(text_x, text_y, buffer, font, textcol, szperchar.data());
    }

    if (cursor_alpha > 0)
    {
        if (insert_mode && selected < 0 && cursor < (signed_t)szperchar.size()-1)
        {
            int cx2 = text_x + szperchar[cursor+1] - 2;
            if (cursor_alpha == 255) {
                canvas.fill_rect(int4(cx, text_y, cx + 2, text_y + textsize.height), textcol);
                canvas.fill_rect(int4(cx2, text_y, cx2 + 2, text_y + textsize.height), textcol);
                canvas.fill_rect(int4(cx+2, text_y, cx2, text_y+2), textcol);
                canvas.fill_rect(int4(cx + 2, text_y + textsize.height-2, cx2, text_y + textsize.height), textcol);
            } else {
                canvas.blend_rect(int4(cx, text_y, cx + 2, text_y + textsize.height), textcol, (u8)cursor_alpha);
                canvas.blend_rect(int4(cx2, text_y, cx2 + 2, text_y + textsize.height), textcol, (u8)cursor_alpha);
                canvas.blend_rect(int4(cx + 2, text_y, cx2, text_y + 2), textcol, (u8)cursor_alpha);
                canvas.blend_rect(int4(cx + 2, text_y + textsize.height - 2, cx2, text_y + textsize.height), textcol, (u8)cursor_alpha);
            }
        }
        else
        {
            if (cursor_alpha == 255)
                canvas.fill_rect(int4(cx, text_y, cx + 2, text_y + textsize.height), textcol);
            else
                canvas.blend_rect(int4(cx, text_y, cx + 2, text_y + textsize.height), textcol, (u8)cursor_alpha);
        }
    }

    canvas.draw_edge(ramka);
    
}

void InputView::copy()
{
	if (selected >= 0)
	{
		signed_t r0 = min(cursor, selected);
		signed_t r1 = max(cursor, selected);

		std::wstring ct = buffer.substr(r0, r1 - r0);
		for (size_t i = 0; i < ct.size(); ++i)
			ct[i] = view_to_char(ct[i]);

		OpenClipboard(nullptr);
		EmptyClipboard();

		signed_t len = ct.size() + 1;

		HANDLE text = GlobalAlloc(GMEM_MOVEABLE, len * sizeof(wchar_t));
		void *d = GlobalLock(text);
		memcpy(d, ct.c_str(), ct.size() * sizeof(wchar_t));
		*(((wchar_t *)d) + ct.size()) = 0;
		GlobalUnlock(text);

		SetClipboardData(CF_UNICODETEXT, text);
		CloseClipboard();
	}
}

void InputView::paste()
{
	if (IsClipboardFormatAvailable(CF_UNICODETEXT) && OpenClipboard(nullptr))
	{
		HGLOBAL hg = GetClipboardData(CF_UNICODETEXT);
		if (hg)
		{
			const wchar_t *p = (const wchar_t*)GlobalLock(hg);
			quiet = true;
			for (size_t i = 0; 0 != p[i]; ++i)
			{
				if (!p[i + 1])
					quiet = false;
				on_char(p[i], true);
			}
			GlobalUnlock(hg);
		}
		CloseClipboard();

        on_char(0, false);
	}

}

wchar_t InputView::char_to_view(wchar_t c) const
{
	if (c >= '0' && c <= '9')
		return c;
	
	if (c >= 'a' && c <= 'z')
		return c;

	if (c >= 'A' && c <= 'Z')
		return c + 32;

	switch (c)
	{
	case '*':
		return 215;
	case '/':
		return 247;
	case '-':
		return 8722;

	case '+':
	case '(':
	case ')':
	case '^':
	case '&':
	case '|':
	case '<':
	case '>':
	case '%':
	case '.':
	case ' ':
    case '#':
    case '$':
    case '\\':
		return c;
	default:
		break;
	}

	return 0;
}
wchar_t InputView::view_to_char(wchar_t c) const
{
	switch (c)
	{
	case 215:
		return '*';
	case 247:
		return '/';
	case 8722:
		return '-';
	}
	return c;
}


void InputView::move_cursor(signed_t delta, bool by_words)
{
    if (by_words)
    {
        for (;;)
        {
			cursor += delta;

            if (cursor < 0)
            {
                cursor = 0;
                return;
            }
            if (cursor > (signed_t)buffer.length())
            {
                cursor = (signed_t)buffer.length();
                return;
            }

            wchar_t c = buffer[cursor];
            if (c == '.' || c == ' ')
                break;
        }


        return;
    }

	cursor += delta;
	if (cursor < 0)
		cursor = 0;
	if (cursor > (signed_t)buffer.length())
		cursor = (signed_t)buffer.length();

}


/*virtual*/ bool InputView::on_key(int vk, bool down)
{
	bool handled = false;
    signed_t keepcursor = cursor;
    bool invld = false, tch = false;
    switch (vk)
    {
	case 'C':
		if (down && ((GetAsyncKeyState(VK_CONTROL) & 0x8000) == 0x8000))
		{
			copy();
			handled = true;
		}
		break;
	case 'V':
		if (down && ((GetAsyncKeyState(VK_CONTROL) & 0x8000) == 0x8000))
		{
			paste();
			handled = true;
		}
		break;
	case VK_INSERT:
        if (down)
        {
			if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) == 0x8000)
			{
				copy();
				break;
			}
			else if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) == 0x8000)
			{
				paste();
				break;
			}

            insert_mode = !insert_mode;
            invld = true;
        }
        break;
    case VK_RIGHT:
        if (down)
        {
            bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) == 0x8000;
            bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) == 0x8000;

            if (shift && selected < 0)
                selected = cursor;

            if (!shift)
            {
                if (selected >= 0 && selected > cursor)
                    cursor = selected - 1;
                if (selected >= 0)
                    invld = true;
                selected = -1;
            }
            move_cursor(1,ctrl);
        }
        break;
    case VK_LEFT:
        if (down)
        {
            bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) == 0x8000;
            bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) == 0x8000;

            if (shift && selected < 0)
                selected = cursor;

            if (!shift)
            {
                if (selected >= 0 && selected < cursor)
                    cursor = selected + 1;

                if (selected >= 0)
                    invld = true;
                selected = -1;
            }
			move_cursor(-1, ctrl);
        }
        break;
    case VK_HOME:
        if (down)
        {
            if (cursor > 0)
            {
                bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) == 0x8000;
                if (selected < 0 && shift)
                    selected = cursor;
                if (!shift)
                    selected = -1;
                cursor = 0;
			}
			else if (selected >= 0)
			{
				invld = true;
				selected = -1;
			}
        }
        break;
    case VK_END:
        if (down)
        {
            if (cursor < (signed_t)buffer.length())
            {
                bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) == 0x8000;
                if (selected < 0 && shift)
                    selected = cursor;
                if (!shift)
                    selected = -1;
                cursor = (signed_t)buffer.length();
			}
			else if (selected >= 0) {
				invld = true;
				selected = -1;
			}

        }
        break;
    case VK_DELETE:
        if (down)
        {
            if (selected >= 0)
            {
				if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) == 0x8000)
					copy();

				signed_t r0 = min(cursor, selected);
				signed_t r1 = max(cursor, selected);
                buffer.erase(r0, r1-r0);
                tch = true;
                selected = -1;
                cursor = r0;
            }
            else if (cursor < (signed_t)buffer.length())
            {
                buffer.erase(buffer.begin() + cursor);
                tch = true;
            }
        }
        break;
	case VK_BACK:

		if (down)
		{
			if (selected >= 0)
			{
				signed_t r0 = min(cursor, selected);
				signed_t r1 = max(cursor, selected);
				buffer.erase(r0, r1 - r0);
				selected = -1;
				cursor = r0;
				tch = true;
				invld = true;
				handled = true;

			}
			else if (cursor > 0)
			{
				buffer.erase(buffer.begin() + (--cursor));
				tch = true;
				invld = true;
				handled = true;
			}
		}

		break;
    case VK_RETURN:
        if (down)
            on_key_enter();
        break;
    default:
        break;
    }
    if (tch)
    {
        on_text_changed();
        invld = true;
    }
    if (invld || keepcursor != cursor)
    {
        start_cursor_animation_cycle();
        invalidate();
    }
	return handled;
}

/*virtual*/ void InputView::on_char(wchar_t c, bool batch)
{
    if (c == 0)
    {
		on_text_changed();
		start_cursor_animation_cycle();
		invalidate();
        return;
    }

	c = char_to_view(c);

    if (c != 0)
    {
        bool replace = insert_mode;
        if (selected >= 0)
        {
			signed_t r0 = min(cursor, selected);
			signed_t r1 = max(cursor, selected);
            buffer.erase(r0, r1 - r0);
            selected = -1;
            cursor = r0;
            replace = false;
        }
        if (replace)
        {
            buffer[cursor++] = c;
        }
        else
        {
            buffer.insert(buffer.begin() + cursor++, c);
        }

        if (!batch)
		{
			on_text_changed();
			start_cursor_animation_cycle();
			invalidate();
        }
	}
}

void InputView::start_cursor_animation_cycle()
{

    cacyclestart = timeGetTime();
    cursor_alpha = 255;

	//text_x = PADDING;
	int cx = szperchar[max(selected,cursor)] - 1;
    int cxm = szperchar[buffer.length()] - 1;

    if (cxm < canvas_w - PADDING)
    {
        text_x = PADDING;
        return;
    }

    if (cx + text_x > canvas_w - PADDING)
    {
        text_x = canvas_w - PADDING - cx;
    }
    else if (cx + text_x < PADDING)
    {
        text_x = PADDING - cx;
    }

}

void InputView::animation_tick()
{
    DWORD ct = timeGetTime();
    int d = (int)(ct - cacyclestart);
    int nca = d < CURSOR_TIME_FULL ? 255 : (math::lerp_int(255, 0, (float)(d - CURSOR_TIME_FULL) * (1.0f / CURSOR_TIME_FADEOUT)));
    if (nca > 255) nca = 255;
    if (nca < 0 || !is_active()) nca = 0;
    if (nca != cursor_alpha)
    {
        cursor_alpha = nca;
        invalidate();
    }
	if (nca == 0 && !is_active())
		stop_animation();
    else if (d > (CURSOR_TIME_FULL+CURSOR_TIME_FADEOUT))
        start_cursor_animation_cycle();
}

void InputView::set_cursor_pos(size_t pos)
{
	selected = -1;
    if (pos > buffer.length())
        pos = buffer.length();
	prevcursor = pos;
	cursor = prevcursor;
    start_cursor_animation_cycle();
}

void InputView::select_all()
{
	selected = 0;
    prevcursor = buffer.length();
	cursor = prevcursor;
	start_cursor_animation_cycle();
}

void InputView::set_text(const std::wstring_view &t, bool selectall)
{
	std::wstring b(t);
	bool expand_text = b.length() >= prevbuffer.length() && memcmp(b.c_str(), prevbuffer.c_str(), sizeof(wchar_t) * prevbuffer.length()) == 0;
	prevbuffer = b;
	buffer = b;
	if (selectall)
	{
		selected = 0;
		prevcursor = t.size();
		cursor = prevcursor;
	}
	else
	{
		if (!expand_text)
		{
			selected = -1;
			if (cursor > (signed_t)b.length())
				cursor = b.length();
			prevcursor = cursor;
		}
	}
	invalidate();
	on_text_changed();
	start_cursor_animation_cycle();

	if (!selectall || !is_active())
		cursor_alpha = 0;
}


/*virtual*/ void InputView::activate()
{
	super::activate();

	if (textsize.y == 0)
		on_text_changed();

	run_animation();
	invalidate();
}

void InputView::update_cursor_by_mouse(const int2 &mp)
{
    int md = math::maximum<int>::value;
    int fx = -1;
    for (int i = 0, c = (int)szperchar.size(); i < c; ++i)
    {
        int cx = text_x + szperchar[i];
        int delta = math::abs(cx - mp.x);
        if (delta < md)
        {
            fx = i;
            md = delta;
        }
    }
    if (fx >= 0)
    {
        if (selected < 0)
        {
            selected = fx;
            selected_by_mouse = true;
        }
        else if (!selected_by_mouse)
        {
            //int r0 = min(cursor, selected);
            //int r1 = max(cursor, selected);
            // TODO: check click on selected
            selected = fx;
            selected_by_mouse = true;


        }
        if (cursor != fx)
        {
            cursor = fx;
			prevcursor = fx;
            start_cursor_animation_cycle();
            invalidate();
        }
    }
}

/*virtual*/ void InputView::on_mm(const int2& mp)
{
    if (selected_by_mouse)
        update_cursor_by_mouse(mp);

    super::on_mm(mp);
}

/*virtual*/ bool InputView::on_mout(const int2& mp)
{
    if (selected_by_mouse)
    {
        update_cursor_by_mouse(mp);
        return false;
    }

    if (selected_by_mouse)
    {
        selected_by_mouse = false;
        if (cursor == selected)
            selected = -1;
        invalidate();
    }

    super::on_mout(mp);
    return true;
}

/*virtual*/ void InputView::on_lbm(const int2& p, bool down)
{
    if (down)
    {
		activate();
        update_cursor_by_mouse(p);
    }
    else
    {
        if (selected_by_mouse)
        {
            selected_by_mouse = false;
            if (cursor == selected)
                selected = -1;
            invalidate();
        }

    }
}

void CalculatorInputView::colorize_ramka(errset e)
{
    calculated = false;
	switch (e)
	{
    case errset::OK:
    case errset::EMPTY:
    case errset::INF:
        calculated = true;
		set_ramka_color(0xff008000);
		break;
    case errset::CALCULATING:
		set_ramka_color(0xff0000ff);
		break;
	default:
		set_ramka_color(0xFFFF0000);
		break;
	}
}

/*virtual*/ void CalculatorInputView::on_text_changed()
{
    calculated = false;
	if (!quiet)
		colorize_ramka(mainview->calc_show_answer(buffer));
	super::on_text_changed();
}

static size_t find_bracket_pair(const std::wstring_view& s, size_t i)
{
    if (s[i] != '(')
        return s.npos;
    signed_t c = 1;
    ++i;
    for (size_t cnt = s.length();i<cnt;++i)
    {
        wchar_t x = s[i];
        if (x == '(')
        {
            ++c;
            continue;
        }
        if (x == ')')
        {
            --c;
            if (c == 0)
                return i;
        }
    }
    return s.npos;
}

/*virtual*/ void CalculatorInputView::on_key_enter()
{
    if (calculated && buffer.length() > 0)
    {
        if (find_bracket_pair(buffer, 0) != buffer.length() - 1)
        {
            buffer.insert(buffer.begin(), '(');
            buffer.push_back(')');
            on_text_changed();
            set_cursor_pos(buffer.length());
        }
        else {
            if (!is_cursor_at_end())
            {
                set_cursor_pos(buffer.length());
            }
            else
            {
                value r = mainview->lastresult;
                if (r.error() == errset::OK)
                {
                    r.clamp_frac(10);
                    buffer = r.to_string(10,20);
					on_text_changed();
					select_all();
                }
            }
        }
    }
}


void ResultView::colorize_text(errset e)
{
	switch (e)
	{
	case errset::OK:
	case errset::EMPTY:
    case errset::INF:
		set_text_color(0xff000000);
		break;
	case errset::CALCULATING:
		set_text_color(0xff0000ff);
		break;
	default:
		set_text_color(0xFFFF0000);
		break;
	}
}

ResultView::ResultView(signed_t fmt_id) :InputView(true), result_fmt_id(fmt_id)
{
}

ResultView::~ResultView()
{
    result = nullptr;
}

/*virtual*/ void ResultView::on_stop()
{
}

void ResultView::set_result_expression(ptr::shared_ptr<calculating_value> rslt)
{
    if (!rslt)
    {
        result = nullptr;
        return;
    }

    for (ResultFormat& r : cfg.get_results())
    {
        if (r.active && r.fmt_id == result_fmt_id)
        {
			result = rslt;

            if (!r.epreparsed.empty())
            {
				r.calcmachine->set_var(cfg.c_result, result);
                result = r.calcmachine->start_eval(r.epreparsed, (r.precision+1) / 2);
            }
            showradix = r.radix;
            showprec = r.precision;
            break;
        }
    }
}

static void addprogress(std::wstring& nvs)
{
	static std::wstring_view mm = L"+=\u00d7\u00f7\u2260\u00b1";
	signed_t x = (rand() % mm.length());
	nvs.push_back(mm[x]);
}

bool ResultView::show()
{
    if (!result)
    {
        set_text_color(0xff808080);
        return false;
    }

    std::wstring nvs;
    errset e;
    bool neednextcall = false;

	if (result->is_final_result_ready() || result->get_value().error() == errset::EMPTY)
	{
		value r = result->get_value();
		result = nullptr;
        e = r.error();

        if (e == errset::OK)
        {
            if (showprec >= 0)
                r.round((showprec+1)/2);
            nvs = r.to_string(showradix, showprec);
        }
        else if (e == errset::INF)
        {
            if (!is_hexview())
            {
                if (r.is_negative())
                    nvs.push_back(L'\u2212');
                nvs.append(TRS("Infinity"));
            }
		} else
        {
			nvs = error_text(e);
        }
	}
	else
	{
        neednextcall = true;
		if (result->is_intermedate_result_ready())
		{
			value val = result->get_value();
			e = val.error();
            nvs = val.to_string(showradix, showprec);
            addprogress(nvs);
		}
		else
		{
			e = errset::CALCULATING;
            addprogress(nvs);
		}
	}

	colorize_text(e);
    set_text(nvs, false);

    return neednextcall;
}


///*virtual*/ void ResultView::on_event(signed_t eventid)
//{
    //if (eventid == EVT_END_OF_CALC)
    //{
      //  set_text(showstr, false);
    //}
//}
