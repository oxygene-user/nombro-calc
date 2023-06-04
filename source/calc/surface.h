#pragma once

struct ImageInfo
{
    int2 sz;
    signed pitch : 24; // pitch is signed and can be < 0
    unsigned bitpp : 8;

    ImageInfo():pitch(0), bitpp(0) {}
    ImageInfo(const ImageInfo &inf, const int2&sz) :sz(sz), pitch(inf.pitch), bitpp(inf.bitpp) {}
    ImageInfo(const int2&sz, u8 bitpp) :sz(sz), pitch(((bitpp + 1) >> 3) * sz.x), bitpp(bitpp) {}
    ImageInfo(const int2&sz, u8 bitpp, i16 pitch) :sz(sz), pitch(pitch), bitpp(bitpp) { ASSERT(math::abs(pitch) >= (sz.x*bytepp())); }

    int bytepp() const { return (bitpp + 1) >> 3; };
    bool operator==(const ImageInfo&d) const { return sz == d.sz && /*pitch == d.pitch &&*/ bitpp == d.bitpp; }
    bool operator!=(const ImageInfo&d) const { return sz != d.sz || /*pitch != d.pitch ||*/ bitpp != d.bitpp; }

    ImageInfo &set_width(int w) { sz.x = w; return *this; }
    ImageInfo &set_height(int h) { sz.y = h; return *this; }
    ImageInfo &set_size(const int2 &szz) { sz = szz; return *this; }

    ImageInfo chsize(const int2 &szz) const { return ImageInfo(szz, bitpp, pitch); }

    int4 rect() const { return int4(0, sz); }
};

class Surface : public ImageInfo
{
    u8*         body;
    HBITMAP     mem_bitmap = nullptr;
    HDC         mem_dc = nullptr;

public:
    ~Surface() { reset(); }
    Surface(const int2 &sz, int monitor)
    {
        create(sz, monitor);
    }
    void reset();
    void create(const int2 &sz, int monitor);
    void flush(HDC dc);
    HDC get_dc() { return mem_dc; }

    u8 *  getbody(const int2& pos)
    {
        return body + pos.x * bytepp() + pos.y * pitch;
    }
    const u8 *  getbody(const int2& pos) const
    {
        return body + pos.x * bytepp() + pos.y * pitch;
    }
    u8 *  getbody() { return body; }
    const u8 *getbody() const { return body; }
};
