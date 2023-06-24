#include "pch.h"

volatile unsigned char op::calctag = 0;

//static op_sys_remove_bypass opsys_remove_bypass;



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
		x0.calc_inverse(invx0, precision * 2);
		value x1 = ((invx0 * a) + x0) * half;

		if (x1.compare(x0, precision) == 0)
			break;

		//std::wstring s1 = x1.to_string();
		//std::wstring s2 = x0.to_string();

		x0 = x1;
		x0.clamp_frac(precision * 2);
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

	return { calc_sqrt(calculated_params[0], precision * 2), true };
}

/*virtual*/ calc_result_t op_exp::calc(const std::vector<value> &calculated_params, signed_t precision, context *ctx) const
{
	value x = calculated_params[0];

	if (x.is_infinity())
		return { x.is_negative() ? value() : x, true};

	if (x.is_zero())
		return { value(1,0), true };

	exp_context* ectx = (exp_context*)ctx;

	usingle ix;
	if (x.is_zero_frac() && x.to_unsigned(ix))
	{
		value e = op_e::calc_e(precision + 10);

		if (ix == 1)
			return { e, true };

		value rslt(1, 0);

		for (; ix != 0; ix >>= 1)
		{
			if (ectx->calctag != op::calctag)
				return { value(), true };

			if (0 != (ix & 1))
			{
				rslt = rslt * e;
				rslt.clamp_frac(precision * 2);

				if (ix == 1)
					break;

			}
			e = e * e;
			e.clamp_frac(precision * 2);

			if (e.int_size() * 2 > cfg.get_maxnum())
			{
				return { value(errset::RESULT_OVERFLOW), true };
			}

		}

		if (x.is_negative())
		{
			value r;
			rslt.calc_inverse(r, precision * 2);
			rslt = r;
		}

		ASSERT(rslt.get_precision() != value::P_UNDEFINED);

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
				signed_t ipa = math::nmax((signed_t)ectx->s.get_core()->integer.size(), (signed_t)ectx->x0.get_core()->integer.size());
				if (ipa * 2 > cfg.get_maxnum())
				{
					return { value(errset::RESULT_OVERFLOW), true};
				}
				signed_t isz = ipa + eqsz;
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
					ectx->x0.calc_inverse(r, p * 2);
					if (!maxprec)
						r.round(p);
					else
						r.clamp_frac(p * 2);

					ASSERT(r.get_precision() != value::P_UNDEFINED);
					return { r, maxprec };
				}
				if (!maxprec)
					ectx->x0.round(p);
				else
					ectx->x0.clamp_frac(p*2);

				ASSERT(ectx->x0.get_precision() != value::P_UNDEFINED);

				return { ectx->x0, maxprec };
			}
		}

		ectx->x0 = ectx->s;
	}
}

/*virtual*/ void op_exp::mutate(operator_node* mynode) const
{
	ASSERT(dynamic_cast<const op_exp*>(mynode->op) != nullptr);

	// replace node as below for faster calculations
	// exp x => exp (int x) * exp (frac x)

	mynode->params[0]->mutate();

	operator_node* makeint = new operator_node(OP(op_int));
	makeint->add_par(mynode->params[0], false);
	makeint->mutate();
	
	operator_node* exp1 = new operator_node(OP(op_exp));
	exp1->add_par(ptr::shared_ptr<node>(makeint), false);
	exp1->mutated = true; // mark it already mutated due this node should not be mutated

	operator_node* makefrac = new operator_node(OP(op_frac));
	makefrac->add_par(mynode->params[0], false);
	makefrac->mutate();

	operator_node* exp2 = new operator_node(OP(op_exp));
	exp2->add_par(ptr::shared_ptr<node>(makefrac), false);
	exp2->mutated = true; // mark it already mutated due this node should not be mutated

	mynode->op = OP(op_mul);
	mynode->params.clear();
	mynode->add_par(ptr::shared_ptr<node>(exp1), true);
	mynode->add_par(ptr::shared_ptr<node>(exp2), false);
	mynode->mutate();

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
        lctx->xx.calc_div(el, lctx->n, precision*2);
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
				if (!maxprec)
					r.round(p);
				else
					r.clamp_frac(p * 2);
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

	bool neg = y.is_negative();

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

			if (neg)
			{
				value invx;
				x.calc_inverse(invx, precision * 2);
				return { invx.bypass(), true };
			}

			return { x.bypass(), true };
		}

		for (; iy != 0; iy >>= 1 )
		{
			if (0 != (iy & 1))
			{
				rslt = rslt * x;
				rslt.clamp_frac(precision * 2);
				if (iy == 1)
					break;
			}
			x = x * x;
			x.clamp_frac(precision * 2);

			if (x.int_size() * 2 > cfg.get_maxnum())
			{
				return { value(errset::RESULT_OVERFLOW), true };
			}

		}
	}
	else
	{
		for (; !y.is_zero_int(); y.div_by_2_int())
		{
			if (0 != (y.get_core()->getu8(-1) & 1))
			{
				rslt = rslt * x;
				rslt.clamp_frac(precision*2);
			}
			x = x * x;
			x.clamp_frac(precision*2);

			if (x.int_size() * 2 > cfg.get_maxnum())
			{
				return { value(errset::RESULT_OVERFLOW), true };
			}

		}
	}

	if (neg)
	{
		value invx;
		rslt.calc_inverse(invx, precision*2);
		rslt = invx;
	}


	//rslt.clamp_frac(precision);
	//rslt.round(precision);
	return { rslt.bypass(), true };

}

/*virtual*/ void op_pow::mutate(operator_node* mynode) const
{
	ASSERT(dynamic_cast<const op_pow*>(mynode->op) != nullptr);

	//node *p = mynode->params[1].get();

	// replace node as below
	// x ^ y => exp (y * ln x)

	// how it works:
	// the current node mutates into a node according to the formula (see above)
	// however, if y is an integer, the formula is not needed
	// therefore, if op_pow works on an integer variant
	// then mark its result as BYPASS, then the calculation engine
	// will pass such a result immediately to the output

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
		// consts
		ops.emplace_back(new op_pi());
		ops.emplace_back(new op_e());

        ops.emplace_back(new op_ln());
		ops.emplace_back(new op_sqrt());
		ops.emplace_back(new op_exp());

		// helpers
		ops.emplace_back(new op_int());
		ops.emplace_back(new op_frac());
		ops.emplace_back(new op_anorm());

		ops.emplace_back(new op_pow());
		ops.emplace_back(new op_sin());
		ops.emplace_back(new op_cos());
		ops.emplace_back(new op_tan());

		// base
		ops.emplace_back(new op_div());
        ops.emplace_back(new op_mul());
		ops.emplace_back(new op_mod());
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

