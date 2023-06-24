#pragma once

class op_sin : public op
{
public:

	op_sin() :op(PRECEDENCE_FUNC) {}

	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;

	/*virtual*/ std::wstring_view name() const override { return WSTR("sin"); }
	/*virtual*/ const char* d_name() const override { return "sin"; }
	/*virtual*/ bool is_valid_param(size_t pnumpref, size_t pnumpostf) const override
	{
		return pnumpref == 0 && pnumpostf == 1;
	}
	/*virtual*/ void mutate(operator_node*) const;

	static value calc_sin(const value& v, signed_t precision);
};


class op_cos : public op
{
public:

	op_cos() :op(PRECEDENCE_FUNC) {}

	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;

	/*virtual*/ std::wstring_view name() const override { return WSTR("cos"); }
	/*virtual*/ const char* d_name() const override { return "cos"; }
	/*virtual*/ bool is_valid_param(size_t pnumpref, size_t pnumpostf) const override
	{
		return pnumpref == 0 && pnumpostf == 1;
	}
	/*virtual*/ void mutate(operator_node*) const;

	static value calc_cos(const value& v, signed_t precision);
};

class op_tan : public op
{
public:

	op_tan() :op(PRECEDENCE_FUNC) {}

	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;

	/*virtual*/ std::wstring_view name() const override { return WSTR("tan"); }
	/*virtual*/ const char* d_name() const override { return "tan"; }
	/*virtual*/ bool is_valid_param(size_t pnumpref, size_t pnumpostf) const override
	{
		return pnumpref == 0 && pnumpostf == 1;
	}
	/*virtual*/ void mutate(operator_node*) const;

};
