#include "pch.h"

static void upsize(size_t rqsz, std::vector<usingle>& v)
{
	ASSERT(v.size() <= rqsz);
	size_t count = rqsz - v.size();
	if (count)
		v.insert(v.begin(), count, 0);
}

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

typedef std::vector<usingle> bar;
static std::tuple<bar*, const bar*> prepare(const std::vector<value>& calculated_params, bar& vv1, bar& vv2)
{
	value v1(calculated_params[0]); v1.make_zero_exponent();
	value v2(calculated_params[1]); v2.make_zero_exponent();

	v1.to_unsigned(vv1);
	v2.to_unsigned(vv2);

	std::vector<usingle>* r, * f;
	if (vv1.size() > vv2.size())
	{
		r = &vv1;
		f = &vv2;
	}
	else
	{
		r = &vv2;
		f = &vv1;
	}


	bool resz = false;
	if (v1.is_negative())
	{
		upsize(r->size(), *f);
		resz = value::neg(vv1);
	}

	if (v2.is_negative())
	{
		upsize(r->size(), *f);
		resz |= value::neg(vv2);
	}

	if (resz)
	{
		if (vv1.size() > vv2.size())
		{
			r = &vv1;
			f = &vv2;
		}
		else
		{
			r = &vv2;
			f = &vv1;
		}
	}

	return { r, f };

}

/*virtual*/ calc_result_t op_and_c::calc(const std::vector<value>& calculated_params, signed_t /*precision*/, context* /*ctx*/) const
{
	bar vv1, vv2;
	auto pr = prepare(calculated_params, vv1, vv2);

	value res;
	for (signed_t i = 0, cnt = std::get<1>(pr)->size(); i < cnt; ++i)
	{
		usingle orv = (*std::get<0>(pr))[i] & (*std::get<1>(pr))[i];
		(*std::get<0>(pr))[i] = orv;
	}
    for (signed_t i = std::get<1>(pr)->size(), cnt = std::get<0>(pr)->size(); i < cnt; ++i)
        (*std::get<0>(pr))[i] = 0;

	res.set_unsigned(*std::get<0>(pr));
	res.clamp_frac(0);
	return { res, true };
}

/*virtual*/ calc_result_t op_xor_c::calc(const std::vector<value>& calculated_params, signed_t /*precision*/, context* /*ctx*/) const
{
	bar vv1, vv2;
	auto pr = prepare(calculated_params, vv1, vv2);

	value res;
	for (signed_t i = 0, cnt = std::get<1>(pr)->size(); i < cnt; ++i)
	{
		usingle orv = (*std::get<0>(pr))[i] ^ (*std::get<1>(pr))[i];
		(*std::get<0>(pr))[i] = orv;
	}

	res.set_unsigned(*std::get<0>(pr));
	res.clamp_frac(0);
	return { res, true };
}


/*virtual*/ calc_result_t op_or_c::calc(const std::vector<value>& calculated_params, signed_t /*precision*/, context* /*ctx*/) const
{
	bar vv1, vv2;
	auto pr = prepare(calculated_params, vv1, vv2);

	value res;
	for (signed_t i = 0, cnt = std::get<1>(pr)->size(); i < cnt; ++i)
	{
		usingle orv = (*std::get<0>(pr))[i] | (*std::get<1>(pr))[i];
		(*std::get<0>(pr))[i] = orv;
	}

	res.set_unsigned(*std::get<0>(pr));
	res.clamp_frac(0);
	return { res, true };
}

