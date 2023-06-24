#pragma once

typedef std::tuple<value, bool> calc_result_t;
struct operator_node;

#define OP(opt) op::findop<opt>()

class op
{
public:

	enum
	{
		PRECEDENCE_CONST = 1,
		PRECEDENCE_FUNC = 2,
		PRECEDENCE_POW = 3,
		PRECEDENCE_MULT = 4,
		PRECEDENCE_PLUS = 5,
		PRECEDENCE_SHIFT = 6,


		PRECEDENCE_LOWEST
	};

	static volatile unsigned char calctag;

	class context
	{
	public:
		u8 calctag;
		bool used = false;
		context(u8 ct) :calctag(ct) {}
		virtual ~context() {}
	};

    op *bigger = nullptr; // pointer to op with bigger name (name of current op is substring of bigger)
	signed_t precedence;

	op(signed_t precedence) :precedence(precedence) {}
    virtual ~op() {}
	virtual calc_result_t calc(const std::vector<value>& /*calculated_params*/, signed_t /*precision*/, context*) const { ERRORM(__FILE__, __LINE__, "calc not defined"); return { value(), true }; };
	virtual void mutate(operator_node*) const {};
    virtual std::wstring_view name() const = 0;
	virtual const char* d_name() const = 0;
	
	virtual context *create_context(u8 ct) const { return new context(ct); }

    typedef std::vector<std::unique_ptr<op>> allops;
    static const allops &all();

	virtual bool is_valid_param(size_t pnumpref, size_t pnumpostf) const = 0;

	template<typename OPT> static inline const OPT* findop()
	{
		for (const auto& o : all())
		{
			const OPT* oo = dynamic_cast<const OPT*>(o.get());
			if (oo != nullptr)
				return oo;
		}
		return nullptr;
	}

};


#include "op_base.h"
#include "op_constants.h"
#include "op_helpers.h"
#include "op_trigonometry.h"

class op_pow : public op
{
public:

	op_pow() :op(PRECEDENCE_POW) {}

	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;

	/*virtual*/ void mutate(operator_node*) const;

	/*virtual*/ std::wstring_view name() const override { return WSTR("^"); }
	/*virtual*/ const char* d_name() const override { return "pow"; }
	virtual bool is_valid_param(size_t pnumpref, size_t pnumpostf) const override
	{
		return pnumpref == 1 && pnumpostf == 1;
	}
};

class op_sqrt : public op
{
public:

	op_sqrt() :op(PRECEDENCE_FUNC) {}

	/*virtual*/ calc_result_t calc(const std::vector<value> &calculated_params, signed_t precision, context *ctx) const override;

	/*virtual*/ std::wstring_view name() const override { return WSTR("sqrt"); }
	/*virtual*/ const char* d_name() const override { return "sqrt"; }
	virtual bool is_valid_param(size_t pnumpref, size_t pnumpostf) const override
	{
		return pnumpref == 0 && pnumpostf == 1;
	}

	static value calc_sqrt(const value& v, signed_t precision);
};

class op_exp : public op
{
	struct exp_context : public context
	{
		value an, s, x;
		value x0;
        usingle n = 0;
		signed_t precision = 5;
		signed_t comparetail = math::minimum<signed_t>::value;
		bool negx = false;
		bool compint = true;

		exp_context(u8 ct): context(ct), an(1,0), x(errset::CALCULATING)
		{
		}

		void reset(const value&xx)
		{
			s = value();
			an = value(1, 0);
			x = xx;
			x0 = xx;
			n = 0;
			precision = 5;
			negx = x.is_negative();
			if (negx)
				x.set_negative(false);
		}
		void check_reset(const value&xx, signed_t prec)
		{
			if (x.error() == errset::CALCULATING || x.compare(xx, prec) != 0)
				reset(xx);

			precision = prec; // TODO: semiresult
		}

		signed_t calccomparetail() const
		{
			if (compint && comparetail != math::minimum<signed_t>::value)
			{
				signed_t isz = math::nmax((signed_t)s.get_core()->integer.size(), (signed_t)x0.get_core()->integer.size());
				return comparetail - isz - 2; // always rollback 2 digits to be sure all doing right
			}
			else
				return comparetail;

		}
	};

public:
	op_exp() :op(PRECEDENCE_FUNC) {}


	/*virtual*/ calc_result_t calc(const std::vector<value> &calculated_params, signed_t precision, context *ctx) const override;

	/*virtual*/ std::wstring_view name() const override { return WSTR("exp"); }
	/*virtual*/ const char* d_name() const override { return "exp"; }

	virtual context *create_context(u8 ct) const { return new exp_context(ct); }
	virtual bool is_valid_param(size_t pnumpref, size_t pnumpostf) const override
	{
		return pnumpref == 0 && pnumpostf == 1;
	}

	/*virtual*/ void mutate(operator_node*) const;
};

class op_ln : public op
{
    struct ln_context : public context
    {
        value z, x, x2, xx, s, ps, postmul = value(1,0);

        usingle n = 3;
        signed_t precision = 5;

        ln_context(u8 ct):context(ct), x(errset::CALCULATING)
        {
        }

        void reset(const value&iz, signed_t prec)
        {
			value maxx(10, 0);
			z = iz;
			x = iz;

			for (;;)
			{
				if (x.compare(maxx, 0) < 0)
					break;

				x = op_sqrt::calc_sqrt(x, precision*16);
				postmul = postmul + postmul;
			}

            precision = 5;
            n = 3;

			
			x.sub(1);
			x.calc_div(s, x+2, prec+10);
			x = s;
            xx = x;
            x2 = x * x;
            s = x;
        }
        void check_reset(const value&iz, signed_t prec)
        {
            if (z.error() == errset::CALCULATING || z.compare(iz, prec) != 0)
                reset(iz, prec);

			precision = prec; // TODO: semiresult
        }
    };

public:
	op_ln() :op(PRECEDENCE_FUNC) {}


    /*virtual*/ calc_result_t calc(const std::vector<value> &calculated_params, signed_t precision, context *ctx) const override;

    /*virtual*/ std::wstring_view name() const override { return WSTR("ln"); }
	/*virtual*/ const char* d_name() const override { return "ln"; }

    virtual context *create_context(u8 ct) const { return new ln_context(ct); }
    virtual bool is_valid_param(size_t pnumpref, size_t pnumpostf) const override
    {
        return pnumpref == 0 && pnumpostf == 1;
    }
};



class op_shiftleft : public op
{
public:
	op_shiftleft() :op(PRECEDENCE_SHIFT) {}

	/*virtual*/ calc_result_t calc(const std::vector<value> &calculated_params, signed_t precision, context *ctx) const override;

	/*virtual*/ std::wstring_view name() const override { return WSTR("<<"); }
	/*virtual*/ const char* d_name() const override { return "shl"; }

	virtual bool is_valid_param(size_t pnumpref, size_t pnumpostf) const override
	{
		return pnumpref == 1 && pnumpostf == 1;
	}
};

class op_shiftrite : public op
{
public:

	op_shiftrite() :op(PRECEDENCE_SHIFT) {}

	/*virtual*/ calc_result_t calc(const std::vector<value> &calculated_params, signed_t precision, context *ctx) const override;

	/*virtual*/ std::wstring_view name() const override { return WSTR(">>"); }
	/*virtual*/ const char* d_name() const override { return "shr"; }
	virtual bool is_valid_param(size_t pnumpref, size_t pnumpostf) const override
	{
		return pnumpref == 1 && pnumpostf == 1;
	}
};

