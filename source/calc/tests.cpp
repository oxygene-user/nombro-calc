#include "pch.h"


void do_some_tests()
{
	if (true) return;

	value x; x.set_unsigned(1000);

	signed_t st = timeGetTime();
	for (int i = 0; i < 1000; ++i)
	{
		x = x + op_sqrt_c::calc_sqrt(x, 100);
		x.clamp_frac(100);
	}
	signed_t et = timeGetTime();
	signed_t t = et - st;
	
	MessageBoxW(nullptr, std::to_wstring(t).c_str(), L"test", MB_OK);
}