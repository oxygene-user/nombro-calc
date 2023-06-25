#pragma once

typedef std::tuple<value, bool> calc_result_t;
struct operator_node;

#define OP(o) ((o##_c *)op::getop(o))
#define FUNC npars(0,1) // function with one arg
#define INFOP npars(1,1)  // infix operator like +

struct npars
{
	uhalf prepn = 0;
	uhalf pospn = 0;
	npars() {}
	npars(uhalf prepn, uhalf pospn) :prepn(prepn), pospn(pospn) {}

	bool operator==(const npars& oo) const
	{
		return ref_cast<usingle>(*this) == ref_cast<usingle>(oo);
	}
	bool operator()(uhalf _prepn, uhalf _pospn) const
	{
		return prepn == _prepn && pospn == _pospn;
	}
};

#define OPS \
		O( pi ) \
		O( e ) \
		O( phi ) \
		O( neg, FUNC, std::wstring_view(L"\u2212", 1) ) \
		O( ln, FUNC ) \
		O( sqrt, FUNC ) \
		O( exp, FUNC ) \
		O( int, FUNC ) \
		O( frac, FUNC ) \
		O( anorm, FUNC ) \
		O( pow, INFOP, WSTR("^") ) \
		O( sin, FUNC ) \
		O( cos, FUNC ) \
		O( tan, FUNC ) \
		O( div, INFOP, std::wstring_view(L"\u00f7", 1) ) \
		O( mul, INFOP, std::wstring_view(L"\u00d7", 1) ) \
		O( mod, INFOP, WSTR("\\") ) \
		O( minus, INFOP, std::wstring_view(L"\u2212", 1) ) \
		O( plus, INFOP, WSTR("+") ) \
		O( shl, INFOP, WSTR("<<") ) \
		O( shr, INFOP, WSTR(">>") ) \

#define O(o, ...) op_##o##,
enum operator_e {
	OPS
	OPS_COUNT
};
#undef O

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

	std::wstring_view name;
    op *bigger = nullptr; // pointer to op with bigger name (name of current op is substring of bigger)
	op* synonym = nullptr; // synonym (op with same name but lower precedence)
	signed_t precedence;
	npars reqpars;

	op(signed_t precedence) :precedence(precedence) {}
    virtual ~op() {}
	virtual calc_result_t calc(const std::vector<value>& /*calculated_params*/, signed_t /*precision*/, context*) const { ERRORM(__FILE__, __LINE__, "calc not defined"); return { value(), true }; };
	virtual void mutate(operator_node*) const {};
	
	virtual context *create_context(u8 ct) const { return new context(ct); }

    typedef std::unique_ptr<op> allops[OPS_COUNT];
    static const allops &all();

	static inline const op* getop(operator_e o)
	{
		return all()[o].get();
	}

#ifdef LOGGER
	const char* debug_name = nullptr;
	const char* d_name() const { return debug_name; }
#endif
};


#include "op_base.h"
#include "op_constants.h"
#include "op_helpers.h"
#include "op_trigonometry.h"

class op_pow_c : public op
{
public:

	op_pow_c() :op(PRECEDENCE_POW) {}
	/*virtual*/ calc_result_t calc(const std::vector<value>& calculated_params, signed_t precision, context* ctx) const override;
	/*virtual*/ void mutate(operator_node*) const;
};

class op_sqrt_c : public op
{
public:

	op_sqrt_c() :op(PRECEDENCE_FUNC) {}
	/*virtual*/ calc_result_t calc(const std::vector<value> &calculated_params, signed_t precision, context *ctx) const override;
	static value calc_sqrt(const value& v, signed_t precision);
};

class op_exp_c : public op
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
	op_exp_c() :op(PRECEDENCE_FUNC) {}

	/*virtual*/ calc_result_t calc(const std::vector<value> &calculated_params, signed_t precision, context *ctx) const override;
	virtual context *create_context(u8 ct) const { return new exp_context(ct); }
	/*virtual*/ void mutate(operator_node*) const;
};

class op_ln_c : public op
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

				x = op_sqrt_c::calc_sqrt(x, precision*16);
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
	op_ln_c() :op(PRECEDENCE_FUNC) {}

    /*virtual*/ calc_result_t calc(const std::vector<value> &calculated_params, signed_t precision, context *ctx) const override;
    virtual context *create_context(u8 ct) const { return new ln_context(ct); }
};



class op_shl_c : public op
{
public:
	op_shl_c() :op(PRECEDENCE_SHIFT) {}
	/*virtual*/ calc_result_t calc(const std::vector<value> &calculated_params, signed_t precision, context *ctx) const override;
};

class op_shr_c : public op
{
public:

	op_shr_c() :op(PRECEDENCE_SHIFT) {}
	/*virtual*/ calc_result_t calc(const std::vector<value> &calculated_params, signed_t precision, context *ctx) const override;
};

