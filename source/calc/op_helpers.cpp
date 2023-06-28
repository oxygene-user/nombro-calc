#include "pch.h"


/*virtual*/ calc_result_t op_int_c::calc(const std::vector<value>& calculated_params, signed_t /*precision*/, context* /*ctx*/) const
{
	ASSERT(calculated_params[0].get_exponent() == 0);
	return { calculated_params[0].clone_int(), true };
}

/*virtual*/ calc_result_t op_frac_c::calc(const std::vector<value>& calculated_params, signed_t /*precision*/, context* /*ctx*/) const
{
	ASSERT(calculated_params[0].get_exponent() == 0);
	return { calculated_params[0].clone_frac(), true };
}


/*virtual*/ calc_result_t op_anorm_c::calc(const std::vector<value>& calculated_params, signed_t precision, context* /*ctx*/) const
{
	// normalize angle
	value angle = calculated_params[0];
	if (angle.is_infinity())
	{
		return { value(errset::BAD_ARGUMENT), true  };
	}
	angle.make_zero_exponent();
	value pi = op_pi_c::calc_pi(precision);
	value pi2 = pi + pi;

	value x = op_mod_c::calc_mod(angle + pi, pi2);
	if (x.is_negative())
		x = x + pi;
	else
		x = x - pi;
	x.clamp_frac(precision);
	return { x, true };
}


/*virtual*/ calc_result_t op_E_c::calc(const std::vector<value>& calculated_params, signed_t precision, context* /*ctx*/) const
{
	if (!calculated_params[1].is_zero_frac())
	{
		return { value(errset::BAD_SYMBOLS_IN_EXPRESION), true };
	}

	value exp10 = calculated_params[1];

	usingle exp;
	if (!exp10.to_unsigned(exp) || (exp / 2 + 1) > math::maximum<i32>::value)
		return { value(errset::XOVERFLOW), true };

	i32 expo = 0;
	bool one10 = 0 != (exp & 1);
	expo = (i32)(exp >> 1);
	if (exp10.is_negative())
		expo = -expo;

	value r;
	if (one10)
	{
		r = calculated_params[0] * (exp10.is_negative() ? value(0, 10) : value(10, 0));
	}
	else
		r = calculated_params[0];
	r.set_exponent(r.get_exponent() + expo);

	return { r.clamp_frac(precision), true };
}

/*virtual*/ calc_result_t op_Q_c::calc(const std::vector<value>& calculated_params, signed_t /*precision*/, context* /*ctx*/) const
{
	value v(calculated_params[0]);
	v.make_zero_exponent();
	return { v, true };
}