#include "pch.h"

Config cfg;


Config::Config()
{

}
Config::~Config()
{

}

void Config::fix()
{
	if (wwidth < 600)
		wwidth = 600;

	if (wheight < 400)
		wheight = 400;

	RECT r; r.left = wposx; r.top = wposy; r.right = r.left + wwidth; r.bottom = r.top + wheight;
	HMONITOR m = MonitorFromRect(&r, MONITOR_DEFAULTTONEAREST);
	MONITORINFOEXW minf;
	minf.cbSize = sizeof(MONITORINFOEXW);
	GetMonitorInfo(m, &minf);
	if (wposx + wwidth < (minf.rcWork.left + 150))
	{
		// totally out-left of monitor
		wposx = minf.rcWork.left - wwidth / 2;
	}
	if (wposx > (minf.rcWork.right - 150))
	{
		// totally out-right of monitor
		wposx = minf.rcWork.right - wwidth / 2;
	}
	if (wposy + wheight < (minf.rcWork.top + 100))
	{
		// totally out-top of monitor
		wposy = minf.rcWork.top - wheight / 2;
	}
	if (wposy > (minf.rcWork.bottom - 100))
	{
		// totally out-bottom of monitor
		wposy = minf.rcWork.bottom - wheight / 2;
	}

}

void Config::load()
{
	std::wstring cfgp(path_concat(get_start_path(), WSTR("config.cfg")));
	load(cfgp, o_save_if_not_exist);
	fix();
}

void Config::load(const std::wstring& fn, usingle options)
{
	bool primary = 0 == (options & o_secondary);

    wsts conf;
    std::vector<u8> cb;
    if (load_buf(fn, cb))
    {
        std::wstring ct = from_utf8(std::string_view((const char *)cb.data(), cb.size()));
		conf.load(ct);
	}
	else
	{
		if (!primary)
			return; // do nothing if savepath file not exist

		if (0 != (options & o_save_if_not_exist))
		{
			conf.clear();

#define CP( t, field, dv, c ) conf.set(__STR1W__(field)).set_value(t(dv)).add_comment(L ## c);
			CFGPARS
#undef CP
			wsts& r = conf.set(c_result);
			wsts &rb1 = r.add_block();
			rb1.set(c_id).set_value(c_default);
			rb1.set(WSTR("expression")).set_value(c_empty);
			rb1.set(WSTR("radix")).set_value(10);
			rb1.set(WSTR("precision")).set_value(100);
			rb1.set(WSTR("placeholder")).set_value(WSTR("Result"));

			wsts& rb2 = r.add_block();
			rb2.set(c_id).set_value(WSTR("hex"));
			rb2.set(WSTR("expression")).set_value(c_empty);
			rb2.set(WSTR("radix")).set_value(16);
			rb2.set(WSTR("precision")).set_value(0);
			rb2.set(WSTR("placeholder")).set_value(WSTR("Result as Hex"));

			wsts& rb3 = r.add_block();
			rb3.set(WSTR("active")).set_value(0);
			rb3.add_comment(WSTR("// deactivated - just example"));
			rb3.set(c_id).set_value(WSTR("sqrt"));
			rb3.set(WSTR("expression")).set_value(WSTR("sqrt result"));
			rb3.set(WSTR("radix")).set_value(10);
			rb3.set(WSTR("precision")).set_value(10);

			std::string cfgs = to_utf8(conf.store());
			save_buf(fn, cfgs);
		}
	}



#define CP( t, field, dv, c ) field = conf.get_##t(__STR1W__(field), dv);
	CFGPARS
#undef CP

	if (primary)
		results.clear();

	if (const wsts* r = conf.get(c_result))
	{
		for (auto it = r->begin(); it; ++it)
		{
			if (primary)
				results.emplace_back( *it );
			else
			{
				std::wstring id = it->get_string(c_id);
				if (id.empty())
					results.emplace_back(*it);
				else {
					for (ResultFormat& rf : results)
					{
						if (rf.id == id)
						{
							rf.load(*it);
						}
					}

				}
			}
		}
	}

	if (results.size() == 0)
	{
		// setup default result
		results.emplace_back();
	}

	if (primary && savepath != WSTR("."))
		load(path_fix(savepath), o_secondary);

}

void Config::save()
{
	beforesave = 0;
	wsts conf;
	std::wstring cfgp;
	std::vector<u8> cb;

	if (savepath == WSTR("."))
		cfgp = path_concat(get_start_path(), WSTR("config.cfg"));
	else
	{
		cfgp = path_fix(savepath);
	}

	if (load_buf(cfgp, cb))
		conf.load(from_utf8(std::string_view((const char*)cb.data(), cb.size())));

#define CP( t, field, dv, c ) conf.set(__STR1W__(field)).set_value(field);
	CFGPARS
#undef CP

	wsts& r = conf.add_block(c_result);
	r.clear();
	for (ResultFormat& rf : results)
	{
		rf.save(r.add_block());
	}

	save_buf(cfgp, to_utf8(conf.store()));

}

void Config::tick()
{
	if (beforesave > 0)
	{
		--beforesave;
		if (beforesave == 0)
			save();
	}

}

void Config::ondie()
{
	if (beforesave > 0)
		save();
}

