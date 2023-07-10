#include "pch.h"

/*virtual*/ ptr::shared_ptr<calculating_value> operator_node::evaluate(signed_t precision)
{
	struct ntu
	{
		ntu() { ++numthreads; }
		~ntu() { --numthreads; }
	};

	class op_calculating_value : public calculating_value
	{
		std::vector<ptr::shared_ptr<calculating_value>> pcparams;
		const ::op *op;
		signed_t precision;
		u8 calctag;
		bool unmark_bypass;

		static void calc_thread(op_calculating_value * z)
		{
			ntu ntu_;

			std::vector<value> cparams;
			std::vector<bool> copied;
			std::unique_ptr<::op::context> ctx(z->op->create_context(z->calctag));
			cparams.insert(cparams.begin(), z->pcparams.size(), value(errset::CALCULATING));
			copied.insert(copied.begin(), z->pcparams.size(), false);
			bool all_final = false;
			for (;!all_final;)
			{
				all_final = true;
				signed_t i = 0;
				for (auto &v : z->pcparams)
				{
					if (copied[i])
					{
						++i;
						continue;
					}

				get_final_bypass_loop:
					value vv = v->wait_for_any_result(cparams[i]);

					if (vv.error() == errset::BYPASS) // don't calc, just pass value as result
					{
						if (z->unmark_bypass)
							vv.unbypass();

						if (v->is_final_result_ready())
						{
							z->set_final_result(vv);
							return;
						}

						z->set_intermediate_result(vv);
						cparams[i] = value(errset::CALCULATING); // reset this param to wait new one

						if (z->is_released())
						{
							z->set_final_result(value(errset::STOP)); // actually delete
							return;
						}

						if (z->calctag != op::calctag)
							return;

						goto get_final_bypass_loop;
					}

					if (vv.is_error())
					{
						z->set_final_result(vv);
						return;
					}


					ASSERT(vv.get_precision() != value::P_UNDEFINED);

					if (z->is_released() || globalstop || z->calctag != op::calctag)
					{
						z->set_final_result(value(errset::STOP)); // actually delete
						return;
					}

					if (v->is_final_result_ready())
					{
						copied[i] = true;
					}
					else
					{
						all_final = false;
					}
					cparams[i++] = vv;
				}

				::op::context* cx = ctx.get();
#ifdef LOGGER
				if (cx == nullptr || !cx->used)
				{
					lg.rec() << z->op->d_name() << cparams;
				}
#endif
				calc_result_t r = z->op->calc(cparams, z->precision, cx);
				if (cx != nullptr)
					cx->used = true;

#ifdef LOGGER
				if (std::get<1>(r))
				{
					lg.rec() << z->op->d_name() << "result" << std::get<0>(r); // << (std::get<1>(r) ? "final" : "temp");
				}
#endif

				if (z->unmark_bypass && std::get<0>(r).error() == errset::BYPASS)
					std::get<0>(r).unbypass();

				if (z->is_released() || z->calctag != op::calctag || globalstop)
				{
					z->set_final_result(value(errset::STOP)); // actually delete
					return;
				}


				if (all_final && std::get<0>(r).precision_better_or_equal_then(z->precision))
				{
					z->set_final_result(std::get<0>(r));
					break;
				}
				else
				{
					if (std::get<1>(r))
					{
						// maximum precision has been reached for current set of params
						signed_t mp = z->precision;
						signed_t rei = -1;
						for (signed_t ii = 0, c = cparams.size(); ii < c; ++ii)
						{
							signed_t p = cparams[ii].get_precision();
							
							if (value::P_ABSOLUTE == p)
								continue;

							if (mp > p)
							{
								rei = ii;
								mp = p;
							}
						}
						if (rei >= 0)
						{
							// found param with smaller precision then current result of op
							// we have to recalculate it

							all_final = false;
							z->set_intermediate_result(std::get<0>(r));
							cparams[rei] = value(errset::CALCULATING); // reset this param to wait new one
						}
						else
						{
							// all params have greater precision then current result op
							// so, assume result is final and finish calculations
							z->set_final_result(std::get<0>(r));
							break;
						}
					}
					else
					{
						all_final = false;
						z->set_intermediate_result(std::get<0>(r));
					}
				}
			}
		}

	public:
		op_calculating_value(u8 ct, const ::op *op, bool unmark_bypass, std::vector<ptr::shared_ptr<node>> &params, signed_t precision): calctag(ct), op(op), precision(precision), unmark_bypass(unmark_bypass)
		{
			for (auto& n : params)
			{
				pcparams.emplace_back(n.get()->evaluate(precision));
			}
			std::thread th( calc_thread, this );
			th.detach();
		}
	};

	ASSERT(calctag == op::calctag);
	if (cv)
		return cv;
	cv = new op_calculating_value(this->calctag, this->op, this->result_unmark_bypass, params, precision);
	return cv;
}

errset operator_node::check()
{

	uhalf pre = 0, post = 0;
	for (const auto &n : params)
	{
		if (n->is_error())
			return n->error();
		if (n->prefix)
			++pre;
		else
			++post;
	}

	if (!op->reqpars(pre, post))
		return errset::BAD_ARGS_NUM;

	return errset::OK;
}

/*virtual*/ bool string_node::absorb(std::vector<ptr::shared_ptr<node>>& heap, signed_t index)
{
	tools::trim(str);
	if (str.empty())
	{
		heap.erase(heap.begin() + index);
		return true;
	}

	// preparse exponential form
	for (size_t offs = 0;;)
	{
		size_t epos = str.find('e', offs);
		if (epos == str.npos)
			break;
		offs = epos + 1;
		if (epos > 0 && epos + 1 < str.length())
		{
			wchar_t prec = str[epos - 1];
			wchar_t postc = str[epos + 1];
			if (is_digit(prec) || prec == '.')
			{
				if (is_digit(postc) || postc == '+' || postc == L'\u2212')
				{
					str[epos] = 'E';
					if (postc == '+')
						str[epos+1] = ' '; // not to confuse the parser with this plus
				}
			}
		}
	}

	const op::allops& all = op::all();
	for (const op &o : all)
	{
		size_t from = 0;
		sag:
		size_t i = str.find(o.name, from);
		if (i == str.npos)
			continue;
		const op* oo = &o;
		while (oo->bigger)
		{
			if (str.substr(i)._Starts_with(oo->bigger->name))
			{
				oo = oo->bigger;
				continue;
			}
			break;
		}

		
		if (is_letter(oo->name[0]))
		{
			if (i > 0 && is_letter(str[i - 1]))
			{
				from = i + 1;
				goto sag;
			}
			if (i + oo->name.length() < str.length() && is_letter(str[i + oo->name.length()]))
			{
				from = i + 1;
				goto sag;
			}
		}

		ptr::shared_ptr<node> n = new operator_node(oo);
		heap.insert(heap.begin() + index, n);
		signed_t myindex = index + 1;
		if (i > 0)
		{
			std::wstring lns = str.substr(0, i);
			tools::trim(lns);
			if (!lns.empty())
			{
				// insert left part of string (before operator) as string node to heap
				ptr::shared_ptr<node> sn = new string_node(lns);
				heap.insert(heap.begin() + index, sn);
				++myindex;
			}
		}
		str.erase(0, i+oo->name.length()); // and fix current string node
		tools::trim(str);
		if (str.empty())
		{
			heap.erase(heap.begin() + myindex);
		}
		return true;
	}

	size_t z = str.find(L',');
	if (z != str.npos)
	{
		std::wstring lns = str.substr(0, z);
		str.erase(0, z+1);
		tools::trim(lns);
		tools::trim(str);

		if (str.empty() || lns.empty())
		{
			heap.clear();
			heap.emplace_back(new value_node(value(errset::EMPTY_SUBEXPRESSION)));
			return false;
		}

		ptr::shared_ptr<node> sn = new string_node(lns);
		heap.insert(heap.begin() + index, sn);
		return true;
	}

	std::wstring int_part, non10radix;
	std::wstring frac_part;
	bool frac_in_progress = false;
	bool var_in_progress = false;
	bool bin_disallowed = false;
	signed_t radix = 10;

	for (size_t i = 0, cnt = str.length(); i < cnt; ++i)
	{
		wchar_t c = str[i];
		if (c >= '0' && c <= '9')
		{
			if (var_in_progress)
				continue;

			if (radix != 10)
			{
				non10radix.push_back(c);
			} else if (frac_in_progress)
				frac_part.push_back(c);
			else
			{
				int_part.push_back(c);
				if (c >= '2')
					bin_disallowed = false;
			}
			continue;
		}
		if (c == '.')
		{
			if (frac_in_progress || radix != 10)
			{
				heap.clear();
				heap.emplace_back(new value_node(value(errset::NUMBER_NOT_RECOGNIZED)));
				return false;
			}
			frac_in_progress = true;
			continue;
		}
		if (c == 'x' && !var_in_progress)
		{
			if (frac_in_progress || radix == 16)
			{
				heap.clear();
				heap.emplace_back(new value_node(value(errset::NUMBER_NOT_RECOGNIZED)));
				return false;
			}
			if (int_part.length() == 1 && int_part[0] == '0')
			{
				radix = 16;
				non10radix = std::move(int_part);
				continue;
			}
			if (int_part.size() == 0)
				var_in_progress = true;
			else
			{
				heap.clear();
				heap.emplace_back(new value_node(value(errset::NUMBER_NOT_RECOGNIZED)));
				return false;
			}
			continue;
		}
		if (c == '$' || c == '#')
		{
			bool numip = frac_in_progress || radix == 16 || !non10radix.empty() || !int_part.empty();
			if (var_in_progress || numip)
			{
				heap.clear();
				heap.emplace_back(new value_node(value(numip ? errset::NUMBER_NOT_RECOGNIZED : errset::BAD_SYMBOLS_IN_EXPRESION)));
				return false;
			}
			radix = 16;
			continue;
		}
		if (c >= 'a' && c <= 'f')
		{
			if (var_in_progress)
				continue;

			if (frac_in_progress)
			{
				heap.clear();
				heap.emplace_back(new value_node(value(errset::NUMBER_NOT_RECOGNIZED)));
				return false;
			}

			if (!int_part.empty() && c == 'b')
			{
				if (!bin_disallowed && i + 1 == str.length())
				{
					radix = 2;
					non10radix = std::move(int_part);
					break;
				}

				radix = 16;
				non10radix = std::move(int_part);
			}

			bin_disallowed = false;

			if (radix == 16)
				non10radix.push_back(c);
			else if (int_part.empty())
				var_in_progress = true;

			continue;
		}

		if (c >= 'g' && c <= 'z')
		{
			if (var_in_progress)
				continue;

			if (radix == 10 && c == 'h')
			{
				if (i + 1 != str.length())
				{
					heap.clear();
					heap.emplace_back(new value_node(value(errset::NUMBER_NOT_RECOGNIZED)));
					return false;
				}

				non10radix = std::move(int_part);
				radix = 16;
				bin_disallowed = false;
				break;
			}

			if (radix == 16 || frac_in_progress || !int_part.empty())
			{
				heap.clear();
				heap.emplace_back(new value_node(value(errset::NUMBER_NOT_RECOGNIZED)));
				return false;
			}

			var_in_progress = true;
			continue;
		}

		heap.clear();
		heap.emplace_back(new value_node(value(errset::BAD_SYMBOLS_IN_EXPRESION)));
		return false;
	}

	if (var_in_progress)
	{
		variable = true;
		return true;
	}

	if (radix == 10)
	{
		ptr::shared_ptr<node> rv(new value_node(value(int_part, frac_part, 10)));
		heap[index] = rv;
		return true;
	}

	ptr::shared_ptr<node> rv(new value_node(value(non10radix, radix)));
	heap[index] = rv;
	return true;
}

bool operator_node::addparnext(std::vector<ptr::shared_ptr<node>>& heap, signed_t index1)
{
	node* an = heap[index1 + 1];
	if (operator_node* on = dynamic_cast<operator_node*>(an))
	{
		if (!on->prepared())
		{
			if (on->op->reqpars(0, 1) && size_t(index1 + 2) < heap.size())
			{
				if (!on->addparnext(heap, index1 + 1))
				{
					heap.clear();
					heap.emplace_back(new value_node(value(errset::BAD_ARGS_NUM)));
					return false;
				}
			}
			else {
				heap.clear();
				heap.emplace_back(new value_node(value(errset::BAD_ARGS_NUM)));
				return false;
			}
		}
	}

	add_par(heap[index1 + 1], false);
	heap.erase(heap.begin() + index1 + 1);
	return true;

};



/*virtual*/ bool operator_node::absorb(std::vector<ptr::shared_ptr<node>>& heap, signed_t index)
{
	if (heap.size() == 1)
	{
		if (!op->reqpars(0, 0))
		{
			heap.clear();
			heap.emplace_back(new value_node(value(errset::BAD_ARGS_NUM)));
		}

		// nothing to absorb
		return false;
	}

	if (index == 0)
	{
		if (op->reqpars(0, 1))
			return addparnext(heap, index) && heap.size() > 1;

		if (op->reqpars(0, 0))
		{
			prp = true;
			return true;
		}

		heap.clear();
		heap.emplace_back(new value_node(value(errset::BAD_ARGS_NUM)));
		return false;
	}

	if (size_t(index + 1) == heap.size())
	{
		if (op->reqpars(1, 0))
		{
			add_par(heap[index-1], true);
			heap.erase(heap.begin()+index-1);
			return heap.size() > 1;
		}

		if (op->reqpars(0, 0))
		{
			prp = true;
			return true;
		}

		heap.clear();
		heap.emplace_back(new value_node(value(errset::BAD_ARGS_NUM)));
		return false;
	}
	
	// so operator is not 1st and not last in heap
	// middle

	if (op->reqpars(1, 1))
	{
		// can absorb left and right
		add_par(heap[index - 1], true);
		if (addparnext(heap, index))
			heap.erase(heap.begin() + index - 1);
		return heap.size() > 1;
	}
	else {
		node* pn = heap[index - 1].get();
		if (dynamic_cast<value_node*>(pn) != nullptr)
		{
			if (op->homonym != nullptr && op->homonym->reqpars(1, 1))
			{
				// so, synonym can accept value at left
				op = op->homonym;
				return true;
			}
		} else if (dynamic_cast<operator_node*>(pn) != nullptr)
		{
			operator_node* on = (operator_node*)pn;
			if (on->prp && op->homonym != nullptr && op->homonym->reqpars(1, 1))
			{
				op = op->homonym;
				return true;
			}
		}
	}

	if (op->reqpars(0, 1))
	{
		// can absorb only right
		return addparnext(heap, index);
	}

	if (op->reqpars(1, 0))
	{
		// can absorb only left
		add_par(heap[index - 1], false);
		heap.erase(heap.begin() + index - 1);
		return true;
	}

	if (op->reqpars(0, 0))
	{
		// can absorb nothing
		prp = true;
		return true;
	}

	heap.clear();
	heap.emplace_back(new value_node(value(errset::BAD_ARGS_NUM)));

	return false;
}

ptr::shared_ptr<node> etree::parse(const std::wstring_view &expression)
{
    if (expression.size() > 2048)
    {
		ptr::shared_ptr<node> rv(new value_node(value(errset::EXPRESSION_TOO_BIG)));
        return rv;
    }

	std::vector<ptr::shared_ptr<node>> heap;

	size_t prev = 0, j = expression.length();
	for (size_t i = 0; i < j; ++i)
	{
		wchar_t c = expression[i];
		if (c == '(')
		{
			if (prev < i)
				heap.emplace_back(new string_node(std::wstring(expression.substr(prev,i-prev))));

			int cnt = 1;
			size_t bre = expression.npos;
			for (size_t x = i + 1; x < j; ++x)
			{
				wchar_t cc = expression[x];
				if (cc == '(')
					++cnt;
				else if (cc == ')')
				{
					--cnt;
					if (cnt == 0)
					{
						bre = x;
						break;
					}
				}
			}
			if (bre == expression.npos || cnt != 0)
			{
				ptr::shared_ptr<node> rv(new value_node(value(errset::PARENTHESIS_FAIL)));
				return rv;
			}

			ptr::shared_ptr<node> brn = parse(expression.substr(i + 1, bre - i - 1));
			if (brn->is_error())
				return brn;

			prev = bre + 1;
			i = bre;

			heap.push_back(brn);
		}
	}

	if (prev < j)
		heap.emplace_back(new string_node(std::wstring(expression.substr(prev, j - prev))));

	for (bool absorbing = true; absorbing;)
	{
		absorbing = false;
		bool somesn = false;

		for (signed_t i = 0, c = heap.size(); i < c; ++i)
		{
			node* n = heap[i].get();
			if (string_node* sn = dynamic_cast<string_node*>(n))
			{
				if (sn->variable)
				{
					auto it = vars.find(sn->str);
					if (it != vars.end())
					{
						if (it->second->is_final_result_ready() && it->second->get_value().error() == errset::EMPTY)
						{
							ptr::shared_ptr<node> rv(new value_node(it->second->get_value()));
							return rv;
						}

						ptr::shared_ptr<node> rv(new expression_node(it->second));
						heap[i] = rv;
						absorbing = true;
						break;
					}

					ptr::shared_ptr<node> rv(new value_node(value(errset::VARIABLE_NOT_FOUND)));
					return rv;
				}

				if (sn->absorb(heap, i))
				{
					absorbing = true;
					break;
				}
				if (heap.size() == 1)
				{
					if (value_node* vn = dynamic_cast<value_node*>(heap[0].get()))
						if (vn->is_error())
							return heap[0];
				}
				somesn = true;
			}
		}

		if (somesn)
		{
			ptr::shared_ptr<node> rv(new value_node(value(errset::BAD_SYMBOLS_IN_EXPRESION)));
			return rv;
		}

		if (absorbing)
			continue;

		// 1st of all lets prepare consts
		for (signed_t i = 0, c = heap.size(); i < c; ++i)
		{
			node* n = heap[i].get();
			if (operator_node* on = dynamic_cast<operator_node*>(n))
			{
				if (on->prepared())
					continue;

				if (on->op->precedence == op::PRECEDENCE_CONST && on->op->reqpars == npars(0,0))
				{
					if (heap[i]->absorb(heap, i))
					{
						absorbing = true;
						break;
					}

				}
			}
		}

		if (absorbing)
			continue;

		signed_t index = -1;
		signed_t preced = op::PRECEDENCE_LOWEST;
		for (signed_t i = 0, c = heap.size(); i < c; ++i)
		{
			node* n = heap[i].get();
			if (operator_node* on = dynamic_cast<operator_node*>(n))
			{
				if (on->prepared())
					continue;

				if (on->op->precedence < preced)
				{
					index = i;
					preced = on->op->precedence;
				}
			}
		}

		if (index >= 0 && heap[index]->absorb(heap, index))
		{
			absorbing = true;
			continue;
		}


		absorbing = false;
	}

	if (heap.size() > 1)
	{
		ptr::shared_ptr<node> rv(new value_node(value(errset::OPERATOR_NOT_FOUND)));
		return rv;
	}

	if (heap.size() == 1)
	{
		heap[0]->mutate();
		return heap[0];
	}

	{
		ptr::shared_ptr<node> rv = new value_node(errset::EMPTY_SUBEXPRESSION);
		return rv;
	}
}


ptr::shared_ptr<calculating_value> etree::evaluate(const std::wstring_view &expression, signed_t precision, bool newcalc)
{
	if (newcalc)
		++op::calctag; // this action will break all current calculations

    root = nullptr;

	if (expression.empty())
		return new calculating_value(errset::OK);

    root = parse(expression);
    return root->evaluate(precision);
}

etree::etree()
{
	ptr::shared_ptr<calculating_value> v = new calculating_value(value(errset::INF));
	set_var(std::wstring(WSTR("inf")), v);
}
