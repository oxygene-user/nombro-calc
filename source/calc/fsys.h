#pragma once


std::wstring get_start_path();
std::wstring path_fix(const std::wstring& path);
std::wstring path_concat(const std::wstring_view &path, const std::wstring_view &fn);
bool load_buf(const std::wstring& fn, std::vector<u8>& b);
void save_buf(const std::wstring& fn, const std::string& b);

std::wstring from_utf8(const std::string_view& src);
std::string to_utf8(const std::wstring_view& src);