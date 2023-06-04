#include "pch.h"
#include "str_helpers.h"

signed_t ResultFormat::next_id()
{
	static signed_t pool = 1;
	return pool++;
}

ResultFormat::ResultFormat() :id(cfg.c_default)
{
	fmt_id = next_id();
}

ResultFormat::ResultFormat(const wsts& s)
{
	fmt_id = next_id();
	load(s);
}

ResultFormat::~ResultFormat()
{
	if (calcmachine)
	{
		calcmachine->stop_eval();
		calcmachine = nullptr;
	}
}

void ResultFormat::load(const wsts& s)
{
#define RFP(nam, def) nam = helpers::readp<decltype(nam)>::read(s, __STR1W__(nam), def);
	RFS
#undef RFP

	if (radix != 10)
		precision = 0; // fractional part of numbers supported only for decimal values

	s.get_comments([this](const std::wstring& n, std::wstring& c) {
		comments[n] = c;
	});

	if (!expression.empty())
	{
		epreparsed = expression;
		replace_all(epreparsed, L'*', L'\u00d7');
		replace_all(epreparsed, L'/', L'\u00f7');
		replace_all(epreparsed, L'-', L'\u2212');
		
		
		calcmachine.reset(new calc_machine());
	}
}



void ResultFormat::save(wsts& b)
{
#define RFP(nam, def) { auto &xx = b.set(WSTR(__STR1__(nam))).set_value(nam); if (auto *cmnt = getcmnt(WSTR(__STR1__(nam)))) xx.add_comment(*cmnt); }
	RFS
#undef RFP
}


