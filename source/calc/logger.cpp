#include "pch.h"

#ifdef LOGGER
#include <codecvt>

logger lg;


logger::item& logger::item::operator<<(const char* s)
{
	char pc = length() > 0 ? (*this)[length() - 1] : ' ';
	if (pc != ' ' && pc != '[' && s[0] != ',' && s[0] != ' ' && s[0] != ']')
		this->push_back(' ');
	this->append(s);
	return *this;
}
logger::item& logger::item::operator<<(signed_t x)
{
	*this << std::to_string(x).c_str();
	return *this;
}
logger::item& logger::item::operator<<(const std::vector<value>& args)
{
	*this << "args( ";
	for (const value& v : args)
	{
		*this << v << ",";
	}
	if ((*this)[length() - 1] == ',')
		(*this)[length() - 1] = ')';
	else this->push_back(')');

	return *this;
}
logger::item& logger::item::operator<<(const value& val)
{
	std::wstring vv = val.to_string(10, 111);

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::string narrow = converter.to_bytes(vv);

	std::string s = std::string(vv.begin(), vv.end());
	*this << s.c_str() << "[" << val.get_precision() << "]";

	return *this;
}
#endif