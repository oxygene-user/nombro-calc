#pragma once

class AboutView : public DialogView
{
	typedef DialogView super;
	signed_t ok = 0;
public:
	AboutView();

	/*virtual*/ void created() override;
	/*virtual*/ void on_event(EventType et, signed_t id);
};
