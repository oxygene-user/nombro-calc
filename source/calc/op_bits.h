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

class op_and_c : public op
{
public:

	op_and_c() :op(PRECEDENCE_AND) {}
	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;
};

class op_xor_c : public op
{
public:

	op_xor_c() :op(PRECEDENCE_XOR) {}
	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;
};

class op_or_c : public op
{
public:

	op_or_c() :op(PRECEDENCE_OR) {}
	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;
};

