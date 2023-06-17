#pragma once

class ResultView;
class CalculatorInputView;
class MainView : public SystemWindowView
{
    typedef SystemWindowView super;
    ptr::shared_ptr<CalculatorInputView> input_view;
    std::vector< ptr::shared_ptr<ResultView> > answers;
	ptr::shared_ptr<calculating_value> result;

    /*
    static DWORD WINAPI systicker(void *);
    void ticker(); // works in other tread

    std::atomic<DWORD> lasttick = 0;
    std::atomic<int> cnttick = 0;

    volatile bool allow_tick = true;
    volatile bool ticker_works = false;
    volatile bool ticker_starting = false;
    volatile bool ticker_stop = false;
    */

    DWORD viewrefresh = 0;

    errset show_answer();
	virtual void animation_tick() override;
public:

	value lastresult;
    etree parsed;

    /*
    std::atomic<int> sleep = 10;
    */

    MainView();
    virtual ~MainView();

    void handle_tick();

    bool create(int nCmdShow);
    /*virtual*/ void created() override;
    /*virtual*/ void draw(HDC dc) override;
    /*virtual*/ void on_destroy() override;
    /*virtual*/ bool on_key(int vk, bool down) override;
    /*virtual*/ void on_char(wchar_t c, bool batch) override;
    /*virtual*/ void on_poschange(const int4& newposnsize);

	/*virtual*/ void activate() override;

    errset calc_show_answer(const std::wstring_view &buf);

};

