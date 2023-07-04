#include "pch.h"


void do_some_tests()
{
	
	
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


	if (true) return;


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




	x.set_unsigned(1000);
	y.set_unsigned(132);

	signed_t st = timeGetTime();
	for (int i = 0; i < 5000; ++i)
	{
		x.calc_div_impl(z,y,102);
		x = x + z;
		y = y + z;
	}
	signed_t et = timeGetTime();
	signed_t t = et - st;

	value r1 = z;

	x.set_unsigned(1000);
	y.set_unsigned(132);

	st = timeGetTime();
	for (int i = 0; i < 5000; ++i)
	{
		y.calc_inverse(z, 106);
		z = x * z;
		z.clamp_frac(101);

		value::aline_exponent(x, z);

		x = x + z;
		y = y + z;
	}
	et = timeGetTime();
	signed_t t2 = et - st;

	value r2 = z;
	
	MessageBoxW(nullptr, (std::to_wstring(t) + L"/"+ std::to_wstring(t2)).c_str(), L"test", MB_OK);
}