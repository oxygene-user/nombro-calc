#pragma once

class op_sin_c : public op
{
public:

	op_sin_c() :op(PRECEDENCE_FUNC) {}

	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;
	/*virtual*/ void mutate(operator_node*) const;
	static value calc_sin(const value& v, signed_t precision);
};


class op_cos_c : public op
{
public:

	op_cos_c() :op(PRECEDENCE_FUNC) {}

	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;
	/*virtual*/ void mutate(operator_node*) const;
	static value calc_cos(const value& v, signed_t precision);
};

class op_tan_c : public op
{
public:

	op_tan_c() :op(PRECEDENCE_FUNC) {}
	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;
	/*virtual*/ void mutate(operator_node*) const;

};
