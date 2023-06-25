#pragma once

class op_mul_c : public op
{
public:

	op_mul_c() :op(PRECEDENCE_MULT) {}
	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;
};

class op_div_c : public op
{
public:

	op_div_c() :op(PRECEDENCE_MULT) {}
	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;
};

class op_plus_c : public op
{
public:
	op_plus_c() :op(PRECEDENCE_PLUS) {}
	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;
};

class op_minus_c : public op
{
public:
	op_minus_c() :op(PRECEDENCE_PLUS) {}
	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;
};

class op_neg_c : public op
{
public:
	op_neg_c() :op(PRECEDENCE_FUNC) {}
	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;
};


class op_mod_c : public op
{
public:

	op_mod_c() :op(PRECEDENCE_MULT) {}

	static value calc_mod(const value& v, const value& m);
	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;
};
