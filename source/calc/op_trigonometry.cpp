#include "pch.h"


value op_sin_c::calc_sin(const value& x, signed_t precision)
{
	// Taylor series

	//            x^3   x^5   x^7
	//sin x = x - --- + --- - --- ...
	//			   3!    5!    7!

	value acc = x;
	value xx = x * x;
	value ch = x * xx;
	value dd = value(6,0);
	value ddn = value(3,0);

	for (bool minus = true;;minus = !minus)
	{
		value pac = acc;

		value m;
		ch.calc_div(m, dd, precision * 2);
		if (minus)
		{
			acc = acc - m;
		}
		else
		{
			acc = acc + m;
		}
		ddn.add(1);
		dd = dd * ddn;
		ddn.add(1);
		dd = dd * ddn;
		ch = ch * xx;

		acc.clamp_frac(precision + 10);

		if (acc.compare(pac, precision) == 0)
			break;

		ch.clamp_frac(precision + 10);

	}
	acc.round(precision);
	return acc;

}

/*virtual*/ calc_result_t op_sin_c::calc(const std::vector<value>& calculated_params, signed_t precision, context* /*ctx*/) const
{
	if (calculated_params[0].is_infinity())
		return { value(errset::BAD_ARGUMENT), true};

	return { calc_sin(calculated_params[0], optimal_precision(calculated_params[0].get_precision(), precision)), true };
}


/*virtual*/ void op_sin_c::mutate(operator_node* mynode) const
{
	//op::mutate(mynode);

	ASSERT(dynamic_cast<const op_sin_c*>(mynode->op) != nullptr);

	// make formula:
	// sin x == sin( anorm(x,pi) )

	mynode->params[0]->mutate();

	operator_node* anorm = new operator_node(OP(op_anorm));
	anorm->add_par(mynode->params[0], false);
	anorm->mutate();

	mynode->params.clear();
	mynode->add_par(ptr::shared_ptr<node>(anorm), true);
	mynode->mutated = true;
}


value op_cos_c::calc_cos(const value& x, signed_t precision)
{
	// Taylor series

	//            x^2   x^4   x^6
	//cos x = 1 - --- + --- - --- ...
	//			   2!    4!    6!

	value acc = value(1,0);
	value xx = x * x;
	value ch = xx;
	value dd = value(2, 0);
	value ddn = value(2, 0);

	for (bool minus = true;; minus = !minus)
	{
		value pac = acc;

		value m;
		ch.calc_div(m, dd, precision * 2);
		if (minus)
		{
			acc = acc - m;
		}
		else
		{
			acc = acc + m;
		}
		ddn.add(1);
		dd = dd * ddn;
		ddn.add(1);
		dd = dd * ddn;
		ch = ch * xx;

		acc.clamp_frac(precision + 10);

		if (acc.compare(pac, precision) == 0)
			break;

		ch.clamp_frac(precision + 10);

	}
	acc.round(precision);
	return acc;

}

/*virtual*/ calc_result_t op_cos_c::calc(const std::vector<value>& calculated_params, signed_t precision, context* /*ctx*/) const
{
	if (calculated_params[0].is_infinity())
		return { value(errset::BAD_ARGUMENT), true };

	return { calc_cos(calculated_params[0], optimal_precision(calculated_params[0].get_precision(), precision)), true };
}


/*virtual*/ void op_cos_c::mutate(operator_node* mynode) const
{
	//op::mutate(mynode);

	ASSERT(dynamic_cast<const op_cos_c*>(mynode->op) != nullptr);

	// make formula:
	// cos x == cos( anorm(x,pi) )

	mynode->params[0]->mutate();

	operator_node* anorm = new operator_node(OP(op_anorm));
	anorm->add_par(mynode->params[0], false);
	anorm->mutate();

	mynode->params.clear();
	mynode->add_par(ptr::shared_ptr<node>(anorm), true);
	mynode->mutated = true;
}


/*virtual*/ calc_result_t op_tan_c::calc(const std::vector<value>& /*calculated_params*/, signed_t /*precision*/, context* /*ctx*/) const
{
	// tan never called directly
	return { value(errset::BAD_ARGUMENT), true };
}


/*virtual*/ void op_tan_c::mutate(operator_node* mynode) const
{
	//op::mutate(mynode);

	ASSERT(dynamic_cast<const op_tan_c*>(mynode->op) != nullptr);

	// make formula:
	// cos x == cos( anorm(x,pi) )

	mynode->params[0]->mutate();

	operator_node* anorm = new operator_node(OP(op_anorm));
	anorm->add_par(mynode->params[0], false);
	anorm->mutate();

	operator_node* sinus = new operator_node(OP(op_sin));
	sinus->add_par(ptr::shared_ptr<node>(anorm), false);
	sinus->mutated = true; // don't mutate sinus due anorm already applied

	operator_node* cosinus = new operator_node(OP(op_cos));
	cosinus->add_par(ptr::shared_ptr<node>(anorm), false);
	sinus->mutated = true;  // don't mutate cosinus due anorm already applied

	mynode->op = OP(op_div);
	mynode->params.clear();
	mynode->add_par(ptr::shared_ptr<node>(sinus), true);
	mynode->add_par(ptr::shared_ptr<node>(cosinus), false);
	mynode->mutated = true;
}
