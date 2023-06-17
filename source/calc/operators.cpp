#include "pch.h"

volatile unsigned char op::calctag = 0;

//static op_sys_remove_bypass opsys_remove_bypass;


/*virtual*/ calc_result_t op_plus::calc(const std::vector<value>& calculated_params, signed_t precision, context* /*ctx*/) const
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

/*virtual*/ calc_result_t op_minus::calc(const std::vector<value> &calculated_params, signed_t precision, context * /*ctx*/) const
{
	if (calculated_params.size() == 1)
	{
		if (calculated_params[0].is_infinity())
			return { value(errset::INF, !calculated_params[0].is_negative()), true };

		value rv(calculated_params[0]);
		return { rv.minus().clamp_frac(precision), true };
	}

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
		return { value(errset::INF, !calculated_params[1].is_negative()), true};

	return { (calculated_params[0] - calculated_params[1]).clamp_frac(precision), true };
}


/*virtual*/ calc_result_t op_mul::calc(const std::vector<value> &calculated_params, signed_t precision, context * /*ctx*/) const
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
		prec = precision;
		fna = true;
	}
	return { (calculated_params[0] * calculated_params[1]).round(prec, fna), true };
}


/*virtual*/ calc_result_t op_div::calc(const std::vector<value> &calculated_params, signed_t precision, context * /*ctx*/) const
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
	calculated_params[0].calc_div(rv, calculated_params[1], precision + 10);
	rv.round(precision);
	return { rv, true };
}

/*virtual*/ calc_result_t op_pi::calc(const std::vector<value>& /*calculated_params*/, signed_t precision, context* /*ctx*/) const
{
	// https://en.wikipedia.org/wiki/Gauss%E2%80%93Legendre_algorithm

	value a(1,0); // 1
	value b(2,0);
	value tmp = op_sqrt::calc_sqrt(b, precision + 20);
	tmp.calc_inverse(b, precision + 20); // b <- 1/sqrt(2)
	value t(0, 25); // 0.25
	value p(1,0);
	value two(2,0);
	value four(4,0);
	value half(0, 50);

	for (;;)
	{
		value an = (a + b) * half; // an <- (a+b)/2;
		value bn = op_sqrt::calc_sqrt(a * b, precision + 20);
		value ann = a - an;
		value tn = t - p * ann * ann;
		value pn = p * two;

		an.clamp_frac(precision+5);
		bn.clamp_frac(precision + 5);
		tn.clamp_frac(precision + 5);
		pn.clamp_frac(precision + 5);

		a = an;
		b = bn;
		t = tn;
		p = pn;

		signed_t eqd = a.compare_tail(b);
		if (eqd>=precision)
			break;
	}

	value ab = a + b;
	value otvet;
	p = four * t;
	a = ab * ab;
	a.calc_div(otvet, p, precision);
	otvet.clamp_frac(precision);
	return { otvet, true };
}

value op_e::calc_e(signed_t precision)
{
	value d(24,0);
	value f(5,0);
	value acc(2, 66, precision+1);
	value pacc;
	for (;;)
	{
		value s;
		d.calc_inverse(s, precision + 20);
		acc = acc + s;
		acc.clamp_frac(precision+1);

		signed_t eqv = pacc.compare_tail(acc);
		if (eqv >= precision)
			break;

		d = d * f;
		f.add(1);
		pacc = acc;
	}
	return acc;
}

/*virtual*/ calc_result_t op_e::calc(const std::vector<value>& /*calculated_params*/, signed_t precision, context* /*ctx*/) const
{
	return { calc_e(precision), true};
}

value heron_sqrt(const value& a, signed_t precision)
{
	value half(0, 50);
	value x0, invx0;
	usingle z;
	if (a.to_unsigned(z))
	{
		u64 zz = math::dround(sqrt((double)z));
		x0.set_unsigned(zz + 1);
	}
	else
	{
		x0 = a * value(0, 10);
	}
	for (;;)
	{
		x0.calc_inverse(invx0, precision + 10);
		value x1 = ((invx0 * a) + x0) * half;

		if (x1.compare(x0, precision) == 0)
			break;

		//std::wstring s1 = x1.to_string();
		//std::wstring s2 = x0.to_string();

		x0 = x1;
		x0.clamp_frac(precision + 10);
	}
	return x0;
}

value bakhshali_sqrt(const value& x, signed_t precision)
{
	value half(0, 50);
	value x0, invb, invx;
	usingle z;
	if (x.to_unsigned(z))
	{
		u64 zz = math::dround(sqrt((double)z));
		x0.set_unsigned(zz + 1);
	}
	else
	{
		x0 = x * value(0, 10);
	}


	for (;;)
	{
		x0.calc_inverse(invx, precision + 10);
		value a = (x - x0 * x0) * half * invx;
		value b = x0 + a;
		b.calc_inverse(invb, precision + 10);
		value x1 = b - (a * a * half * invb);

		if (x1.compare(x0, precision) == 0)
			break;

		//std::wstring s1 = x1.to_string(10, 20);
		//std::wstring s2 = x0.to_string(10, 20);

		x0 = x1;
		x0.clamp_frac(precision + 10);
	}
	return x0;
}


value op_sqrt::calc_sqrt(const value& a, signed_t precision)
{
	return heron_sqrt(a, precision);
	//return bakhshali_sqrt(a, precision); // slow
}

/*virtual*/ calc_result_t op_sqrt::calc(const std::vector<value> &calculated_params, signed_t precision, context * /*ctx*/) const
{
	if (calculated_params[0].is_negative())
		return { value(errset::BAD_ARGUMENT), true };

	if (calculated_params[0].is_infinity())
		return { calculated_params[0], true };

	return { calc_sqrt(calculated_params[0], precision).round(precision), true };
}

/*virtual*/ calc_result_t op_exp::calc(const std::vector<value> &calculated_params, signed_t precision, context *ctx) const
{
	value x = calculated_params[0];

	if (x.is_infinity())
		return { x.is_negative() ? value() : x, true};

	if (x.is_zero())
		return { value(1,0), true };

	usingle ix;
	if (x.is_zero_frac() && x.to_unsigned(ix))
	{
		value e = op_e::calc_e(precision + 10);

		if (ix == 1)
			return { e, true };

		value rslt(1, 0);

		for (; ix != 0; ix >>= 1)
		{
			if (0 != (ix & 1))
			{
				rslt = rslt * e;
				rslt.clamp_frac(precision + 10);

				if (ix == 1)
					break;

			}
			e = e * e;
			e.clamp_frac(precision + 10);
		}

		return { rslt, true };
	}

	/*
	//size_t nn = 1ull << 32;
	value vnn; vnn.set_integer(1ull << 32);
	vnn = x * vnn.calc_inverse(60) + value(1,0);

	//vnn = value(3,0);

	for (int i = 0; i < 32; ++i)
	{
		vnn = vnn * vnn; // ^2
		vnn.clamp_frac(60);
	}

	//vnn = vnn * vnn; // ^4
	//vnn = vnn * vnn; // ^8
	//vnn = vnn * vnn; // ^16
	//vnn = vnn * vnn; // ^32
	//vnn = vnn * vnn; // ^64
	//vnn = vnn * vnn; // ^128
	//vnn = vnn * vnn; // ^256
	//vnn = vnn * vnn; // ^512

	value x0 = vnn;
	//std::wstring xxx = x0.to_string();

	//value x0 = vnn;


	//2 * 2 = 4
	//4 * 4 = 16
	//16 * 16 = 256
	//256 * 256 = 65536
	//65536 * 65536 = 4294967296
	*/

	signed_t maxprecision = x.get_precision();
	if (value::P_ABSOLUTE == maxprecision)
		maxprecision = MAX_PRECISION;

	exp_context *ectx = (exp_context *)ctx;
	ectx->check_reset(x, precision);

    //value t = ectx->an;

	for (;; ++ectx->n)
	{
		if (ectx->calctag != op::calctag)
			return { value(), true };

		ectx->s = ectx->s + ectx->an;

        //t = t * ectx->x * get(ectx->n, precision + 10);

        //ectx->an = ectx->an * ectx->x * get(ectx->n, precision+10);
        //ectx->an.clamp_frac(precision + 10);
        (ectx->an * ectx->x).clamp_frac(precision + 10).calc_div(ectx->an, ectx->n + 1, precision + 10);

		if (ectx->n > 20)
		{
			signed_t eqsz = ectx->s.compare_tail(ectx->x0, ectx->calccomparetail());
			if (eqsz >= 0)
			{
				ectx->compint = false;
				ectx->comparetail = eqsz;
			}
			else
			{
				signed_t isz = math::nmax((signed_t)ectx->s.get_core()->integer.size(), (signed_t)ectx->x0.get_core()->integer.size()) + eqsz;
				ectx->comparetail = isz;
			}
			if (eqsz >= ectx->precision + 2)
			{
				++ectx->n;
				ectx->x0 = ectx->s;
				bool maxprec = ectx->precision >= maxprecision || ectx->precision >= precision;
				signed_t p = min(ectx->precision, precision);
				if (ectx->negx && p <= ectx->x0.int_size())
				{
					// negative x means inverse, so grow precision for correct result 
					p = ectx->x0.int_size() + 1;
				}
				ectx->precision = eqsz + 5; // next precision we have to generate
				if (ectx->negx)
				{
					value r;
					ectx->x0.calc_inverse(r, p + 10);
					r.round(p);
					return { r, maxprec };
				}
				ectx->x0.round(p);
				
				return { ectx->x0, maxprec };
			}
		}

		ectx->x0 = ectx->s;
	}
}

/*virtual*/ calc_result_t op_ln::calc(const std::vector<value> &calculated_params, signed_t precision, context *ctx) const
{
    value x = calculated_params[0];
	if (x.is_negative())
		return { value(errset::BAD_ARGUMENT), true };

	if (x.is_zero())
		return { value(errset::INF, true), true };

	if (x.is_infinity())
		return { value(errset::INF, false), true };

    signed_t maxprecision = min(MAX_PRECISION, value::P_ABSOLUTE == x.get_precision() ? MAX_PRECISION : x.get_precision());
	ASSERT(x.get_precision() != value::P_UNDEFINED);

    ln_context *lctx = (ln_context *)ctx;
    lctx->check_reset(x, precision);


    for (;; lctx->n += 2)
    {
		if (globalstop || lctx->calctag != op::calctag)
			return { value(), true};

        //lctx->xx = lctx->xx * lctx->x2;
		value::mul(lctx->xx, lctx->xx, lctx->x2);
        value el;
        lctx->xx.calc_div(el, lctx->n, precision+10);
		lctx->xx.clamp_frac(precision*2);
		lctx->s = lctx->s + el;
        
        if (lctx->n > 9)
        {
			//&& lctx->s.compare(lctx->ps, lctx->precision + 2) == 0

			signed_t eqsz = lctx->s.compare_tail(lctx->ps);
			if (eqsz >= lctx->precision + 2)
			{
				lctx->n += 2;
				lctx->ps = lctx->s;

				bool maxprec = lctx->precision >= maxprecision || lctx->precision >= precision;
				signed_t p = min(lctx->precision, precision);
				
				lctx->precision = eqsz + 5; // next precision we have to generate
				if (lctx->precision > MAX_PRECISION)
					lctx->precision = MAX_PRECISION;

				value r = (lctx->s + lctx->s) * lctx->postmul; // x2
				r.round(p);

				return { r, maxprec };
			}

        }

        lctx->ps = lctx->s;
    }
}



/*virtual*/ calc_result_t op_shiftleft::calc(const std::vector<value> &calculated_params, signed_t precision, context * /*ctx*/) const
{
	value r;
	if (calculated_params[1].is_negative())
	{
		calculated_params[0].calc_shift_rite(r, calculated_params[1]);
	}
	else
	{
		calculated_params[0].calc_shift_left(r, calculated_params[1]);
	}

	r.clamp_frac(precision);

	return { r, true };
}

/*virtual*/ calc_result_t op_shiftrite::calc(const std::vector<value> &calculated_params, signed_t precision, context * /*ctx*/) const
{
	value r;
	if (calculated_params[1].is_negative())
	{
		calculated_params[0].calc_shift_left(r, calculated_params[1]);
	}
	else
	{
		calculated_params[0].calc_shift_rite(r, calculated_params[1]);
	}

	r.clamp_frac(precision);

	return { r, true };
}

/*virtual*/ calc_result_t op_pow::calc(const std::vector<value>& calculated_params, signed_t precision, context* /*ctx*/) const
{
	value y( calculated_params[1] );

	if (!y.is_zero_frac())
	{
		// frac size of y is not zero, do calc by formula exp (y * ln x) e.g. just return y as is (see mutate - formula already processing)
		y.clamp_frac(precision);
		return { y, true }; // y as is
	}

	if (y.is_zero_int())
	{
		// any ^ 0 = 1
		// return 1 as result
		return { value(1,0).bypass(), true};
	}


	// y is integer, so we can simple multiply x by x y times

	//value x[sizeof(void*) * 8] = { (calculated_params[0]) }; // array 

	// x
	// x * x = x2
	// x2 * x2 = x4
	// x4 * x4 = x8
	// ...
	// x32 * x32 = x64
	// so on

	value rslt(1, 0);
	value x(calculated_params[0]);

	usingle iy;
	if (y.to_unsigned(iy))
	{
		if (iy == 1)
		{
			return { x.bypass(), true };
		}

		for (; iy != 0; iy >>= 1 )
		{
			if (0 != (iy & 1))
			{
				rslt = rslt * x;
				rslt.clamp_frac(precision + 10);
				if (iy == 1)
					break;
			}
			x = x * x;
			x.clamp_frac(precision + 10);
		}
	}
	else
	{
		for (; !y.is_zero_int(); y.div_by_2_int())
		{
			if (0 != (y.get_core()->getu8(-1) & 1))
			{
				rslt = rslt * x;
				rslt.clamp_frac(precision + 10);
			}
			x = x * x;
			x.clamp_frac(precision + 10);
		}
	}

	//rslt.clamp_frac(precision);
	rslt.round(precision);
	return { rslt.bypass(), true };

}

/*virtual*/ void op_pow::mutate(operator_node* mynode) const
{
	ASSERT(dynamic_cast<const op_pow*>(mynode->op) != nullptr);

	//node *p = mynode->params[1].get();

	// replace node as below
	// x ^ y => exp (y * ln x)

	// как это работает:
	// текущий нод мутирует в нод по формуле (см. выше)
	// однако, если y целочисленный, в формуле нет необходимости
	// поэтому, если op_pow отработает по целочисленному варианту
	// то ствой результат пометит как BYPASS, тогда вычислитель
	// проведет такой результат сразу на выход

	// lets mutate params now
	mynode->params[0]->mutate();
	mynode->params[1]->mutate();
	
	operator_node* powtrycalc = new operator_node(OP(op_pow));
	powtrycalc->add_par(mynode->params[0], false);
	powtrycalc->add_par(mynode->params[1], false);
	powtrycalc->mutated = true; // mark it already mutated due this node should not be mutated

	operator_node * lnnode = new operator_node(OP(op_ln));
	lnnode->add_par(mynode->params[0], false);
	lnnode->mutate();

	operator_node* mulnode = new operator_node(OP(op_mul));
	mulnode->add_par(ptr::shared_ptr<node>(powtrycalc), true);
	mulnode->add_par(ptr::shared_ptr<node>(lnnode), false);
	mulnode->mutate();
	
	mynode->op = OP(op_exp);
	mynode->result_unmark_bypass = true;
	mynode->params.clear();
	mynode->add_par(ptr::shared_ptr<node>(mulnode), false);
	mynode->mutate();

}


////////////////////


const op::allops &op::all()
{
    static allops ops;
    if (ops.size() == 0)
    {
		ops.emplace_back(new op_pi());
		ops.emplace_back(new op_e());

        ops.emplace_back(new op_ln());
		ops.emplace_back(new op_sqrt());
		ops.emplace_back(new op_exp());
		
		ops.emplace_back(new op_pow());
		ops.emplace_back(new op_div());
        ops.emplace_back(new op_mul());
		
		ops.emplace_back(new op_minus());
		ops.emplace_back(new op_plus());

		ops.emplace_back(new op_shiftleft());
		ops.emplace_back(new op_shiftrite());


        // setup bigger
        for (auto &o1 : ops)
        {
            for (auto &o2 : ops)
            {
                if (o1.get() == o2.get())
                    continue;
                if (o1->name().find(o2->name(), 0) != std::wstring::npos)
                {
                    ASSERT(o2->bigger == nullptr);
                    o2->bigger = o1.get();
                }
            }
        }
    }
    return ops;
}

