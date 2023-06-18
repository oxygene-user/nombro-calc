#pragma once

#ifdef LOGGER
class value;
class logger
{

public:
	class item : public std::string
	{
		
	public:
		item& operator <<(const char* s);
		item& operator <<(signed_t x);
		item& operator <<(const std::vector<value> &args);
		item& operator <<(const value &val);
	};
private:
	std::vector<item> items;
	std::mutex mutex;

public:

	void reset()
	{
		std::unique_lock<std::mutex> m(mutex);
		items.clear();
		items.reserve(100);
	}


	item& rec()
	{
		std::unique_lock<std::mutex> m(mutex);
		items.emplace_back();
		return items[items.size()-1];
	}

};

extern logger lg;

#endif