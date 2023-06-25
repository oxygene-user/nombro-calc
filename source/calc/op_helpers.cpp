#include "pch.h"


/*virtual*/ calc_result_t op_int_c::calc(const std::vector<value>& calculated_params, signed_t /*precision*/, context* /*ctx*/) const
{
	return { calculated_params[0].clone_int(), true };
}

/*virtual*/ calc_result_t op_frac_c::calc(const std::vector<value>& calculated_params, signed_t /*precision*/, context* /*ctx*/) const
{
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
	value pi = op_pi_c::calc_pi(precision);
	value pi2;
	value::mul(pi2, pi, 2);

	value x = op_mod_c::calc_mod(angle + pi, pi2);
	if (x.is_negative())
		x = x + pi;
	else
		x = x - pi;
	x.clamp_frac(precision);
	return { x, true };
}
