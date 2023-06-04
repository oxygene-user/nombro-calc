#pragma once

template <typename TCH> TCH get_last_char(const std::basic_string<TCH>& s)
{
	size_t l = s.length();
	if (l > 0)
		return s[l - 1];
	return 0;
}

template <typename TCH> void trunc_len(std::basic_string<TCH>& s, size_t numchars = 1)
{
	size_t l = s.length();
	if (l >= numchars)
		s.resize(l - numchars);
}

template <typename TCH> std::basic_string<TCH>& replace_all(std::basic_string<TCH>& s, const std::basic_string_view<TCH> &s1, const std::basic_string_view<TCH>& s2)
{
	for (size_t f = 0;;)
	{
		f = s.find(s1, f);
		if (f == s.npos)
			break;

		s.replace(f, s.size(), s2);
		f += s2.size();
	}
	return s;
}

template <typename TCH> std::basic_string<TCH>& replace_all(std::basic_string<TCH>& s, TCH c1, TCH c2)
{
	for (size_t f = 0;;)
	{
		f = s.find(c1, f);
		if (f == s.npos)
			break;

		s[f] = c2;
		++f;
	}
	return s;
}


inline std::string build_string(const char* s, ...)
{
	char str[1024];

	va_list args;
	va_start(args, s);
	vsnprintf(str, sizeof(str), s, args);
	va_end(args);
	str[sizeof(str) - 1] = 0;
	return std::string(str);
}

inline std::string build_string(const char* fn, int ln, const char* s, ...)
{
	char str[1024];

	int t = sprintf_s(str, sizeof(str), "%s(%i): ", fn, ln);

	va_list args;
	va_start(args, s);
	vsnprintf(str+t, sizeof(str)-t, s, args);
	va_end(args);
	
	str[sizeof(str) - 1] = 0;

	return std::string(str);
}










#define BUILD_ASTRVIEW(x,y) std::string_view(x,y)

#if defined _MSC_VER
#define BUILD_WSTRVIEW(x,y) std::wstring_view(L##x,y)
#define CONST_STR_BUILD( tc, s ) _const_str_build<tc>::get( s, L##s, sizeof(s)-1 )
#define WIDE2(s) L##s
#elif defined __GNUC__
#define WSPTR_MACRO(x,y) std::wstring_view(u##x,y)
#define CONST_STR_BUILD( tc, s ) _const_str_build<tc>::get( s, u##s, sizeof(s)-1 )
#define WIDE2(s) u##s
#endif


template<typename T> struct _const_str_build
{
};
template<> struct _const_str_build<char>
{
	static std::string_view get(const char* sa, const wchar_t*, signed_t len) { return std::string_view(sa, len); }
};
template<> struct _const_str_build<wchar_t>
{
	static std::wstring_view get(const char*, const wchar_t* sw, signed_t len) { return std::wstring_view(sw, len); }
};


#define ASTR( s ) BUILD_ASTRVIEW( s, sizeof(s)-1 )
#define WSTR( s ) BUILD_WSTRVIEW( s, sizeof(s)-1 )
