#pragma once


class ResultView;
class value;
class calc_machine;
struct ResultFormat
{
	std::wstring id, expression, placeholder, epreparsed;
	signed_t radix = 10;
	signed_t precision = 100;

	std::map<std::wstring, std::wstring, std::less<> > comments;
	std::unique_ptr<calc_machine> calcmachine; // for expression


	signed_t fmt_id;
	bool active = true;

	ResultFormat();
	ResultFormat(const wsts& s);
	ResultFormat(ResultFormat&& of) noexcept
	{
		id = std::move(of.id);
		expression = std::move(of.expression);
		placeholder = std::move(of.placeholder);
		radix = of.radix;
		precision = of.precision;
		comments = std::move(of.comments);
		calcmachine = std::move(of.calcmachine);
		active = of.active;
		fmt_id = of.fmt_id;
	}
	~ResultFormat();

	void load(const wsts& s);
	void save(wsts& b);

	//std::wstring build_result_text(signed_t &pass, const value& v);

	static signed_t next_id();

private:

	const std::wstring* getcmnt(const std::wstring_view& n) const
	{
		auto x = comments.find(n);
		if (x != comments.end())
			return &x->second;
		return nullptr;
	}
};
