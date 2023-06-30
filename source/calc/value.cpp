#include "pch.h"

std::wstring error_text(errset er)
{
	switch (er)
	{
	case errset::OK:
	case errset::EMPTY:
	case errset::CALCULATING:
	case errset::BYPASS:
	case errset::STOP:
		return std::wstring();
	case errset::BAD_ARGS_NUM:
		return std::wstring(TRS("wrong number of arguments"));
	case errset::BAD_SYMBOLS_IN_EXPRESION:
		return std::wstring(TRS("wrong symbols in expression"));
	case errset::EXPRESSION_TOO_BIG:
		return std::wstring(TRS("expression too big"));
	case errset::RESULT_OVERFLOW:
		return std::wstring(TRS("overflow"));
	case errset::PARENTHESIS_FAIL:
		return std::wstring(TRS("parenthesis mismatch"));
	case errset::NUMBER_NOT_RECOGNIZED:
		return std::wstring(TRS("number not recognized"));
	case errset::UNKNOWN_RADIX:
		return std::wstring(TRS("unsupported radix"));
	case errset::EMPTY_SUBEXPRESSION:
		return std::wstring(TRS("empty sub-expression"));
	case errset::DIVISION_BY_ZERO:
		return std::wstring(TRS("division by zero"));
	case errset::XOVERFLOW:
		return std::wstring(TRS("overflow"));
	case errset::VARIABLE_NOT_FOUND:
		return std::wstring(TRS("variable not found"));
	case errset::OPERATOR_NOT_FOUND:
		return std::wstring(TRS("operator not found"));
	case errset::BAD_ARGUMENT:
		return std::wstring(TRS("bad argument"));
	}

	std::wstring es(TRS("error code "));
	es.append(std::to_wstring((int)er));

	return es;
}


static void to_string(std::wstring &outs, const std::vector<u8> &from, signed_t limit = 0)
{
	signed_t i = 0, cnt = limit == 0 ? from.size() : math::minv(from.size(), limit / 2);
    for (;i<cnt;++i)
    {
		u8 n = from[i];
        size_t hi = n / 10;
        size_t lo = n - (hi * 10);
        outs.push_back(tools::as_wchar(hi + '0'));
        outs.push_back(tools::as_wchar(lo + '0'));
    }
	if ((limit & 1) != 0 && i < (signed_t)from.size())
	{
		u8 n = from[i];
		if (n >= 10) // do not add tail zero
		{
			size_t hi = n / 10;
			outs.push_back(tools::as_wchar(hi + '0'));
		}
	}
}

static usingle hex16(const std::wstring_view& from)
{
	usingle v = 0;
	for (size_t i = 0, cnt = min(6, from.length()); i < cnt; ++i)
	{
		wchar_t c = from[i];
		if (c >= '0' && c <= '9')
			v = (v << 4) + (c - '0');
		else {
			c &= ~32; // case up
			if (c >= 'A' && c <= 'F')
				v = (v << 4) + (c - 'A' + 10);
		}
	}
	return v;
}

static errset parse(value::integer_t &to, const std::wstring_view &from, size_t radix, bool intpart)
{
    if (radix == 10)
    {
        size_t numhuns = (from.length() + 1) >> 1;
        to.resize(numhuns);
        
        if ((from.size() & 1) != 0)
        {
            --numhuns;
            if (intpart)
            {
                // insert zero at 1st pos
                // parse from end

                for (signed_t spos = from.size() - 2; spos >= 0; spos -= 2)
                    to[numhuns--] = tools::as_byte((from[spos] - 48) * 10 + (from[spos + 1] - 48));
                to[0] = tools::as_byte(from[0] - 48);

            }
            else
            {
                // append zero at end

                for (signed_t spos = 0, lim = (signed_t)from.size() - 2; spos < lim; spos += 2)
                    to[spos >> 1] = tools::as_byte((from[spos] - 48) * 10 + (from[spos + 1] - 48));
                to[numhuns] = tools::as_byte((from[from.size()-1] - 48) * 10);

            }
        }
        else
        {
            for (signed_t i = 0, l = from.length(); i < l; i += 2)
                to[i >> 1] = tools::as_byte((from[i] - 48) * 10 + (from[i + 1] - 48));
        }


        return errset::OK;
    }

	if (radix == 16)
	{
		if (intpart)
		{
			value hex;
			signed_t i = 6;
			signed_t l = from.length();
			for (; i < l; i += 6)
			{
				usingle x = hex16( from.substr(i-6, 6) );
				value::mul(hex, hex, 0xffffff+1, x);
			}
			i -= 6;
			if (i < l)
			{
				usingle x = hex16(from.substr(i));
				value::mul(hex, hex, (usingle)1 << (4*(l-i)), x);
			}
			hex.move_integer_part(to);
		}
		else
		{
			to.clear();
		}

		return errset::OK;
	}

    return errset::UNKNOWN_RADIX;
}

value::value_core::value_core(u8 ipart, u8 fpart, signed_t fc)
{
	precision = 0;
	negative = false;

	if (ipart)
	{
		integer.resize(1);
		integer[0] = ipart;
	}
	if (fpart)
	{
		frac.resize(fc);
		frac[0] = fpart;
		for (signed_t i = 1; i < fc; ++i)
			frac[i] = fpart;
	}
}

value::value_core::value_core(const std::wstring_view &num, size_t radix)
{
	precision = 0;
	negative = false;

    error = parse(integer, num, radix, true);
}

value::value_core::value_core(const std::wstring_view &int_part, const std::wstring_view &frac_part, size_t radix)
{
	precision = 0;
	negative = false;

    error = parse(integer, int_part, radix, true);
    if (errset::OK == error)
        error = parse(frac, frac_part, radix, false);
}

std::wstring value::value_core::to_string_10(signed_t prc) const
{
    std::wstring outs;
	
	signed_t i = 0;
	if (negative)
	{
		outs.push_back('-');
		++i;
	}

	if (integer.size() == 0)
		outs.push_back('0');
	else
		::to_string(outs, integer);

	bool expon = false;

	if (i+1 < (signed_t)outs.length() && outs[i] == '0' && is_digit(outs[i+1]))
		outs.erase(outs.begin() + i);

	if (prc < 0)
	{
		signed_t isz = outs.length() - i;
		if (!(outs[i] == '0' && frac.size() > 0 && frac[0] < 10))
		{
			//if (outs[i] == '0' && outs.length() > s)
			//{
				//outs.erase(outs.begin() + (s - 1));
				//--isz;
			//}
			outs.insert(outs.begin() + i + 1, L'.');
			signed_t tr = i-prc+2;
			if ((signed_t)outs.length() > tr)
				outs.resize(tr);
			
			if ((signed_t)outs.length() < tr)
			{
				::to_string(outs, frac, tr - outs.length());
				if (outs[outs.length() - 1] == '0')
					outs.resize(outs.length() - 1);
			}
			if (outs[outs.length()-1]=='.')
				outs.push_back('0');

			signed_t expn = isz - 1 + exponent * 2;
			if (expn < 0)
			{
				outs.append(std::wstring_view(L"E\u2212",2));
				expn = -expn;
			}
			else
			{
				outs.append(WSTR("E+"));
			}

			outs.append(std::to_wstring(expn));
			return outs;
		}
		prc = -prc;
		expon = true;
	}
	
	signed_t pti = outs.size();

    if (frac.size() > 0)
    {
		if (exponent == 0)
			outs.push_back('.');
		signed_t sf = outs.length();
        ::to_string(outs, frac, expon ? (1000) : prc);
        size_t lci = outs.size() - 1;
        while (outs[lci] == '0') --lci;
        outs.resize(lci + 1);

		if (expon && ((signed_t)outs.length()-sf) > 0)
		{
			if (exponent != 0)
			{
				outs.insert(outs.begin() + pti, '.');
				++sf;
			}

			if (outs[i+1] == '.' && outs[i] == '0')
			{
				signed_t numz = 0;
				for (signed_t x = sf, xx = outs.length(); x < xx; ++x)
					if (outs[x] == '0') ++numz; else break;

				if (numz >= 1)
				{
					// enough zeros

					outs.erase(i, numz+2);
					if (i + prc + 1 < (signed_t)outs.size())
						outs.resize(i + prc + 1);
					outs.insert(outs.begin() + i+1, L'.');
					if (outs[outs.length() - 1] == '.')
						outs.push_back('0');
					signed_t expn = -(numz + 1) + exponent * 2;
					if (expn < 0)
					{
						outs.append(std::wstring_view(L"E\u2212", 2));
						expn = -expn;
					}
					else
					{
						outs.append(WSTR("E+"));
					}
					outs.append(std::to_wstring(expn));
					return outs;
				}
			}
		}

		if (expon && ((signed_t)outs.length() - sf) > prc)
		{
			outs.resize(sf + prc);
		}
    }

	if (exponent > 0)
	{
		pti += exponent * 2;
		if (pti < (signed_t)outs.size())
			outs.insert(outs.begin()+pti, '.');
		else if (pti > (signed_t)outs.size()) {
			outs.append(pti - outs.size(), '0');
		}
	}
	else if (exponent < 0)
	{
		pti += exponent * 2;
		if (pti > 0)
			outs.insert(outs.begin() + pti, '.');
		else if (pti == 0)
		{
			outs.insert(0, WSTR("0."));

		} else if (pti < 0) {
			outs.insert(0, WSTR("0."));
			outs.insert(2, -pti, L'0');
		}
	}

	signed_t j = i;
	for (; outs[j] == '0';) ++j;
	if (j >= (signed_t)outs.length() || outs[j] == '.')
		--j;
	if (j > i)
		outs.erase(i, j - i);

    return outs;
}

value operator*(const value &vv1, const value &vv2)
{
	value rv;
	value::mul(rv, vv1, vv2);
	return rv;
}

value operator+(const value &vv1, const value &vv2)
{
	value rv;
	value::plus(rv, vv1, vv2);
	return rv;
}

value operator+(const value &vv1, u64 vv2)
{
	value rv(vv1);
	rv.add(vv2);
	return rv;
}

value operator-(const value &vv1, const value &vv2)
{
	value rv;
	value::minus(rv, vv1, vv2);
	return rv;
}

value &value::round(signed_t cprec, bool force_not_absolute)
{
	clamp_frac(cprec, force_not_absolute);
	if (cprec > 0 && (signed_t)core->frac.size() >= cprec)
	{
		bool r99 = true;
        signed_t i = cprec - 1;
		for (; i>=0; --i)
		{
			if (core->frac[i] != 99)
			{
				r99 = false;
				break;
			}
		}
		if (r99)
		{
			if (core->is_multi_ref())
				core = core->clone(CO_CLEAR_FRAC);
			core->frac.clear();
			add(core->integer, 1);
			core->precision = P_ABSOLUTE;
        }
        else
        {
            signed_t num99 = cprec - i;
            if (num99 > 10)
            {
                // 20 digital signs are 9's => enough precision to round

                if (core->is_multi_ref())
                    core = core->clone(0);
                ++core->frac[i]; // was less then 99, so it can be safely increment
                core->frac.resize(i + 1); // cut all 99's
				core->precision = P_ABSOLUTE;
            }
        }
	}
	signed_t clamp = -1;
	for (signed_t i = core->frac.size() - 1; i >= 0; --i)
	{
		if (core->frac[i] == 0)
			clamp = i;
		else break;
	}
	if (clamp >= 0)
		core->frac.resize(clamp);
	if (is_zero() && is_negative())
		set_negative(false);


	return *this;
}

signed_t value::mul_by_100_until_non_zero_int()
{
	if (!is_zero_int())
		return 0;
	for (signed_t x = 0, cnt = core->frac.size(); x < cnt; ++x)
	{
		if (core->frac[x] != 0)
		{
			if (core->is_multi_ref())
				core = core->clone(CO_CLEAR_INT);

			core->integer.resize(1);
			core->integer[0] = core->frac[x];
			core->frac.erase(core->frac.begin(), core->frac.begin() + x + 1);
			return x + 1;
		}
	}
	return -1;
}

signed_t value::div_by_100_until_zero_int()
{
	signed_t intsize = int_size();
	if (core->is_multi_ref())
	{
		value_core *oc = core;
		core = core->clone(CO_CLEAR_INT|CO_CLEAR_FRAC);
		core->frac.insert(core->frac.begin(), oc->integer.end() - intsize, oc->integer.end());
		if (oc->frac.size() > 0)
			core->frac.insert(core->frac.end(), oc->frac.begin(), oc->frac.end());
		return intsize;
	}

	core->frac.insert(core->frac.begin(), core->integer.end() - intsize, core->integer.end());
	core->integer.clear();
	return intsize;
}


void value::div_by_100(signed_t ntimes)
{
	if (core->is_multi_ref())
		core = core->clone(0);

	if (core->integer.size() > 0)
	{
		signed_t nn = min((signed_t)core->integer.size(), ntimes);
		auto r1 = core->integer.end() - nn;
		core->frac.insert(core->frac.begin(), r1, core->integer.end());
		core->integer.resize(core->integer.size() - nn);
		ntimes -= (int)nn;
	}
	if (ntimes)
		core->frac.insert(core->frac.begin(), ntimes, 0);
}

void value::mul_by_100(signed_t ntimes)
{
	if (core->is_multi_ref())
		core = core->clone(0);

	if (core->frac.size() > 0)
	{
		signed_t nn = min((signed_t)core->frac.size(), ntimes);
		if (nn == 1)
		{
			u8 f0 = core->frac[0];
			if (f0 != 0 || !is_zero_int())
				core->integer.push_back(f0);
		}
		else
		{
			core->integer.insert(core->integer.end(), core->frac.begin(), core->frac.begin() + nn);
		}
		core->frac.erase(core->frac.begin(), core->frac.begin() + nn);
		ntimes -= (int)nn;
	}
	if (ntimes && !is_zero_int())
		core->integer.insert(core->integer.end(), ntimes, 0);
}

namespace
{
	template <int ssz> struct maxvdgd;
	template <> struct maxvdgd<8> {
		inline static u8 maxsigned[] = { 18, 44, 67, 44, 7, 37, 9 , 55, 16, 15 }; // 18 446 744 073 709 551 615 - maximum 64-bit unsigned value
		inline signed_t operator()() { return sizeof(maxsigned); }
		inline u8 operator[](signed_t i) const { return maxsigned[i]; }
	};
	template <> struct maxvdgd<4> {
		inline static u8 maxsigned[] = { 42, 94, 96, 72, 95 }; // 4 294 967 295 - maximum 32-bit unsigned value
		inline signed_t operator()() { return sizeof(maxsigned); }
		inline u8 operator[](signed_t i) const { return maxsigned[i]; }
	};
}

void long_mul100(std::vector<usingle>&)
{
	__debugbreak();
}

void long_add(std::vector<usingle>& ar, usingle val)
{
	usingle maxv = (usingle)(-1);
	usingle t = maxv - val;
	usingle addnext = val;
	for (signed_t i = 0, cnt = ar.size(); i < cnt; ++i)
	{
		usingle cv = ar[i];
		ar[i] = cv + addnext;
		if (cv <= t)
		{
			addnext = 0;
			break;
		}
		addnext = 1;
	}
	if (addnext > 0)
		ar.push_back(addnext);
}

void value::to_unsigned(std::vector<usingle>&ar) const
{
	ar.clear();
	const value::value_core* c = core;
	for (signed_t i = 0, cnt = c->integer.size(); i < cnt; ++i)
	{
		if (c->integer[i] > 0)
		{
			for (; i < cnt; ++i)
			{
				long_mul100(ar);
				long_add(ar, c->integer[i]);
			}
			return;
		}
	}

}

std::wstring value::to_string(signed_t radix, signed_t precision) const
{
	if (radix == 10)
		return core->to_string_10(precision);

	if (radix == 16)
	{

		std::wstring outs;

		if (core->integer.size() == 0)
		{
			outs.push_back('0');
			return outs;
		}

		wchar_t hexar[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
		value r0, r1(*this), r2;
		r1.make_zero_exponent();

		wchar_t hex16[4];

		for (;!r1.is_zero_int(); r1 = r0)
		{
			r2 = r1;
			r1.calc_shift_right(r0, 16);
			r0.calc_shift_left(r1, 16);
			usingle x;
			(r2 - r1).to_unsigned(x);
			if (x >= 65536)
			{
				outs = WSTR("internal error");
				break;
			}
			hex16[0] = hexar[(x >> 12) & 0xf];
			hex16[1] = hexar[(x >> 8) & 0xf];
			hex16[2] = hexar[(x >> 4) & 0xf];
			hex16[3] = hexar[(x >> 0) & 0xf];
			outs.insert(0, hex16, 4);
		}

		outs.erase(outs.begin(), std::find_if(outs.begin(), outs.end(), [](wchar_t ch) {
			return ch != '0';
		}));

		if (outs.empty())
			outs.append(WSTR("0x0"));
		else
			outs.insert(0, WSTR("0x"));

		if (is_negative())
			outs.insert(outs.begin(), '-');

		return outs;

	}
	
	return std::wstring(WSTR("unsupported radix"));
}


bool value::to_unsigned(usingle &r) const
{
	maxvdgd<sizeof(usingle)> maxs;

    r = 0;
    const value::value_core *c = core;
    for (signed_t i = 0, cnt = c->integer.size(); i < cnt; ++i)
    {
        if (c->integer[i] > 0)
        {
            if ((cnt - i) > maxs())
                return false;
            if ((cnt - i) == maxs())
            {
                for (signed_t j = i; j < cnt; ++j)
                {
                    u8 v = c->integer[j];
                    u8 vm = maxs[j - i];
                    if (v > vm)
                        return false;
                    if (v < vm)
                        break;
                }
            }

            usingle x = 0;
            for (; i < cnt; ++i)
                x = x * 100u + c->integer[i];
            r = x;
            return true;
        }
    }
    return true;
}

void value::calc_div(value &rslt, const value &divider, signed_t precision) const
{
	// check equals
	if (equals(divider))
	{
		rslt = value(1, 0);
		return;
	}

	signed_t minprec = int_size() + frac_size() + divider.int_size() + divider.frac_size();
	if (precision < minprec)
		precision = minprec;

	usingle val;
	if (divider.is_zero_frac() && divider.to_unsigned(val)) // simple case: divider is integer (frac part is empty)
	{
		calc_div(rslt, val, precision + 1);
		if (divider.is_negative())
			rslt.minus();
		rslt.set_exponent(rslt.get_exponent() - divider.get_exponent());
		return;
	}

	signed_t fsz = divider.frac_size();
	if (fsz + divider.int_size() <= 20) // hardest case: remove frac part by multiply by 100 frac_size times
	{
		value d(divider);
		d.mul_by_100(fsz); // remove frac part
		if (d.to_unsigned(val)) // so, check it can be converted to u64
		{
			calc_div(rslt, val, precision + 1); // use integer division
			if (divider.is_negative())
				rslt.minus();
			//rslt.mul_by_100(fsz);
			rslt.set_exponent(fsz + get_exponent() - divider.get_exponent());
			rslt.clamp_frac(precision);
			return;
		}
	}

	// common case - multiply by inversed divider

	value invv;
	divider.calc_inverse(invv, precision+1);
	rslt = *this * invv;
	rslt.clamp_frac(precision);
}


void value::calc_div(value &rslt, usingle divider, signed_t precision) const
{
    if (divider == 0)
    {
        rslt = value(errset::INF);
        return;
    }
    if (divider == 1)
    {
        rslt = *this;
        return;
    }
    if (divider == 100)
    {

        rslt = *this;
		rslt.set_exponent(rslt.get_exponent() - 1);
		//rslt.div_by_100(1);
        return;
    }

	value keep_self(*this);

    std::vector<u8> *buf = &rslt.alloc_int(0, nullptr);
    std::vector<u8> &frac = rslt.alloc_frac(0, nullptr);

    udouble d = 0;
    const value::value_core *c = keep_self.core;
    const std::vector<u8> *src = &c->integer;
    signed_t i = 0, cnt = c->integer.size();
    for (; i < cnt; ++i)
        if (c->integer[i] > 0)
            break;

    bool infrac = false;
    if (i == cnt)
    {
        src = &c->frac;
        buf = &frac;
        infrac = true;
        i = 0;
        cnt = c->frac.size();
    }

    for (;(signed_t)frac.size() < precision;++i)
    {
        u8 e = 0;
        if (i >= cnt && !infrac)
        {
            src = &c->frac;
            buf = &frac;
            infrac = true;
            i = 0;
            cnt = c->frac.size();
        }
        if (i < cnt)
            e = (*src)[i];

		math::mul100add(d, e); //d = d * 100u + e;
        if (d >= divider)
        {
			udouble x = math::div(d, divider); //d / divider;
            buf->push_back(tools::as_byte(x));
			math::mul(x, divider);
            d -= x;
            if (d == 0 && infrac && i>=cnt)
            {
				rslt.set_exponent(get_exponent());
                rslt.clamp_frac(precision);
				rslt.set_negative(is_negative());
                return;
            }
        }
        else if (infrac || buf->size() > 0)
        {
            buf->push_back(0);
        }

    }
	rslt.set_exponent(get_exponent());
    rslt.clamp_frac(precision);
	rslt.set_negative(is_negative());
}

void value::calc_div_int(value& rslt, usingle divider) const
{
	if (divider == 0)
	{
		rslt = value(errset::DIVISION_BY_ZERO);
		return;
	}
	if (divider == 1)
	{
		rslt = *this;
		rslt.remove_frac();
		return;
	}
	if (divider == 100)
	{
		rslt = *this;
		rslt.div_by_100(1);
		rslt.remove_frac();
		return;
	}

	value keep_self(*this);

	std::vector<u8>* buf = &rslt.alloc_int(0, nullptr);
	rslt.remove_frac();

	udouble d = 0;
	const value::value_core* c = keep_self.core;
	const std::vector<u8>* src = &c->integer;
	signed_t i = 0, cnt = c->integer.size();
	for (; i < cnt; ++i)
		if (c->integer[i] > 0)
			break;

	if (i == cnt)
	{
		// zero this - zero result
		return;
	}

	for (;; ++i)
	{
		u8 e = 0;
		if (i >= cnt)
		{
			rslt.set_negative(is_negative());
			return;
		}
		if (i < cnt)
			e = (*src)[i];

		math::mul100add(d, e); //d = d * 100u + e;
		if (d >= divider)
		{
			udouble x = math::div(d, divider); //d / divider;
			buf->push_back(tools::as_byte(x));
			math::mul(x, divider);
			d -= x;
		}

	}
	//rslt.set_negative(is_negative());
}

void value::calc_inverse(value &rslt, signed_t precision) const
{
    rslt.set_zero();
	value v = *this;
	signed_t muln = v.mul_by_100_until_non_zero_int();
	if (muln < 0)
	{
		// zero divider
        rslt = value(errset::DIVISION_BY_ZERO);
        return;
	}

	muln -= v.div_by_100_until_zero_int();

	value muls[100] = { value(0,0), v };
	for (int i = 2; i < 100; ++i)
		plus(muls[i], muls[i - 1], v);

	value cmp(1, 0);
	float dch = (float)(1.0 / (double)(v.get_core()->getu8(0) * 100 + v.get_core()->getu8(1)));
	for (;;)
	{
		int chd = cmp.get_core()->getu8(-1) * 10000 + cmp.get_core()->getu8(0) * 100 + cmp.get_core()->getu8(1);
		int index = math::fround(dch * chd); // non zero due preparations (muls and divs by 100)

		if (index >= 100)
			index = 99;
		ASSERT(index >= 0);
		signed_t cmpv1 = muls[index].compare(cmp, precision);
		if (cmpv1 == 0)
		{
			rslt.append_frac(tools::as_byte(index));
			break;
		}

		if (index == 0 && cmpv1 < 0)
		{
			rslt.append_frac(0);
			if ((signed_t)rslt.core->frac.size() > precision)
			{
				rslt.core->frac.resize(precision);
				rslt.core->precision = precision;
				break;
			}
			cmp.mul_by_100(1);
			continue;
		}
		ASSERT(index >= 0 && index < 100);
		signed_t cmpv2 = muls[index-1].compare(cmp, precision);
		if (cmpv2 == 0)
		{
			rslt.append_frac(tools::as_byte(index-1));
			break;
		}
		while (cmpv1 == cmpv2)
		{
			if (cmpv1 < 0)
			{
				// both less: means need check smthing above
				++index;

				if (index == 100)
				{
					cmpv1 = 1;
					break;
				}

				cmpv1 = muls[index].compare(cmp, precision);
			}
			else if (cmpv1 > 0)
			{
				--index;
				cmpv2 = muls[index-1].compare(cmp, precision);
			}
		}
		if (cmpv1 == 1 && cmpv2 == -1)
		{
			rslt.append_frac(tools::as_byte(index - 1));
			if ((signed_t)rslt.core->frac.size() > precision)
			{
				rslt.core->frac.resize(precision);
				rslt.core->precision = precision;
				break;
			}
			value cmp2 = cmp - muls[index - 1];
			cmp2.mul_by_100(1);
			cmp = cmp2;
		}
		else
			__debugbreak();
	}

	if (muln >= 0)
	{
		rslt.mul_by_100(muln+1);
	}
	else if (muln < -1)
	{
		rslt.div_by_100(-muln-1);
	}
	rslt.set_exponent(-get_exponent());
}

#if 0
value value::calc_inverse2() const
{
	value v = *this;
	int muln = v.mul_by_100_until_non_zero_int();
	if (muln < 0)
	{
		// zero divider
		return value(E_DIVISION_BY_ZERO);
	}
	value m(0,50);
	u8 low = 0;
	u8 check = 50;
	u8 hi = 100;
	for (;m.get_core()->frac.size() < 100;)
	{
		value r = v * m;
		if (r.is_one())
			break;

		//std::wstring n = r.to_string();
		//std::wstring dd = m.to_string();

		if (r.is_zero_int())
		{
			low = check;
			if (hi == low+1)
			{
				// found
				m.set_frac(low);
				m.append_frac(50);

				low = 0;
				check = 50;
				hi = 100;
				continue;
			}
		}
		else
		{
			hi = check;
			if (hi - 1 == low)
			{
				// found
				m.set_frac(low);
				m.append_frac(50);

				low = 0;
				check = 50;
				hi = 100;
				continue;
			}
		}
		check = (hi + low) / 2;
		m.set_frac(check);

	}
	m.set_negative(core->negative);
	if (muln > 0)
		m.mul_by_100(muln);
	return m;
}
#endif

void value::calc_shift_left(value& r, const value& svu) const
{
	usingle svuu;
	if (svu.to_unsigned(svuu))
	{
		calc_shift_left(r, svuu);
		return;
	}

	r = value(errset::XOVERFLOW);
}

void value::calc_shift_left(value &r, usingle svuu) const
{
	if (svuu > CPNST_MAX_BINARY_SHIFT_VALUE)
	{
		r = value(errset::XOVERFLOW);
		return;
	}
	value m, m63(errset::CALCULATING), curm(errset::CALCULATING);
	for (;svuu > 0;)
	{
		if (svuu > 63)
		{
			svuu -= 63;
			if (m63.error() == errset::CALCULATING)
				m63.set_unsigned(1llu << 63);
			m = m63;
		}
		else
		{
			u64 muler = 1llu << svuu;
			svuu = 0;
			m.set_unsigned(muler);
		}

		if (curm.error() == errset::CALCULATING)
		{
			curm = *this * m;
		}
		else
		{
			curm = curm * m;
		}
	}
	r = curm;
}

void value::calc_shift_right(value& r, const value& svu) const
{
	usingle svuu;
	if (svu.to_unsigned(svuu))
	{
		calc_shift_right(r, svuu);
		return;
	}

	r = value(); // zero
}

void value::calc_shift_right(value &r, usingle svuu) const
{
	const signed_t archbitm1 = (ARCHBITS - 1);

	if (svuu > CPNST_MAX_BINARY_SHIFT_VALUE)
	{
		r = value();
		return;
	}

	value m, m63(errset::CALCULATING), curm(*this);
	for (usingle diver = 0; svuu > 0;)
	{
		if (svuu > archbitm1)
		{
			diver = 1llu << archbitm1;
			svuu -= archbitm1;
			if (m63.error() == errset::CALCULATING)
				m63.set_unsigned(diver);
			m = m63;

		}
		else
		{
			diver = 1llu << svuu;
			svuu = 0;
			m.set_unsigned(diver);
		}

		if (curm.compare(m, 0) >= 0)
		{
			curm.calc_div(curm, diver, 2);
			curm.clamp_frac(0);
		}
		else
		{
			r = value();
			return;
		}

	}

	r = curm;
}

calculating_value::~calculating_value()
{
}

void calculating_value::release()
{
	std::unique_lock<std::mutex> m(lock);

	if (final_result_ready)
	{
		m.unlock();
		die();
		return;
	}

	released = true;
}

void calculating_value::set_final_result(const value& v)
{
	std::unique_lock<std::mutex> m(lock);

	if (released)
	{
		m.unlock();
		die();
		return;
	}

	final_result_ready = true;
	val = v;
	if (val.is_zero() && val.is_negative())
		val.set_negative(false);
	wait.notify_all();
}
