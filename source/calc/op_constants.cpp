#include "pch.h"

std::mutex pimutex;
value picache(errset::EMPTY);

std::mutex emutex;
value ecache(errset::EMPTY);

value op_pi::calc_pi(signed_t precision)
{
	// may be in cache?
	{
		std::unique_lock<std::mutex> m(pimutex);
		if (picache.error() != errset::EMPTY && picache.get_precision() >= precision)
			return value(picache);
	}


	// https://en.wikipedia.org/wiki/Gauss%E2%80%93Legendre_algorithm

	value a(1, 0); // 1
	value b(2, 0);
	value tmp = op_sqrt::calc_sqrt(b, precision + 20);
	tmp.calc_inverse(b, precision + 20); // b <- 1/sqrt(2)
	value t(0, 25); // 0.25
	value p(1, 0);
	value two(2, 0);
	value four(4, 0);
	value half(0, 50);

	signed_t eqd = math::minimum<signed_t>::value;

	for (;;)
	{
		value an = (a + b) * half; // an <- (a+b)/2;
		value bn = op_sqrt::calc_sqrt(a * b, precision + 20);
		value ann = a - an;
		value tn = t - p * ann * ann;
		value pn = p * two;

		an.clamp_frac(precision + 5);
		bn.clamp_frac(precision + 5);
		tn.clamp_frac(precision + 5);
		pn.clamp_frac(precision + 5);

		a = an;
		b = bn;
		t = tn;
		p = pn;

		eqd = a.compare_tail(b, eqd);
		if (eqd >= precision)
			break;
	}

	value ab = a + b;
	value otvet;
	p = four * t;
	a = ab * ab;
	a.calc_div(otvet, p, precision * 2);
	
	{
		std::unique_lock<std::mutex> m(pimutex);
		if (picache.error() == errset::EMPTY || picache.get_precision() < precision)
			picache = otvet;
	}

	return otvet;
}

/*virtual*/ calc_result_t op_pi::calc(const std::vector<value>& /*calculated_params*/, signed_t precision, context* /*ctx*/) const
{
	return { calc_pi(precision), true };
}

value op_e::calc_e(signed_t precision)
{
	// may be in cache?
	{
		std::unique_lock<std::mutex> m(emutex);
		if (ecache.error() != errset::EMPTY && ecache.get_precision() >= precision)
			return value(ecache);
	}

	value d(24, 0);
	value f(5, 0);
	value acc(2, 66, precision + 1);
	value pacc;
	for (;;)
	{
		value s;
		d.calc_inverse(s, precision + 20);
		acc = acc + s;
		acc.clamp_frac(precision + 1);

		signed_t eqv = pacc.compare_tail(acc);
		if (eqv >= precision)
			break;

		d = d * f;
		f.add(1);
		pacc = acc;
	}

	{
		std::unique_lock<std::mutex> m(emutex);
		if (ecache.error() == errset::EMPTY || ecache.get_precision() < precision)
			ecache = acc;
	}

	return acc;
}

/*virtual*/ calc_result_t op_e::calc(const std::vector<value>& /*calculated_params*/, signed_t precision, context* /*ctx*/) const
{
	return { calc_e(precision), true };
}

