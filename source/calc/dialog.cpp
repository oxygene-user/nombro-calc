#include "pch.h"

DialogView::DialogView():SystemWindowView()
{

}

DialogView::~DialogView()
{

}

/*virtual*/ void DialogView::created()
{
	super::created();
}
/*virtual*/ void DialogView::on_destroy()
{

}
/*virtual*/ bool DialogView::on_key(int vk, bool down)
{
	if (vk == VK_ESCAPE && down)
	{
		on_close();
		return true;
	}

	return false;
}
/*virtual*/ void DialogView::on_char(wchar_t /*c*/, bool /*batch*/)
{

}
/*virtual*/ void DialogView::on_poschange(const int4& /*newposnsize*/)
{

}

/*virtual*/ void DialogView::activate()
{

}
