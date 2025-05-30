#include "pch.h"


void do_some_tests()
{
	
	if (true) return;

	std::vector<usingle> mm, mm2;
	for (int i = 0; i < 4; ++i)
	{
		//mm.push_back( rand() * rand() * rand() * rand());
	}

	mm.push_back(1);
	mm.push_back(0);
	value::neg(mm);


	value x, y, z;

	x.set_unsigned(mm);
	x.to_unsigned(mm2);




	//x.set_unsigned(257653);
	//y.set_unsigned(232); //y.set_unsigned(262);

	//x.set_unsigned(65536);
	//y.set_unsigned(256);
	//x = value(6, 55); x.append_frac(36);
	//y = value(2, 56);

	//x.calc_div_impl(z, y, 100);



	x.set_unsigned(10000000000);
	x = x * x;
	//x.div_by_2_int();
	//x.calc_div(z, 2, 100);
	//x.calc_div_int(z, 2);




	x.set_unsigned(1);
	y.set_unsigned(132);

	signed_t st = timeGetTime();
	for (int i = 0; i < 5000; ++i)
	{
		x.calc_div_impl(z,y,102);
		//x = x + z;
		value::aline_exponent(y, z);
		y = y + z;
	}
	signed_t et = timeGetTime();
	signed_t t = et - st;

	value r1 = y;

	x.set_unsigned(1);
	y.set_unsigned(132);
	z.set_unsigned(0);

	st = timeGetTime();
	for (int i = 0; i < 5000; ++i)
	{
		y.calc_inverse(z, 106);
		value::aline_exponent(y, z);
		y = y + z;
		//y.clamp_frac(101);

		//value::aline_exponent(x, z);

		//x = x + z;
		//y = y + z;
	}
	et = timeGetTime();
	signed_t t2 = et - st;

	value r2 = y;
	
	MessageBoxW(nullptr, (std::to_wstring(t) + L"/"+ std::to_wstring(t2)).c_str(), L"test", MB_OK);
}