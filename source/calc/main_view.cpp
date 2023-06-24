#include "pch.h"

#if 0
DWORD WINAPI MainView::systicker(void * mw)
{
    ((MainView *)mw)->ticker();
    return 0;
}

void MainView::ticker()
{
    if (ticker_stop)
        return;

    ticker_works = true;
    ticker_starting = false;

    DWORD mainthreadid = GetWindowThreadProcessId(hwnd, nullptr);

    while (!ticker_stop)
    {
        DWORD ct = timeGetTime();
        int d = (int)(ct - lasttick);

        int actual_sleep = sleep;

        //if (is_sys_loop)
            //sleep = 100;

        if (d >= 10)
        {
            if (d > 1000)
                cnttick = 0;

            allow_tick = false;
            if (cnttick < 10)
            {
                ++cnttick;
                lasttick = ct;
                PostThreadMessageW(mainthreadid, WM_TICK, 0, 0);
            }
        } else if (!allow_tick)
            actual_sleep = 1;

        if (actual_sleep >= 0)
            Sleep(actual_sleep);
    }
    ticker_works = false;
}

void MainView::handle_tick()
{
    /*
    --cnttick;
    if (cnttick > 0)
    {
        MSG msgm;
        while (PeekMessage(&msgm, nullptr, WM_TICK, WM_TICK, PM_REMOVE))
            --cnttick;
    }
    */

    //input_view->animate();

    //lasttick = timeGetTime();
    //allow_tick = true;
}
#endif

MainView::MainView() :SystemWindowView(nullptr)
{
}

MainView::~MainView()
{
    //ticker_stop = true;

    /*
    if (!ticker_starting)
    {
        // wait ticker stop
        for (; ticker_works;)
        {
            Sleep(0);
        }
    }
    */

}

/*virtual*/ void MainView::created()
{
    //ticker_starting = true;
    //CloseHandle(CreateThread(nullptr, 0, systicker, this, 0, nullptr));

	const int hpadding = 10;
	const int ivh = 40;

	input_view = new CalculatorInputView();
	add_view(input_view, new Position(new AbsolutePVal(10), new AbsolutePVal(hpadding), new PaddingPVal(10), new AbsolutePVal(hpadding+ivh)));

	int y = hpadding + ivh + hpadding;
	for (ResultFormat& r : cfg.get_results())
	{
		if (!r.active)
			continue;

		ptr::shared_ptr<ResultView> iv = new ResultView(r.fmt_id);
		answers.push_back(iv);
		add_view(iv, new Position(new AbsolutePVal(10), new AbsolutePVal(y), new PaddingPVal(10), new AbsolutePVal(y + ivh)));
		y += ivh + hpadding;

		if (r.radix == 16)
			iv->set_hexview();

		iv->set_placeholder(r.placeholder);
	}

	int new_main_window_height = y + ivh + 5;
	set_height(new_main_window_height);

	//answer = new InputView(true);
	//add_view(answer, new Position(new AbsolutePVal(10), new AbsolutePVal(60), new PaddingPVal(10), new AbsolutePVal(100)));

    //answer2 = new InputView(true);
    //add_view(answer2, new Position(new AbsolutePVal(10), new AbsolutePVal(110), new PaddingPVal(10), new AbsolutePVal(150)));



	//input_view->set_text(L"exp (2 \u00d7 ln 2)", true);
	//input_view->set_text(L"sqrt (7)^2", true);
    //input_view->set_text(L"1231\u00f73", true);
	//input_view->set_text(L"1\u00f713.234", true);
	//input_view->set_text(L"1\u00f70.013234", true);
	//input_view->set_text(L"1\u00f716.0", true);
	//input_view->set_text(L"1\u00f71234567.0", true);
	//input_view->set_text(L"exp 22", true);
    //input_view->set_text(L"2\u00d7exp 22", true); // mul
	//input_view->set_text(L"e^8.00000001\u2212exp 8.00000001", true);
	//input_view->on_char('3');
	//input_view->on_char('*');
	//input_view->on_char('-');
	//input_view->on_char('3');

}

/*virtual*/ void MainView::draw(HDC dc)
{
    RECT r;
    GetClientRect(hwnd, &r);
    FillRect(dc, &r, (HBRUSH)(16));
}

/*virtual*/ void MainView::on_destroy()
{
	cfg.save();
    super::on_destroy();
    PostQuitMessage(0);
}

/*virtual*/ bool MainView::on_key(int vk, bool down)
{
    // route any keys to InputView
	bool handled = false;
	if (input_view)
	{
		handled = input_view->on_key(vk, down);
		input_view->activate();
	}
	return handled;
}

/*virtual*/ void MainView::on_char(wchar_t c, bool batch)
{
    // route any chars to InputView
	if (input_view)
	{
		input_view->on_char(c, batch);
		if (!batch)
			input_view->activate();
	}
}

/*virtual*/ void MainView::on_poschange(const int4& newposnsize)
{
	super::on_poschange(newposnsize);
}

/*virtual*/ void MainView::activate()
{
	if (input_view)
		input_view->activate();
}

/*virtual*/ void MainView::animation_tick()
{
	DWORD ct = timeGetTime();
	if (viewrefresh != 0 && (ct - viewrefresh) < 200)
		return;
	viewrefresh = ct;

	input_view->colorize_ramka(show_answer());

	bool na = false;
	for (ResultView* view : answers)
		na |= view->show();
	if (result == nullptr && !na)
		stop_animation();
}

errset MainView::show_answer()
{
	errset e = errset::OK;
	if (result != nullptr)
	{

		if (result->is_final_result_ready() || result->get_value().error() == errset::EMPTY)
		{
			value val = result->get_value();
			result = nullptr;
			e = val.error();
			lastresult = val.round(50);
		}
		else
		{
			if (result->is_intermedate_result_ready())
			{
				value val = result->get_value();
				e = val.error();
				lastresult = val.round(50);
			}
			else
			{
				e = errset::CALCULATING;
				lastresult = value(errset::CALCULATING);
			}
		}
		run_animation();
	}

	return e;

}

errset MainView::calc_show_answer(const std::wstring_view &buf)
{
	errset e = errset::OK;
	if (answers.size() > 0)
	{
		bool empt = false;
		if (buf.empty())
		{
			result = new calculating_value(errset::EMPTY);
			empt = true;
		}
		else
		{
#ifdef LOGGER
			lg.reset();
#endif

			result = parsed.evaluate(buf, cfg.get_precision() / 2 + 1, true);
		}

		for (ResultView* view : answers)
			view->set_result_expression(empt ? nullptr : result);


		e = show_answer();
	}
	return e;
}