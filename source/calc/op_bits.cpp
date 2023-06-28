#include "pch.h"



/*virtual*/ calc_result_t op_shl_c::calc(const std::vector<value>& calculated_params, signed_t precision, context* /*ctx*/) const
{
	value r;
	value v2s(calculated_params[0]); v2s.make_zero_exponent();
	value sv(calculated_params[1]); sv.make_zero_exponent();

	if (sv.is_negative())
	{
		v2s.calc_shift_right(r, sv);
	}
	else
	{
		v2s.calc_shift_left(r, sv);
	}

	r.clamp_frac(precision);

	return { r, true };
}

/*virtual*/ calc_result_t op_shr_c::calc(const std::vector<value>& calculated_params, signed_t precision, context* /*ctx*/) const
{
	value r;
	value v2s(calculated_params[0]); v2s.make_zero_exponent();
	value sv(calculated_params[1]); sv.make_zero_exponent();
	if (sv.is_negative())
	{
		v2s.calc_shift_left(r, sv);
	}
	else
	{
		v2s.calc_shift_right(r, sv);
	}

	r.clamp_frac(precision);

	return { r, true };
}