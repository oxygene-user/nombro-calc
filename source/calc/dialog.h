#pragma once


class DialogView : public SystemWindowView
{
	typedef SystemWindowView super;

public:
	DialogView();
	virtual ~DialogView();

	
	/*virtual*/ void created() override;
	/*virtual*/ void on_destroy() override;
	/*virtual*/ bool on_key(int vk, bool down) override;
	/*virtual*/ void on_char(wchar_t c, bool batch) override;
	/*virtual*/ void on_poschange(const int4& newposnsize);

	/*virtual*/ void activate() override;


};

#include "about.h"
