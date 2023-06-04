#include "pch.h"


std::wstring get_start_path()
{
	wchar_t buf[4096];
	signed_t len = GetModuleFileNameW(nullptr, buf, sizeof(buf)-1);

	std::wstring p(buf, len);
	signed_t z = p.find_last_of('\\');
	p.resize(z);
	return p;
}

std::wstring path_fix(const std::wstring& path)
{
	if (path.size() >= 2 && path[0] == '.' && path[1] == '\\')
	{
		// replace '.' with path of exe file
		return path_concat(get_start_path(), path.substr(2));
	}
	return path;
}

std::wstring path_concat(const std::wstring_view &path, const std::wstring_view &fn)
{
	std::wstring c(path);
	if (c[c.length() - 1] != '\\')
		c.push_back('\\');
	c.append(fn);
	return c;
}

bool load_buf(const std::wstring& fn, std::vector<u8>& b)
{
	HANDLE h = CreateFileW(fn.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (INVALID_HANDLE_VALUE == h)
	{
		b.clear();
		return false;
	}
	signed_t fnl = GetFileSize(h, nullptr);
	b.resize(fnl);
	DWORD r;
	ReadFile(h, b.data(), (DWORD)fnl, &r, nullptr);
	CloseHandle(h);
	if (r != fnl)
	{
		b.clear();
		return false;
	}
	return true;
}

void save_buf(const std::wstring& fn, const std::string& b)
{
	HANDLE h = CreateFileW(fn.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (INVALID_HANDLE_VALUE == h)
		return;
	DWORD w;
	WriteFile(h, b.data(), (int)b.length(), &w, nullptr);
	CloseHandle(h);
}

std::wstring from_utf8(const std::string_view& src)
{
	std::wstring rv;
	rv.resize(src.length(), 0);

	signed_t res = MultiByteToWideChar(CP_UTF8, 0, src.data(), (int)src.length(), rv.data(), (int)rv.capacity());
	rv.resize(res);
	return rv;
}

std::string to_utf8(const std::wstring_view& src)
{
	std::string rv;
	rv.resize(src.length() * 2, 0);
	signed_t l = WideCharToMultiByte(CP_UTF8, 0, src.data(), (int)src.length(), rv.data(), (int)rv.capacity(), nullptr, nullptr);
	rv.resize(l);
	return rv;
}