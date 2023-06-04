#include "pch.h"

void Surface::reset()
{
    if (mem_dc)
    {
        DeleteDC(mem_dc);
        mem_dc = nullptr;
    }
    if (mem_bitmap)
    {
        DeleteObject(mem_bitmap);
        mem_bitmap = nullptr;
    }
    body = nullptr;
    memset(this, 0, sizeof(ImageInfo));
}


void Surface::create(const int2 &isz, int monitor)
{
    reset();

    sz = isz;
    bitpp = 32;

    DEVMODEW devmode;
    devmode.dmSize = sizeof(DEVMODE);

    if (monitor < 0)
        EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &devmode);
    else
    {
        struct mdata
        {
            DEVMODEW *devm;
            int mi;
            static BOOL CALLBACK calcmc(HMONITOR hMonitor, HDC /*hdcMonitor*/, LPRECT /*lprcMonitor*/, LPARAM dwData)
            {
                mdata *m = (mdata *)dwData;
                if (m->mi == 0)
                {
                    MONITORINFOEXW minf;
                    minf.cbSize = sizeof(MONITORINFOEXW);
                    GetMonitorInfo(hMonitor, &minf);
                    EnumDisplaySettingsW(minf.szDevice, ENUM_CURRENT_SETTINGS, m->devm);
                    return FALSE;
                }
                --m->mi;
                return TRUE;
            }
        } mm; mm.mi = monitor; mm.devm = &devmode;

        EnumDisplayMonitors(nullptr, nullptr, mdata::calcmc, (LPARAM)&mm);

    }

    devmode.dmBitsPerPel = 32;
    devmode.dmPelsWidth = sz.x;
    devmode.dmPelsHeight = sz.y;

    HDC tdc = CreateDCW(L"DISPLAY", nullptr, nullptr, &devmode);

    mem_dc = CreateCompatibleDC(tdc);
    if (mem_dc == nullptr)
        __debugbreak();

    BITMAPV4HEADER bmi;

    int ll = sz.x * 4;
    pitch = (ll + 3) & (~3);

    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bV4Size = sizeof(bmi);
    bmi.bV4Width = sz.x;
    bmi.bV4Height = -sz.y;
    bmi.bV4Planes = 1;
    bmi.bV4BitCount = 32;
    bmi.bV4V4Compression = BI_RGB;
    bmi.bV4SizeImage = 0;

    mem_bitmap = CreateDIBSection(tdc, (BITMAPINFO *)&bmi, DIB_RGB_COLORS, (void **)&body, 0, 0);
    ASSERT(mem_bitmap);
    SelectObject(mem_dc, mem_bitmap);
    DeleteDC(tdc);
}

void Surface::flush(HDC dc)
{
    int xx = 0;
    int yy = 0;
    BitBlt(dc, (int)xx, (int)yy, sz.x, sz.y, mem_dc, 0, 0, SRCCOPY);
}