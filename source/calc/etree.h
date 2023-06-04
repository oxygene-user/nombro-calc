#pragma once

struct node : ptr::sync_shared_object
{
	u8 calctag = op::calctag;
	bool prefix = false;
	virtual ~node() {}
	virtual ptr::shared_ptr<calculating_value> evaluate(signed_t precision) = 0;
	virtual errset error() const = 0;
	virtual void mutate() {} // node can mutate itself (generate it's own calc tree) to calc, using other operators. Example: x ^ y => exp (y * ln x)

	virtual bool absorb(std::vector<ptr::shared_ptr<node>>& /*heap*/, signed_t /*index*/) { return false; }

	bool is_error() const
	{
		errset e = error();
		return e != errset::OK && e != errset::INF;
	}

#ifdef _DEBUG
	virtual std::wstring info(int ots) = 0;
#endif
};

struct value_node : public node
{
	value v;
	value_node(const value& v) :v(v) {}
	value_node(const std::wstring_view& num, size_t radix) :v(num, radix) {}
	value_node(const std::wstring_view& int_part, const std::wstring_view& frac_part, size_t radix) :v(int_part, frac_part, radix) {}

	/*virtual*/ ptr::shared_ptr<calculating_value> evaluate(signed_t precision) override
	{
		ASSERT(op::calctag == calctag);

		value vv(v);
		vv.clamp_frac(precision);

		return new calculating_value(vv);
	}
	/*virtual*/ errset error() const override
	{
		return v.error();
	}
#ifdef _DEBUG
	/*virtual*/ std::wstring info(int /*ots*/) override
	{
		return v.to_string(10, 10);
	}
#endif
};

struct expression_node : public node
{
	ptr::shared_ptr<calculating_value> expression;
	expression_node(ptr::shared_ptr<calculating_value> e) :expression(e) {}

	/*virtual*/ ptr::shared_ptr<calculating_value> evaluate(signed_t /*precision*/) override
	{
		// do nothing. Expression should be already calc-in-progress
		return expression;
	}
	/*virtual*/ errset error() const override
	{
		errset e = expression->get_value().error();
		if (e == errset::CALCULATING)
			e = errset::OK;
		return e;
	}
#ifdef _DEBUG
	/*virtual*/ std::wstring info(int /*ots*/) override
	{
		return expression->get_value().to_string(10, 10);
	}
#endif
};

struct string_node : public node
{
	std::wstring str;
	bool variable = false;
	string_node(const std::wstring &s) :str(s) {}

	/*virtual*/ ptr::shared_ptr<calculating_value> evaluate(signed_t /*precision*/) override
	{
		// do nothing. Expression should be already calc-in-progress
		return new calculating_value();
	}
	/*virtual*/ errset error() const override
	{
		return errset::EMPTY;
	}
	/*virtual*/ bool absorb(std::vector<ptr::shared_ptr<node>>& heap, signed_t index) override;
#ifdef _DEBUG
	/*virtual*/ std::wstring info(int /*ots*/) override
	{
		return str;
	}
#endif
};


struct operator_node : public node
{
	ptr::shared_ptr<calculating_value> cv;
	std::vector<ptr::shared_ptr<node>> params;
	const ::op* op;
	bool prp = false;
	bool mutated = false;
	bool result_mark_bypass = false;
	bool result_unmark_bypass = false;
	operator_node(const ::op* o) :op(o) {}

#ifdef _DEBUG
	/*virtual*/ std::wstring info(int ots) override
	{
		std::wstring s(L"op: ");
		s.append(op->name()).append(L", ").append(std::to_wstring(params.size())).append(L" par(s)\r\n");
		int pn = 1;
		for (auto n : params)
		{
			s.append(ots+2, L' ').append(L"par ").append(std::to_wstring(pn)).append(L": ").append(n->info(ots+4));
			if (s[s.length() - 1] != '\n')
				s.append(L"\r\n");
			++pn;
		}
		return s;
	}
#endif

	/*virtual*/ void mutate()
	{
		if (mutated)
			return;

		const ::op *oop = this->op;
		op->mutate(this);
		if (oop == this->op)
		{
			// not mutated
			// iterate params
			for (auto& p : params)
				p->mutate();
			
		}

		mutated = true;
	}

	/*virtual*/ ptr::shared_ptr<calculating_value> evaluate(signed_t precision) override;

	/*virtual*/ bool absorb(std::vector<ptr::shared_ptr<node>>& heap, signed_t index) override;

	/*virtual*/ errset error() const override
	{
		for (const auto& p : params)
			if (p->is_error())
				return p->error();

		return errset::OK;
	}
	void add_par(ptr::shared_ptr<node> par, bool is_prefix)
	{
		params.emplace_back(par);
		params[params.size() - 1]->prefix = is_prefix;
		prp = true;
	}
	bool prepared() const
	{
		return prp;
	}
	errset check();
};

class etree
{
	
	signed_t opsfrom;
	signed_t numsfrom;

	std::map<std::wstring, ptr::shared_ptr<calculating_value>, std::less<> > vars;

    ptr::shared_ptr<node> root;
	ptr::shared_ptr<node> parse(const std::wstring_view &expression);

public:

	etree();

	void set_var(const std::wstring& vn, ptr::shared_ptr<calculating_value> vv)
	{
		vars[vn] = vv;
	}
	ptr::shared_ptr<calculating_value> evaluate(const std::wstring_view &expression, signed_t precision, bool newcalc);
};


class calc_machine : public etree
{
	ptr::shared_ptr<calculating_value> curval;
public:

	~calc_machine()
	{
		curval = nullptr;
	}

	ptr::shared_ptr<calculating_value> start_eval(const std::wstring_view& expression, signed_t precision)
	{
		stop_eval();
		curval = evaluate(expression, precision, false /* due this is addition calculation for view */);
		return curval;
	}
	void stop_eval()
	{
		curval = nullptr;
	}
};