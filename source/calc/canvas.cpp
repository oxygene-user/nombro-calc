#include "pch.h"
#include <intrin.h>

void img_helper_fill(u8 *des, const ImageInfo &des_info, Color color)
{
    int desnl = des_info.pitch - des_info.sz.x*des_info.bytepp();
    int desnp = des_info.bytepp();

    switch (des_info.bytepp())
    {
    case 1:
        for (int y = 0; y < des_info.sz.y; y++, des += desnl)
            for (int x = 0; x < des_info.sz.x; x++, des += desnp)
                *(u8 *)des = (u8)color;
        break;
    case 2:
        for (int y = 0; y < des_info.sz.y; y++, des += desnl)
            for (int x = 0; x < des_info.sz.x; x++, des += desnp)
                *(u16 *)des = (u16)color;
        break;
    case 3:
        for (int y = 0; y < des_info.sz.y; y++, des += desnl) {
            for (int x = 0; x < des_info.sz.x; x++, des += desnp) {
                *(u16 *)des = (u16)color;
                *(u8 *)(des + 2) = (u8)(color >> 16);
            }
        }
        break;
    case 4:
        for (int y = 0; y < des_info.sz.y; y++, des += desnl)
            for (int x = 0; x < des_info.sz.x; x++, des += desnp)
                *(u32 *)des = color;
        break;
    }
}

namespace sse_consts
{
    ALIGN(16) u8 preparetgtc_1[16] = { 255, 0, 255, 1, 255, 2, 255, 3, 255, 4, 255, 5, 255, 6, 255, 7 };
    ALIGN(16) u8 preparetgtc_2[16] = { 255, 8, 255, 9, 255, 10, 255, 11, 255, 12, 255, 13, 255, 14, 255, 15 };
};

static const ALIGN(16) u8 preparealphas[16] = { 6, 255, 6, 255, 6, 255, 6, 255, 14, 255, 14, 255, 14, 255, 14, 255 };
static const ALIGN(16) u16 sub256[16] = { 256, 256, 256, 256, 256, 256, 256, 256 };

/*

static void alphablend_row_sse_no_clamp_aa(u8 *dst_argb, const u8 *src_argb, int w, const u16 * alpha_mul)
{
    for (; w > 0; w -= 4, dst_argb += 16, src_argb += 16)
    {
        __m128i pta1 = _mm_load_si128((const __m128i *)sse_consts::preparetgtc_1);
        __m128i pta2 = _mm_load_si128((const __m128i *)sse_consts::preparetgtc_2);
        __m128i t3 = _mm_load_si128((const __m128i *)alpha_mul);
        __m128i t4 = _mm_load_si128((const __m128i *)sub256);
        __m128i prepa = _mm_load_si128((const __m128i *)preparealphas);

        __m128i zero = _mm_setzero_si128();
        __m128i t5 = _mm_lddqu_si128((const __m128i *)src_argb);
        __m128i tt5 = _mm_mulhi_epu16(_mm_unpacklo_epi8(t5, zero), t3);
        __m128i tt6 = _mm_mulhi_epu16(_mm_unpackhi_epi8(t5, zero), t3);
        __m128i t1 = _mm_lddqu_si128((const __m128i *)dst_argb);

        _mm_storeu_si128((__m128i *)dst_argb, _mm_packus_epi16(_mm_add_epi16(_mm_mulhi_epu16(_mm_shuffle_epi8(t1, pta1), _mm_sub_epi16(t4, _mm_shuffle_epi8(tt5, prepa))), tt5), _mm_add_epi16(_mm_mulhi_epu16(_mm_shuffle_epi8(t1, pta2), _mm_sub_epi16(t4, _mm_shuffle_epi8(tt6, prepa))), tt6)));
    }
}

static void alphablend_row_sse_no_clamp(u8 *dst_argb, const u8 *src_argb, int w)
{
    for (; w > 0; w -= 4, dst_argb += 16, src_argb += 16)
    {
        __m128i pta1 = _mm_load_si128((const __m128i *)sse_consts::preparetgtc_1);
        __m128i pta2 = _mm_load_si128((const __m128i *)sse_consts::preparetgtc_2);
        __m128i t4 = _mm_load_si128((const __m128i *)sub256);
        __m128i t3 = _mm_load_si128((const __m128i *)preparealphas);

        __m128i zero = _mm_setzero_si128();
        __m128i t5 = _mm_lddqu_si128((const __m128i *)src_argb);
        __m128i tt5 = _mm_unpacklo_epi8(t5, zero);
        __m128i tt6 = _mm_unpackhi_epi8(t5, zero);
        __m128i t1 = _mm_lddqu_si128((const __m128i *)dst_argb);

        _mm_storeu_si128((__m128i *)dst_argb, _mm_packus_epi16(_mm_add_epi16(_mm_mulhi_epu16(_mm_shuffle_epi8(t1, pta1), _mm_sub_epi16(t4, _mm_shuffle_epi8(tt5, t3))), tt5), _mm_add_epi16(_mm_mulhi_epu16(_mm_shuffle_epi8(t1, pta2), _mm_sub_epi16(t4, _mm_shuffle_epi8(tt6, t3))), tt6)));
    }
}

static void img_helper_alpha_blend_pm(u8 *dst, int dst_pitch, const u8 *sou, const ImageInfo &src_info, u8 alpha, bool guaranteed_premultiplied)
{
    auto alphablend_row_1 = [](u8 *dst, const u8 *sou, int width)
    {
        for (const u8 *soue = sou + width * 4; sou < soue; dst += 4, sou += 4)
            *(Color *)dst = tools::alpha_blend_pm(*(Color *)dst, *(Color *)sou);
    };

    auto alphablend_row_1_noclamp = [](u8 *dst, const u8 *sou, int width)
    {
        for (const u8 *soue = sou + width * 4; sou < soue; dst += 4, sou += 4)
            *(Color *)dst = tools::alpha_blend_pm_no_clamp(*(Color *)dst, *(Color *)sou);
    };

    auto alphablend_row_2 = [](u8 *dst, const u8 *sou, int width, u8 alpha)
    {
        for (const u8 *soue = sou + width * 4; sou < soue; dst += 4, sou += 4)
            *(Color *)dst = tools::alpha_blend_pm(*(Color *)dst, *(Color *)sou, alpha);
    };

    if (cpu::caps(CPU_SSSE3) && guaranteed_premultiplied)
    {
        int w = src_info.sz.x & ~3;
        u8 * dst_sse = dst;
        const u8 * src_sse = sou;

        if (alpha == 255)
        {
            if (w)
                for (int y = 0; y < src_info.sz.y; ++y, dst_sse += dst_pitch, src_sse += src_info.pitch)
                    alphablend_row_sse_no_clamp(dst_sse, src_sse, w);

            if (int ost = src_info.sz.x & 3)
            {
                dst += w * 4;
                sou += w * 4;
                for (int y = 0; y < src_info.sz.y; ++y, dst += dst_pitch, sou += src_info.pitch)
                    alphablend_row_1_noclamp(dst, sou, ost);
            }
        }
        else
        {
            u16 am = (u16)(alpha + 1) << 8; // valid due alpha < 255
            ALIGN(16) u16 amul[8] = { am, am, am, am, am, am, am, am };

            if (w)
                for (int y = 0; y < src_info.sz.y; ++y, dst_sse += dst_pitch, src_sse += src_info.pitch)
                    alphablend_row_sse_no_clamp_aa(dst_sse, src_sse, w, amul);


            if (int ost = src_info.sz.x & 3)
            {
                dst += w * 4;
                sou += w * 4;
                for (int y = 0; y < src_info.sz.y; ++y, dst += dst_pitch, sou += src_info.pitch)
                    alphablend_row_2(dst, sou, ost, alpha);
            }

        }

        return;
    }

    if (alpha == 255)
    {
        if (guaranteed_premultiplied)
            for (int y = 0; y < src_info.sz.y; ++y, dst += dst_pitch, sou += src_info.pitch)
                alphablend_row_1_noclamp(dst, sou, src_info.sz.x);
        else
            for (int y = 0; y < src_info.sz.y; ++y, dst += dst_pitch, sou += src_info.pitch)
                alphablend_row_1(dst, sou, src_info.sz.x);

    }
    else
        for (int y = 0; y < src_info.sz.y; ++y, dst += dst_pitch, sou += src_info.pitch)
            alphablend_row_2(dst, sou, src_info.sz.x, alpha);
}
*/

static void img_helper_blend_fill_pm(u8 *dst, const ImageInfo &dst_info, Color fillcol, u8 aalpha)
{
    auto alphablend_row_1 = [](u8 *dst, Color col, int width)
    {
        for (const u8 *dste = dst + width * 4; dst < dste; dst += 4)
            *(Color *)dst = tools::alpha_blend_pm(*(Color *)dst, col);
    };

    auto alphablend_row_1_noclamp = [](u8 *dst, Color col, int width)
    {
        for (const u8 *dste = dst + width * 4; dst < dste; dst += 4)
            *(Color *)dst = tools::alpha_blend_pm_no_clamp(*(Color *)dst, col);
    };

    auto alphablend_row_2 = [](u8 *dst, Color col, int width, u8 alpha)
    {
        for (const u8 *dste = dst + width * 4; dst < dste; dst += 4)
            *(Color *)dst = tools::alpha_blend_pm(*(Color *)dst, col, alpha);
    };

    if (aalpha == 255)
    {
        for (int y = 0; y < dst_info.sz.y; ++y, dst += dst_info.pitch)
            alphablend_row_1_noclamp(dst, fillcol, dst_info.sz.x);
    }
    else
        for (int y = 0; y < dst_info.sz.y; ++y, dst += dst_info.pitch)
            alphablend_row_2(dst, fillcol, dst_info.sz.x, aalpha);
}

void Canvas::fill(Color col)
{
    img_helper_fill(surface->getbody(), *surface, col);
}

void Canvas::fill_rect(const int4 &rect, Color col)
{
	if (rect.left >= size().x || rect.right < 0)
		return;
	if (rect.left < 0 || rect.right > size().x)
	{
		int4 r(rect);
		if (rect.left < 0)
			r.left = 0;
		if (r.right > size().x)
			r.right = size().x;
		img_helper_fill(surface->getbody(r.lt()), surface->chsize(r.rect_size()), col);
	} else
		img_helper_fill(surface->getbody(rect.lt()), surface->chsize(rect.rect_size()), col);
}

void Canvas::blend_rect(const int4 &rect, Color col, u8 a)
{
	if (rect.left >= size().x || rect.right < 0)
		return;
	double na;
    col = tools::premultiply(col, a, na);
    img_helper_blend_fill_pm(surface->getbody(rect.lt()), surface->chsize(rect.rect_size()), col, 255);
}

void Canvas::draw_edge(Color col)
{
    int w = surface->sz.width;
    int h = surface->sz.height;
    int bpp = surface->bytepp(); // bytes per pixel
    u8 *destop = surface->getbody();
    u8 *destop_end = destop + bpp * w;
    u8 *desbottom = destop + surface->pitch * (h - 1);
    for (;destop < destop_end; destop += bpp, desbottom += bpp)
    {
        *(u32 *)destop = col;
        *(u32 *)desbottom = col;
    }

    u8 *desleft = surface->getbody() + surface->pitch;
    u8 *desleft_end = desleft + surface->pitch * (h-2);
    u8 *desrite = desleft + bpp * (w-1);
    for (; desleft < desleft_end; desleft += surface->pitch, desrite += surface->pitch)
    {
        *(u32 *)desleft = col;
        *(u32 *)desrite = col;
    }
}

void Canvas::draw_text(int x, int y, const std::wstring& s, HFONT font, Color col, const int * posses)
{
    HDC dc = surface->get_dc();
    HGDIOBJ of = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB((col >> 16) & 0xff, (col >> 8) & 0xff, (col >> 0) & 0xff));
	if (posses == nullptr)
	{
		TextOutW(dc, x, y, s.c_str(), (int)s.length());
	}
	else
	{
		for (signed_t i = 0, z = s.length(); i < z; ++i)
		{
			signed_t tx = x + posses[i];
			if (tx >= 0 && tx < size().x)
				TextOutW(dc, (int)tx, y, s.c_str() + i, 1);
		}
	}
    SelectObject(dc, of);
}

