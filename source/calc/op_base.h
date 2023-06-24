#pragma once

class op_mul : public op
{
public:

	op_mul() :op(PRECEDENCE_MULT) {}

	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;

	/*virtual*/ std::wstring_view name() const override
	{
		return std::wstring_view(L"\u00d7", 1);
	}
	/*virtual*/ const char* d_name() const override { return "mul"; }

	virtual bool is_valid_param(size_t pnumpref, size_t pnumpostf) const override
	{
		return pnumpref == 1 && pnumpostf == 1;
	}
};

class op_div : public op
{
public:

	op_div() :op(PRECEDENCE_MULT) {}

	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;

	/*virtual*/ std::wstring_view name() const override { return std::wstring_view(L"\u00f7", 1); }
	/*virtual*/ const char* d_name() const override { return "div"; }
	virtual bool is_valid_param(size_t pnumpref, size_t pnumpostf) const override
	{
		return pnumpref == 1 && pnumpostf == 1;
	}
};

class op_plus : public op
{
public:
	op_plus() :op(PRECEDENCE_PLUS) {}


	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;

	/*virtual*/ std::wstring_view name() const override { return WSTR("+"); }
	/*virtual*/ const char* d_name() const override { return "plus"; }
	virtual bool is_valid_param(size_t pnumpref, size_t pnumpostf) const override
	{
		return pnumpref == 1 && pnumpostf == 1;
	}
};

class op_minus : public op
{
public:
	op_minus() :op(PRECEDENCE_PLUS) {}

	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;

	/*virtual*/ std::wstring_view name() const override { return std::wstring_view(L"\u2212", 1); }
	/*virtual*/ const char* d_name() const override { return "minus"; }
	virtual bool is_valid_param(size_t pnumpref, size_t pnumpostf) const override
	{
		return pnumpref <= 1 && pnumpostf == 1;
	}
};

class op_mod : public op
{
public:

	op_mod() :op(PRECEDENCE_MULT) {}

	static value calc_mod(const value& v, const value& m);

	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;

	/*virtual*/ std::wstring_view name() const override { return WSTR("\\"); }
	/*virtual*/ const char* d_name() const override { return "mod"; }

	virtual bool is_valid_param(size_t pnumpref, size_t pnumpostf) const override
	{
		return pnumpref == 1 && pnumpostf == 1;
	}

};
