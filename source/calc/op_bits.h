#pragma once


class op_shl_c : public op
{
public:
	op_shl_c() :op(PRECEDENCE_SHIFT) {}
	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;
};

class op_shr_c : public op
{
public:

	op_shr_c() :op(PRECEDENCE_SHIFT) {}
	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;
};

