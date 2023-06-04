#pragma once

#include <string>

class Canvas
{
    Surface * surface;
public:
    Canvas(Surface * surface) :surface(surface) {}

    int2 size() const
    {
        return surface->sz;
    }

    void fill(Color col);
    void fill_rect(const int4 &rect, Color col);
    void blend_rect(const int4 &rect, Color col, u8 a);
    void draw_edge(Color col);

    void draw_text(int x, int y, const std::wstring& s, HFONT font, Color col, const int * posses = nullptr);

};
