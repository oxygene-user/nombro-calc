#pragma once

enum class errset
{
    OK,
	EMPTY,
	CALCULATING, // not yet calculated
	BYPASS, // operator should bypass value as result without any calculations
	STOP,

	INF,

    BAD_ARGS_NUM,
    BAD_SYMBOLS_IN_EXPRESION,
    EXPRESSION_TOO_BIG,
	RESULT_OVERFLOW,
	PARENTHESIS_FAIL, // problem with brackets
    NUMBER_NOT_RECOGNIZED,
    UNKNOWN_RADIX,
	EMPTY_SUBEXPRESSION,
	DIVISION_BY_ZERO,
	XOVERFLOW, // 
	VARIABLE_NOT_FOUND,
	OPERATOR_NOT_FOUND,
	BAD_ARGUMENT,

	// dont forget to update error_text func
};

std::wstring error_text(errset er);

#define INFINITY_PRECISION (1000000)
#define MAX_PRECISION (100)

enum consts
{
	CPNST_MAX_BINARY_SHIFT_VALUE = 16384,
};

class value;
value operator*(const value &, const value &);
value operator-(const value &, const value &);
value operator+(const value &, const value &);
value operator+(const value &, u64);

class value
{
public:
	class bvec : public std::vector<u8>
	{

	};
	typedef bvec integer_t;
	typedef bvec frac_t;
private:

	friend value operator+(const value &vv1, const value &vv2);
	friend value operator+(const value &vv1, u64);
	friend value operator-(const value &vv1, const value &vv2);
	friend value operator*(const value &vv1, const value &vv2);

	enum
	{
		CO_CLEAR_INT = 1,
		CO_CLEAR_FRAC = 2
	};

	static void set_unsigned(bvec&buf, u64 v)
	{
		signed_t i = buf.size() - 1;
		for (; i >= 0 && v > 0; --i)
		{
            u64 vt = v / 100;
			u8 vv = tools::as_byte(v - vt * 100);
			buf[i] = vv;
			v = vt;
		}
		if (i > 0)
			memset(buf.data(), 0, i+1);
	}

	static bool add(bvec &tgt, const bvec&src, bool acc)
	{
		ASSERT(tgt.size() == src.size());
		for (signed_t i = tgt.size() - 1; i >= 0; --i)
		{
			signed_t sum = tgt[i] + src[i];
			
			if (acc)
			{
				++sum;
				acc = false;
			}

			if (sum >= 100)
			{
				acc = true;
				sum -= 100;
			}
			tgt[i] = tools::as_byte(sum);
		}
		return acc;
	}

	static void add(bvec &buf, u64 v)
	{
		if (v > 99)
		{
			bvec num;
			
			if (buf.size() < 20)
				buf.insert(buf.begin(), 20-buf.size(), 0);

			num.resize(buf.size());
			set_unsigned(num, v);
			if (add(buf, num, v))
				buf.insert(buf.begin(), 1, 1);

			trunc_left(buf);

			return;
		}

		if (buf.size() == 0)
		{
			buf.push_back(tools::as_byte(v));
			return;
		}

		signed_t n = buf.size() - 1;
		for (; n >= 0; --n)
		{
			u8 t = buf[n];
			t = tools::as_byte(v + t);
			if (t < 100)
			{
				buf[n] = t;
				return;
			}
			t -= 100;
			buf[n] = t;
			v = 1;
		}
		buf.insert(buf.begin(), 1, 1);
	}

	static bool sub_frac(std::vector<u8> &tgt, const std::vector<u8> &src1, const std::vector<u8> &src2)
	{
		//ASSERT(tgt.size() == src.size());

		auto get = [](signed_t index, const std::vector<u8> &src) ->signed_t
		{
			return index >= (signed_t)src.size() ? 0 : src[index];
		};

		bool acc = false;
		for (signed_t i = tgt.size() - 1; i >= 0; --i)
		{
			signed_t sum = get(i, src1) - get(i, src2) - acc;
			acc = false;

			if (sum < 0)
			{
				acc = true;
				sum += 100;
			}
			tgt[i] = tools::as_byte(sum);
		}
		return acc;
	}

	static bool sub_int(std::vector<u8> &tgt, const std::vector<u8> &src1, const std::vector<u8> &src2, bool acc)
	{
		auto get = [](signed_t index, const std::vector<u8> &src) ->signed_t
		{
			signed_t aindex = src.size() - index - 1;
			return aindex < 0 ? 0 : src[aindex];
		};

		for (signed_t i = tgt.size() - 1, j = 0; i >= 0; --i, ++j)
		{
			signed_t sum = get(j, src1) - get(j, src2) - (acc ? 1 : 0);
			acc = false;

			if (sum < 0)
			{
				acc = true;
				sum += 100;
			}
			tgt[i] = tools::as_byte(sum);
		}
		return acc;
	}


	static const value::bvec&get_min(const value::bvec&s1, const value::bvec&s2)
	{
		if (s1.size() < s2.size())
			return s1;
		return s2;
	}

	static const value::bvec&get_max(const value::bvec &s1, const value::bvec&s2)
	{
		if (s1.size() >= s2.size())
			return s1;
		return s2;
	}

	static void copy_int(integer_t &tgt, const integer_t&src)
	{
		ASSERT(tgt.size() >= src.size());
		size_t zero = tgt.size() - src.size();
		memcpy(tgt.data() + zero, src.data(), src.size());
		for (size_t i = 0; i < zero; ++i)
			tgt[i] = 0;
	}

	static void copy_frac(std::vector<u8> &tgt, const std::vector<u8> &src)
	{
		ASSERT(tgt.size() >= src.size());
		memcpy(tgt.data(), src.data(), src.size());
		for (size_t i = src.size(), c = tgt.size(); i < c; ++i)
			tgt[i] = 0;
	}

	static void unsigned_plus(value &rv, const value &vv1, const value &vv2) // plus ignore sign
	{
		const value::value_core *c1 = vv1.get_core();
		const value::value_core *c2 = vv2.get_core();

		i32 expo = c1->exponent;
		ASSERT(expo == c2->exponent);

		frac_t& frac = rv.alloc_frac(max(c1->frac.size(), c2->frac.size()), nullptr);
		const frac_t &frac2 = get_max(c1->frac, c2->frac);
		copy_frac(frac, get_min(c1->frac, c2->frac));
		bool acc = add(frac, frac2, false);
		integer_t & inte = rv.alloc_int(max(c1->integer.size(), c2->integer.size()), nullptr);
		const integer_t&inte2 = get_max(c1->integer, c2->integer);
		copy_int(inte, get_min(c1->integer, c2->integer));
		acc = add(inte, inte2, acc);
		if (acc)
			inte.insert(inte.begin(), 1, 1);

		rv.set_exponent(expo);
		rv.set_negative(false);
		rv.trunc();
	}

	static void unsigned_minus(value &rv, const value &vv1, const value &vv2) // minus ignore sign but can produce negative
	{
		const value::value_core *c1 = vv1.get_core();
		const value::value_core *c2 = vv2.get_core();

		i32 expo = c1->exponent;
		ASSERT(expo == c2->exponent);


		std::vector<u8> & frac = rv.alloc_frac(max(c1->frac.size(), c2->frac.size()), 0);
		std::vector<u8> & inte = rv.alloc_int(max(c1->integer.size(), c2->integer.size()), 0);
		bool acc = sub_frac(frac, c1->frac, c2->frac);
		acc = sub_int(inte, c1->integer, c2->integer, acc);
		if (acc)
		{
			// vv2 greater then vv1
			// swap them and recalculate

			acc = sub_frac(frac, c2->frac, c1->frac);
			acc = sub_int(inte, c2->integer, c1->integer, acc);
			ASSERT(acc == 0);
			rv.set_negative(true);
		}
		else
			rv.set_negative(false);

		rv.set_exponent(expo);
		rv.trunc();
	}


	static void trunc_left(std::vector<u8> &buf)
	{
		signed_t i = 0;
		for (signed_t cnt = buf.size(); i < cnt && buf[i] == 0; ++i);
		if (i > 0)
			buf.erase(buf.begin(), buf.begin() + i);
	}

	static void plus(value &rv, const value &vv1, const value &vv2)
	{
		const value::value_core *c1 = vv1.get_core();
		const value::value_core *c2 = vv2.get_core();

		if (!c1->negative)
		{
			if (c2->negative)
			{
				value::unsigned_minus(rv, vv1, vv2);
				return;
			}
		}
		else if (!c2->negative)
		{
			value::unsigned_minus(rv, vv2, vv1);
			return;
		}
		else {
			// both negative

			value::unsigned_plus(rv, vv1, vv2);
			rv.set_negative(true);
			return;
		}
		value::unsigned_plus(rv, vv1, vv2);
	}

	static void minus(value &rv, const value &vv1, const value &vv2)
	{
		const value::value_core *c1 = vv1.get_core();
		const value::value_core *c2 = vv2.get_core();

		if (!c1->negative)
		{
			// 5 - -2 = 5+2
			if (c2->negative)
			{
				value::unsigned_plus(rv, vv1, vv2);
				return;
			}
		}
		else if (!c2->negative)
		{
			// -5 - 2 = -(5+2)
			value::unsigned_plus(rv, vv1, vv2);
			rv.set_negative(true);
			return;
		}
		else {
			// both negative
			// -5 - -2 = 2 - 5
			value::unsigned_minus(rv, vv2, vv1);
			return;
		}

		value::unsigned_minus(rv, vv1, vv2);
		return;
	}

public:

	static void aline_exponent(value& a1, value& a2)
	{
		i32 edelta = a1.get_exponent() - a2.get_exponent();
		if (edelta == 0)
			return;

		if (a1.get_exponent() == 0)
		{
			a2.make_zero_exponent();
			return;
		}
		if (a2.get_exponent() == 0)
		{
			a1.make_zero_exponent();
			return;
		}

		// to decrease exponent, need to mul-by-100
		// to increase exponent, need to div-by-100

		if (edelta < 0)
		{
			// either increase a1 exponent by (-edelta) or decrease a2 exponent by (-edelta)
			if (a2.int_size() < a1.frac_size())
			{
				// better to decrease a2 exponent due it means mul-by-100 and rise int part
				a2.mul_by_100(-edelta);
				a2.set_exponent(a1.get_exponent());
			}
			else {
				// better to increase a1 exponent due it means div-by-100 and rise frac part
				a1.div_by_100(-edelta);
				a1.set_exponent(a2.get_exponent());
			}
		} else {
			// either decrease a1 exponent by (edelta) or increase a2 exponent by (edelta)

			if (a1.int_size() < a2.frac_size())
			{
				// better to decrease a1 exponent due it means mul-by-100 and rise int part
				a1.mul_by_100(edelta);
				a1.set_exponent(a2.get_exponent());
			}
			else {
				// better to increase a2 exponent due it means div-by-100 and rise frac part
				a2.div_by_100(edelta);
				a2.set_exponent(a1.get_exponent());
			}

		}
		
	}

	static void mul(value& rv, const value& vv1, usingle vv2, usingle plus = 0)
	{
		if (vv2 == 0)
		{
			rv = value();
			if (plus != 0)
				rv.set_unsigned(plus);
			return;
		}

		const value::value_core* c1 = vv1.get_core();
		i32 expo = c1->exponent;

#ifdef MODE64
		std::vector<u8> mulbuf; mulbuf.resize(c1->size() + 10);
#else
		std::vector<u8> mulbuf; mulbuf.resize(c1->size() + 5);
#endif
		memset(mulbuf.data(), 0, mulbuf.size());

		size_t iput = mulbuf.size() - 1;
		size_t s1 = c1->size();

		udouble accum = plus;
		for (size_t i1 = 0; i1 < s1; ++i1, --iput)
		{
			size_t v1 = c1->from_end(i1);
			if (v1)
				math::mulplus(accum, vv2, v1);
			usingle rm;
			accum = math::div(accum, 100, &rm);
			mulbuf[iput] = tools::as_byte(rm);
		}
		for (; accum > 0; --iput)
		{
			if (accum < 100)
			{
				mulbuf[iput] = tools::as_byte(accum);
				break;
			}

			usingle rm;
			accum = math::div(accum, 100, &rm);
			mulbuf[iput] = tools::as_byte(rm);

		}

		size_t fracsize = c1->frac.size();
		size_t ipartsize = mulbuf.size() - fracsize;
		rv.alloc_int(ipartsize, mulbuf.data());
		rv.alloc_frac(fracsize, mulbuf.data() + ipartsize);
		rv.set_negative(c1->negative);
		rv.set_exponent(expo);
		rv.trunc();

	}

	static void mul(value& rv, const value& vv1, const value& vv2)
	{
		usingle uv;
		if (vv2.is_zero_frac() && vv2.to_unsigned(uv))
		{
			mul(rv, vv1, uv);
			if (vv2.is_negative())
				rv.minus();
			rv.set_exponent(vv1.get_exponent() + vv2.get_exponent());
			return;
		}

		if (vv1.is_zero_frac() && vv1.to_unsigned(uv))
		{
			mul(rv, vv2, uv);
			if (vv1.is_negative())
				rv.minus();
			rv.set_exponent(vv1.get_exponent() + vv2.get_exponent());
			return;
		}

		const value::value_core* c1 = vv1.get_core();
		const value::value_core* c2 = vv2.get_core();
		i32 expo = c1->exponent + c2->exponent;
		//std::wstring t1 = c1->to_string();
		//std::wstring t2 = c2->to_string();

		std::vector<u8> mulbuf; mulbuf.resize(c1->size() + c2->size());
		memset(mulbuf.data(), 0, mulbuf.size());
		size_t s1 = c1->size();

		for (signed_t i2 = 0, s2 = c2->size(); i2 < s2; ++i2)
		{
			size_t v2 = c2->from_end(i2);
			size_t iput = mulbuf.size() - i2 - 1;

			size_t accum = 0;
			for (size_t i1 = 0; i1 < s1; ++i1)
			{
				size_t v1 = c1->from_end(i1);
				accum += mulbuf[iput - i1] + v2 * v1;
				size_t keep = accum / 100;
				if (keep > 0)
					accum -= keep * 100;
				mulbuf[iput - i1] = tools::as_byte(accum);
				accum = keep;
			}
			if (accum > 0)
				mulbuf[iput - s1] = tools::as_byte(accum);
		}

		size_t fracsize = c1->frac.size() + c2->frac.size();
		size_t ipartsize = mulbuf.size() - fracsize;
		rv.alloc_int(ipartsize, mulbuf.data());
		rv.alloc_frac(fracsize, mulbuf.data() + ipartsize);

		//std::wstring rslt = rv.get_core()->to_string();

		rv.set_negative(c1->negative != c2->negative);
		rv.set_exponent(expo);
		rv.trunc();

	}

public:

	enum
	{
		P_UNDEFINED = 0,
		P_ABSOLUTE = -1,
	};


    struct value_core : ptr::sync_shared_object
    {
		value_core(u8 ipart, u8 fpart, signed_t clonef = 1);
        value_core(const std::wstring_view &num, size_t radix);
        value_core(const std::wstring_view &int_part, const std::wstring_view &frac_part, size_t radix);

        value_core(errset e, bool neg = false):error(e)
        {
			precision = P_UNDEFINED;
			negative = neg;
        }
		integer_t integer;
		frac_t frac;
		errset error = errset::OK;
		i32 exponent = 0; // x (100^exponent)
		signed precision : 15;
		bool negative : 1;

        value_core * clone(size_t cloneoptions) const
        {
            value_core * r = new value_core(error);
			if (0 == (CO_CLEAR_INT & cloneoptions))
				r->integer = integer;
			if (0 == (CO_CLEAR_FRAC & cloneoptions))
				r->frac = frac;
			r->exponent = exponent;
			r->precision = precision;
			r->negative = negative;
            return r;
        }

		signed_t int_size() const
		{
			for (signed_t i = 0, cnt = integer.size(); i < cnt; ++i)
			{
				if (integer[i] > 0)
					return cnt - i;
			}
			return 0;
		}


        size_t size() const
        {
            return integer.size() + frac.size();
        }

        size_t from_end(size_t index) const
        {
            if (index >= frac.size())
            {
                index -= frac.size();
                if (index >= integer.size())
                    return 0;

                return integer[integer.size()-index-1];
            }
            return frac[frac.size() - index-1];
        }

		u8 getu8(signed_t i) const // -big .. -1 - get int part, 0 .. big - get frac part
		{
			if (i >= 0)
			{
				if (i >= (signed_t)frac.size())
					return 0;
				return frac[i];
			}
			signed_t ii = integer.size() + i;
			if (ii < 0)
				return 0;
			return integer[ii];
		}

        std::wstring to_string_10(signed_t precision) const;

		bool is_zero_frac() const
		{
			for (signed_t i = 0, cnt = frac.size(); i < cnt; ++i)
			{
				if (frac[i] > 0)
					return false;
			}
			return true;
		}

		signed_t frac_size() const
		{
			for (signed_t i = frac.size()-1; i >= 0; --i)
			{
				if (frac[i] > 0)
					return i + 1;
			}
			return 0;
		}


    };
private:
    ptr::shared_ptr<value_core> core;

	value(value_core *c) :core(c) {}

public:
    value(errset e = errset::OK, bool neg = false) { core = new value_core(e, neg); }
    value(const value &v):core(v.core) {}
    value(value &&v) noexcept { core = v.core; v.core = nullptr; }

    value(const std::wstring_view &num, size_t radix) { core = new value_core(num, radix); }
    value(const std::wstring_view &int_part, const std::wstring_view &frac_part, size_t radix) { core = new value_core(int_part, frac_part, radix); }
	value(u8 ipart, u8 fpart, signed_t fracclone = 1) { core = new value_core(ipart, fpart, fracclone); }


    virtual ~value() {}

    value & operator=(const value &v)
    {
        core = v.core;
		return *this;
    }
    value & operator=(value &&v) noexcept
    {
        core = v.core;
        v.core = nullptr;
		return *this;
    }

	signed_t get_flat(bvec& b) const
	{
		signed_t isz = int_size();
		signed_t fsz = frac_size();
		
		signed_t prereserve = 1;
		signed_t is = core->integer.size() - isz;
		if (isz > 0 && core->integer[is] < 10)
			--prereserve;

		signed_t sz = 2 * (isz + fsz) + prereserve;
		b.resize(sz);
		for (signed_t i = 0; i < isz; ++i)
		{
			u8 n = core->integer[i + is];
			u8 tens = n / 10;
			u8 rem = n - tens * 10;
			b[prereserve + i * 2] = tens;
			b[prereserve + i * 2 + 1] = rem;
		}
		signed_t os = prereserve + isz * 2;
		for (signed_t i = 0; i < fsz; ++i)
		{
			u8 n = core->frac[i];
			u8 tens = n / 10;
			u8 rem = n - tens * 10;
			b[os + i * 2] = tens;
			b[os + i * 2 + 1] = rem;
		}

		return os;

	}


	void move_integer_part(integer_t& intp)
	{
		if (core->is_multi_ref())
			__debugbreak();
		intp = std::move(core->integer);
	}

	errset error() const
    {
        return core->error;
    }

	bool is_error() const
	{
		errset e = error();
		return e != errset::OK && e != errset::INF;
	}

	value &bypass()
	{
		if (!is_error())
		{
			if (core->is_multi_ref())
				core = core->clone(0);
			core->error = errset::BYPASS;
		}
		return *this;
	}

	value &unbypass()
	{
		if (error() == errset::BYPASS)
		{
			if (core->is_multi_ref())
				core = core->clone(0);
			core->error = errset::OK;
		}
		return *this;
	}

	value clone_int() const
	{
		return value(core->clone(CO_CLEAR_FRAC));
	}
	value clone_frac() const
	{
		return value(core->clone(CO_CLEAR_INT));
	}

	value &round(signed_t cprec, bool force_not_absolute = false);

	value& remove_frac()
	{
		if (is_zero_frac())
			return *this;

		if (core->is_multi_ref())
		{
			core = core->clone(CO_CLEAR_FRAC);
			core->precision = P_ABSOLUTE;
			return *this;
		}
		core->frac.clear();
		core->precision = P_ABSOLUTE;
		return *this;
	}

	signed_t get_precision() const
	{
		return is_zero_frac() ? P_ABSOLUTE : core->precision;
	}

	bool precision_better_or_equal_then(signed_t opr)
	{
		signed_t p = get_precision();
		if (p == opr || p == P_ABSOLUTE)
			return true;
		if (p >= 0 && opr >= 0)
			return p >= opr;
		return false;
	};


    const value_core *get_core() const
    {
        return core;
    }

	void make_zero_exponent()
	{
		if (core->exponent > 0)
		{
			mul_by_100(core->exponent);
			core->exponent = 0;
		}
		else if (core->exponent < 0)
		{
			div_by_100(-core->exponent);
			core->exponent = 0;
		}
	}

	signed_t mul_by_100_until_non_zero_int();
	signed_t div_by_100_until_zero_int();
	void div_by_100(signed_t ntimes); // shifts int and frac parts (not changes exponent field)
	void mul_by_100(signed_t ntimes); // shifts int and frac parts (not changes exponent field)

	value& div_by_2_int()
	{
		calc_div_int(*this, 2);
		return *this;
	}

	value & inc()
	{
		if (core->is_multi_ref())
			core = core->clone(0);

		if (is_zero_int())
		{
			core->integer.resize(1);
			core->integer[0] = 1;
		}
		else
		{
            if (core->negative)
            {
                value one(1, 0), me(*this);
                plus(*this, me, one);

            } else
			    add(core->integer, 1);
		}
		return *this;
	}

	value & add(u64 v)
	{
		if (is_negative())
		{
			value x, me(*this);
			x.set_unsigned(v);
			plus(*this, me, x);
		}
		else
		{
			if (core->is_multi_ref())
				core = core->clone(0);

			add(core->integer, v);
		}
		return *this;
	}

    value &sub(u64 v)
    {
        value vv, me(*this);
        vv.set_unsigned(v);
        minus(*this, me, vv);
        return *this;
    }

    void set_zero()
    {
        if (core->is_multi_ref())
            core = core->clone(CO_CLEAR_INT | CO_CLEAR_FRAC);
        else
        {
            core->integer.clear();
            core->frac.clear();
        }
        core->precision = 0;
        core->error = errset::OK;
    }

	value &set_unsigned(u64 v) // v is unsigned!
	{
		if (core->is_multi_ref())
			core = core->clone(CO_CLEAR_INT|CO_CLEAR_FRAC);
        else
            core->frac.clear();

		core->integer.resize(10);
        core->negative = false;
        core->precision = 0;
		set_unsigned(core->integer, v);
		trunc_left(core->integer);
		return *this;
	}

	/*
	void set_frac(u8 v)
	{
		if (core->is_multi_ref())
			core = core->clone();
		core->frac[core->frac.size()-1] = v;
	}
	*/

	value &append_frac(u8 v)
	{
		if (core->is_multi_ref())
			core = core->clone(0);
		core->frac.push_back(v);
		return *this;
	}
	value &append_frac(u8 v, signed_t clonen)
	{
		if (core->is_multi_ref())
			core = core->clone(0);
		core->frac.insert(core->frac.end(), clonen, v);
		return *this;
	}

	/*
	bool is_one() const
	{
		if (core->frac.size() == 0)
		{
			if (core->integer.size() > 0 && core->integer[core->integer.size() - 1] == 1)
			{
				for (signed_t i = 0, cnt = core->integer.size()-1; i < cnt; ++i)
				{
					if (core->integer[i] > 0)
						return false;
				}
				return true;
			}
		}
		return false;
	}
	*/

	signed_t int_size() const
	{
		return core->int_size();
	}

	bool is_zero_int() const
	{
		return core->int_size() == 0;
	}

    bool is_zero_frac() const
    {
        return core->is_zero_frac();
    }

	signed_t frac_size() const
	{
		return core->frac_size();
	}

	bool is_zero() const
	{
		return !is_infinity() && is_zero_int() && is_zero_frac();
	}

	bool equals(const value& ov) const
	{
		signed_t mi = -max((signed_t)core->integer.size(), (signed_t)ov.core->integer.size());
		signed_t ma = max(core->frac.size(), ov.core->frac.size());
		for (; mi < ma; ++mi)
		{
			u8 v1 = core->getu8(mi);
			u8 v2 = ov.core->getu8(mi);
			if (v1 > v2)
				return false;
			if (v1 < v2)
				return false;
		}

		return core->exponent == ov.core->exponent;
	}

	signed_t compare(const value &ov, signed_t precision) const
	{
		signed_t mi = -max((signed_t)core->integer.size(), (signed_t)ov.core->integer.size());
		signed_t ma = max(core->frac.size(), ov.core->frac.size());
		if (ma > precision)
			ma = precision;
		for (;mi < ma; ++mi)
		{
			u8 v1 = core->getu8(mi);
			u8 v2 = ov.core->getu8(mi);
			if (v1 > v2)
				return 1;
			if (v1 < v2)
				return -1;
		}
		return 0;
	}

	signed_t compare_tail(const value &ov, signed_t from = math::minimum<signed_t>::value) const // calc eqsz: <0 - not equals, 0 - equals in part, >0 num 100based digits equal in frac part, >= 1000000 - absolutly equals
	{
		signed_t mi = math::nmax(from, -math::nmax((signed_t)core->integer.size(), (signed_t)ov.core->integer.size()));
		signed_t ma = math::nmax(core->frac.size(), ov.core->frac.size());

		if (mi >= 0)
		{
			// compare just frac part

			frac_t& f1 = core->frac;
			frac_t& f2 = ov.core->frac;
			const u8* s1 = f1.data() + mi;
			const u8* e1 = f1.data() + ma;
			const u8* s2 = f2.data() + mi;
			for (; s1 < e1; ++s1, ++s2, ++mi)
			{
				if (*s1 != *s2)
					return mi;
			}

		} else for (; mi < ma; ++mi)
		{
			u8 v1 = core->getu8(mi);
			u8 v2 = ov.core->getu8(mi);
			if (v1 != v2)
				return mi;

		}
		return INFINITY_PRECISION;
	}

	void calc_div_impl(value& r, const value& divider, signed_t precision) const; // calc this/divider (implementation of common case)
	void calc_div(value &r, const value &divider, signed_t precision) const; // calc this/divider
	void calc_div(value &r, usingle divider, signed_t precision) const; // calc this/divider
	void calc_div_int(value& r, usingle divider) const; // calc this/divider without frac (integer division)

	void calc_inverse(value &r, signed_t precision) const; // calc 1/this

	void calc_shift_left(value &r, const value &svu) const; // calc binary shift left by svu (ignore signe, ignore frac)
	void calc_shift_right(value &r, const value &svu) const; // calc binary shift rite by svu (ignore signe, ignore frac)
	void calc_shift_left(value& r, usingle svuu) const; // calc binary shift left by svuu (ignore signe, ignore frac)
	void calc_shift_right(value& r, usingle svuu) const; // calc binary shift rite by svuu (ignore signe, ignore frac)
	
	/*
		old implementation
		slower then new calc_inverse
	*/
	//value calc_inverse2() const; // calc 1/this

	value & clamp_frac(signed_t precision, bool force_not_absoltue = false)
	{
		signed_t fracsize = frac_size();
		if (force_not_absoltue || fracsize > precision)
		{
			if (core->is_multi_ref())
				core = core->clone(0);
            core->precision = precision;
            core->frac.resize(precision);
		} else
		{
			// don't clone due value not changed, only meta info precision
			//if (core->is_multi_ref())
			//	core = core->clone(0);
			core->precision = P_ABSOLUTE;
            core->frac.resize(fracsize); // just cut useless zeros
		}
		return *this;
	}

	value & minus() // just change sign
	{
		if (core->is_multi_ref())
			core = core->clone(0);
		core->negative = !core->negative;
		return *this;
	}

	void set_negative(bool neg)
	{
		if (core->is_multi_ref())
			core = core->clone(0);
		core->negative = neg;
	}

	void set_exponent(i32 expo)
	{
		if (core->is_multi_ref())
			core = core->clone(0);
		core->exponent = expo;
	}

    bool is_negative() const
    {
        return core->negative;
    }

	bool is_infinity() const
	{
		return core->error == errset::INF;
	}

	i32 get_exponent() const
	{
		return core->exponent;
	}


    integer_t & alloc_int(size_t sz, const u8 *filler)
    {
        if (core->is_multi_ref())
            core = core->clone(CO_CLEAR_INT);
        core->integer.resize(sz);
		if (filler != nullptr)
			memcpy(core->integer.data(), filler, sz);
		return core->integer;
    }
    frac_t & alloc_frac(size_t sz, const u8 *filler)
    {
        if (core->is_multi_ref())
            core = core->clone(CO_CLEAR_FRAC);
        core->frac.resize(sz);
		if (filler != nullptr)
			memcpy(core->frac.data(), filler, sz);
        return core->frac;
    }

	std::wstring to_string(signed_t radix, signed_t precision) const;

    bool to_unsigned(usingle &) const; // convert only integer part, returns false, if size of size_t is not enough
	void to_unsigned(std::vector<usingle>&) const; // convert only integer part. generate array of values in little endian

	value &trunc()
	{
		// no need copy-on-demand cloning due no value changed

		signed_t x = core->frac.size() - 1;
		signed_t xx = x;
		for (; x >= 0 && core->frac[x] == 0; --x);
		if (x < xx)
			core->frac.resize(x + 1);

		trunc_left(core->integer);

		return *this;
	}
};


class calculating_value : public ptr::sync_shared_object_ck<ptr::RELEASER>
{
	value val;
	
	mutable std::mutex lock;
	mutable std::condition_variable wait;
	
	volatile bool intermedate_result_ready = false;
	volatile bool final_result_ready = false;
	volatile bool released = false;

protected:
	virtual void die()
	{
		delete this;
	}

public:

	//bool final_result_copied = false;

	virtual ~calculating_value();
	calculating_value(errset e = errset::CALCULATING)
	{
		val = value(e);
		if (e == errset::OK || e == errset::INF)
			final_result_ready = true;
	}
	calculating_value(const value &v)
	{
		val = v;
		final_result_ready = true;
	}

	/*
	void clear_intermediate_ready()
	{
		std::unique_lock<std::mutex> m(lock);
		intermedate_result_ready = false;
	}
	*/

	value wait_for_any_result(const value &prevv)
	{
		std::unique_lock<std::mutex> m(lock);
		for (;;)
		{
			if (intermedate_result_ready)
			{
				value v = val;
				intermedate_result_ready = false;
				return v;
			}
			if (final_result_ready)
			{
				value v = val;
				return v;
			}
			if (prevv.error() == errset::CALCULATING)
				wait.wait(m);
			else
				return prevv;
		}
	}

	value get_value() const
	{
		std::unique_lock<std::mutex> m(lock);
		value v = val;
		return v;
	}

	void set_intermediate_result(const value &v)
	{
		std::unique_lock<std::mutex> m(lock);
		val = v;
		intermedate_result_ready = true;
		wait.notify_all();
	}

	void set_final_result(const value& v);

	bool is_final_result_ready() const
	{
		return final_result_ready;
	}

	bool is_intermedate_result_ready() const
	{
		return intermedate_result_ready;
	}

	void release();

	bool is_released() const
	{
		return released;
	}
};


INLINE signed_t optimal_precision(signed_t vp, signed_t rp)
{
	if (vp == value::P_ABSOLUTE)
		return rp * 2;
	return math::minv(vp, rp * 2);
}




