#pragma once

#define CFGPARS \
    CP( int, wposx, 100, "// window X pos" ) \
    CP( int, wposy, 100, "// window Y pos" ) \
    CP( int, wwidth, 800, "// window width" ) \
    CP( int, wheight, 600, "// window height" ) \
    CP( bool, wmax, false, " // window is max" ) \
    CP( string, savepath, L".", "// save config path (. means this config)" ) \
    CP( int, precision, 100, " // calculation precision (in digits)" ) \
    CP( int, maxnum, 10000, " // maximum digits of integer part of answer" ) \
    CP( int, calctimeout, 2000, " // calculation timeout" ) \


#define RFS \
    RFP(active, true) \
    RFP(options, 0) \
    RFP(id, nullptr) \
	RFP(expression, nullptr) \
	RFP(radix, 10) \
	RFP(precision, 100) \
    RFP(placeholder, nullptr) \



#define TRS(s) WSTR(s)

namespace helpers
{
    template<typename VT> struct readp;
    template<> struct readp<std::wstring>
    {
        static std::wstring read(const wsts& s, const std::wstring_view &pn, void*)
        {
            return s.get_string(pn);
        }
    };
	template<> struct readp<signed_t>
	{
		static signed_t read(const wsts& s, const std::wstring_view& pn, signed_t def)
		{
			return s.get_int(pn, def);
		}
	};
	template<> struct readp<bool>
	{
		static bool read(const wsts& s, const std::wstring_view& pn, signed_t def)
		{
			return s.get_int(pn, def) != 0;
		}
	};
}

#include "resultformat.h"

class Config
{
    typedef std::wstring string;

#define CP( t, field, dv, c ) t field = dv;
    CFGPARS
#undef CP

    std::vector<ResultFormat> results;

    int beforesave = 0;

    enum
    {
        o_save_if_not_exist = 1,
        o_secondary = 2,
    };

    void load(const std::wstring& fn, usingle options);
    void fix();

public:

    std::wstring c_empty;
    std::wstring c_result = std::wstring(WSTR("result"));
    std::wstring c_default = std::wstring(WSTR("default"));
    std::wstring c_id = std::wstring(WSTR("id"));

    Config();
    ~Config();

    void load();
    void tick();
	void save();
    void ondie();

    std::vector<ResultFormat>& get_results() { return results; }

    void lock_results() {}
    void unlock_results() {}

#define CP( t, field, dv, c ) t get_##field() const { return field; }
	CFGPARS
#undef CP

#define CP( t, field, dv, c ) void set_##field(t val) { if (field != val) { field = val; beforesave = 2; } }
	CFGPARS
#undef CP

};


extern Config cfg;