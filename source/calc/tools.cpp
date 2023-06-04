#include "pch.h"

namespace tools
{
    u8 ALIGN(256) multbl[256][256];
}


class tools_init
{
public:
    tools_init()
    {
        for (int a = 0; a < 256; ++a)
            for (int c = 0; c < 256; ++c)
            {
                int k = a * c / 255;
                tools::multbl[a][c] = (u8)k;
            }
    }
};

static tools_init ti;

bool messagebox(const char* s1, const char* s2, int options)
{
    // TODO : show message box for main thread

    return true;
}