#include "pch.h"

/*virtual*/ calc_result_t op_plus_c::calc(const std::vector<value>& calculated_params, signed_t precision, context* /*ctx*/) const
{
	if (calculated_params[0].is_infinity() && calculated_params[1].is_infinity())
	{
		bool neg0 = calculated_params[0].is_negative();
		if (neg0 != calculated_params[1].is_negative())
			return { value(errset::BAD_ARGUMENT), true };

		return { value(errset::INF, neg0), true };
	}

	if (calculated_params[0].is_infinity())
		return { calculated_params[0], true };

	if (calculated_params[1].is_infinity())
		return { calculated_params[1], true };


	return { (calculated_params[0] + calculated_params[1]).clamp_frac(precision), true };
}

/*virtual*/ calc_result_t op_minus_c::calc(const std::vector<value>& calculated_params, signed_t precision, context* /*ctx*/) const
{
	if (calculated_params[0].is_infinity() && calculated_params[1].is_infinity())
	{
		bool neg0 = calculated_params[0].is_negative();
		if (neg0 == calculated_params[1].is_negative())
			return { value(errset::BAD_ARGUMENT), true };

		return { value(errset::INF, neg0), true };
	}


	if (calculated_params[0].is_infinity())
		return { calculated_params[0], true };

	if (calculated_params[1].is_infinity())
		return { value(errset::INF, !calculated_params[1].is_negative()), true };

	return { (calculated_params[0] - calculated_params[1]).clamp_frac(precision), true };
}

/*virtual*/ calc_result_t op_neg_c::calc(const std::vector<value>& calculated_params, signed_t precision, context* /*ctx*/) const
{
	if (calculated_params[0].is_infinity())
		return { value(errset::INF, !calculated_params[0].is_negative()), true };

	value rv(calculated_params[0]);
	return { rv.minus().clamp_frac(precision), true };

}


/*virtual*/ calc_result_t op_mul_c::calc(const std::vector<value>& calculated_params, signed_t precision, context* /*ctx*/) const
{

	if (calculated_params[0].is_infinity() || calculated_params[1].is_infinity())
	{
		if (calculated_params[0].is_zero() || calculated_params[1].is_zero())
			return { value(errset::BAD_ARGUMENT), true };

		bool same_sign = calculated_params[0].is_negative() == calculated_params[1].is_negative();
		return { value(errset::INF, !same_sign), true };
	}

	bool fna = false;
	signed_t prc1 = calculated_params[0].get_precision();
	ASSERT(prc1 != value::P_UNDEFINED);
	if (prc1 == value::P_ABSOLUTE) prc1 = calculated_params[0].frac_size();
	else fna = true;

	signed_t prc2 = calculated_params[1].get_precision();
	ASSERT(prc2 != value::P_UNDEFINED);
	if (prc2 == value::P_ABSOLUTE) prc2 = calculated_params[1].frac_size();
	else fna = true;

	signed_t prec = prc1 + prc2;
	if (prec > precision)
	{
		//prec = precision;
		fna = true;
	}

	value res = calculated_params[0] * calculated_params[1];
	res.clamp_frac(prec, fna);

	ASSERT(res.get_precision() != value::P_UNDEFINED);

	return { res, true };
}


/*virtual*/ calc_result_t op_div_c::calc(const std::vector<value>& calculated_params, signed_t precision, context* /*ctx*/) const
{
	if (calculated_params[0].is_infinity())
	{
		if (calculated_params[1].is_infinity())
			return { value(errset::BAD_ARGUMENT), true };

		bool same_sign = calculated_params[0].is_negative() == calculated_params[1].is_negative();
		return { value(errset::INF, !same_sign), true };
	}

	if (calculated_params[1].is_infinity())
	{
		return { value(), true }; // return zero
	}

	if (calculated_params[1].is_zero())
	{
		if (calculated_params[0].is_zero())
			return { value(errset::BAD_ARGUMENT), true };

		return { value(errset::INF, calculated_params[0].is_negative()), true };
	}


	value rv;
	calculated_params[0].calc_div(rv, calculated_params[1], precision * 2);
	//rv.round(precision);
	return { rv, true };
}


value op_mod_c::calc_mod(const value& v, const value& m)
{
	value rem;
	v.calc_div(rem, m, 1);
	if (v.is_negative())
		rem.set_negative(true);
	rem.clamp_frac(0);
	return v - (rem * m);
}

/*virtual*/ calc_result_t op_mod_c::calc(const std::vector<value>& calculated_params, signed_t /*precision*/, context* /*ctx*/) const
{
	if (calculated_params[0].is_infinity() || calculated_params[1].is_infinity() || calculated_params[1].is_negative() || calculated_params[1].is_zero())
	{
		return { value(errset::BAD_ARGUMENT), true };
	}


	return { calc_mod(calculated_params[0], calculated_params[1]), true };
}
