#pragma once


class op_int : public op
{
public:

	op_int() :op(PRECEDENCE_FUNC) {}

	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;

	/*virtual*/ std::wstring_view name() const override { return WSTR("int"); }
	/*virtual*/ const char* d_name() const override { return "int"; }

	virtual bool is_valid_param(size_t pnumpref, size_t pnumpostf) const override
	{
		return pnumpref == 0 && pnumpostf == 1;
	}
};

class op_frac : public op
{
public:

	op_frac() :op(PRECEDENCE_FUNC) {}

	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;

	/*virtual*/ std::wstring_view name() const override { return WSTR("frac"); }
	/*virtual*/ const char* d_name() const override { return "frac"; }

	virtual bool is_valid_param(size_t pnumpref, size_t pnumpostf) const override
	{
		return pnumpref == 0 && pnumpostf == 1;
	}

};

class op_anorm : public op
{
public:

	op_anorm() :op(PRECEDENCE_FUNC) {}

	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;

	/*virtual*/ std::wstring_view name() const override { return WSTR("anorm"); }
	/*virtual*/ const char* d_name() const override { return "anorm"; }

	virtual bool is_valid_param(size_t pnumpref, size_t pnumpostf) const override
	{
		return pnumpref == 0 && pnumpostf == 1;
	}

};


