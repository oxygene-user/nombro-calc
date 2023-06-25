#pragma once


class op_pi_c : public op
{
	std::mutex mutex;
	value cache = value(errset::EMPTY);

public:

	op_pi_c() :op(PRECEDENCE_CONST) {}

	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;
	static value calc_pi(signed_t precision);
};

class op_e_c : public op
{
	std::mutex mutex;
	value cache = value(errset::EMPTY);

public:

	op_e_c() :op(PRECEDENCE_CONST) {}

	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;
	static value calc_e(signed_t precision);
};

class op_phi_c : public op
{
	std::mutex mutex;
	value cache = value(errset::EMPTY);

public:

	op_phi_c() :op(PRECEDENCE_CONST) {}

	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;
};


