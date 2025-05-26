#include "pch.h"


AboutView::AboutView()
{
	create_desktop_window(L"About oxid-calc", int2(400,300));
}

/*virtual*/ void AboutView::created()
{
	Layout l;
	add_view(new LabelView(TRS("oxid-calc v.0.1")), l.vnext(20));
	add_view(new LinkLabelView(WSTR("https://github.com/oxygene-user/oxid-calc"), TRS("source code")), l.vnext(20));
	ok = add_view(new ButtonView(TRS("Ok")), l.vnext(30));
	
	ajust_size(l);

	super::created();
}


/*virtual*/ void AboutView::on_event(EventType et, signed_t eid)
{
	if (EventType::CONTROL_CLICK == et && eid == ok)
	{
		on_close();
	}
}