#pragma once


class op_int_c : public op
{
public:

	op_int_c() :op(PRECEDENCE_FUNC) {}
	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;
};

class op_frac_c : public op
{
public:

	op_frac_c() :op(PRECEDENCE_FUNC) {}
	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;
};

class op_anorm_c : public op
{
public:

	op_anorm_c() :op(PRECEDENCE_FUNC) {}
	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;
};


