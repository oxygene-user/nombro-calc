#pragma once


class op_pi : public op
{
public:

	op_pi() :op(PRECEDENCE_CONST) {}

	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;

	/*virtual*/ std::wstring_view name() const override { return WSTR("pi"); }
	/*virtual*/ const char* d_name() const override { return "pi"; }
	virtual bool is_valid_param(size_t pnumpref, size_t pnumpostf) const override
	{
		return pnumpref == 0 && pnumpostf == 0;
	}

	static value calc_pi(signed_t precision);
};

class op_e : public op
{
public:

	op_e() :op(PRECEDENCE_CONST) {}

	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;

	/*virtual*/ std::wstring_view name() const override { return WSTR("e"); }
	/*virtual*/ const char* d_name() const override { return "e"; }
	virtual bool is_valid_param(size_t pnumpref, size_t pnumpostf) const override
	{
		return pnumpref == 0 && pnumpostf == 0;
	}

	static value calc_e(signed_t precision);
};



