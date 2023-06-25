#pragma once

#if defined (_M_AMD64) || defined (_M_X64) || defined (WIN64) || defined(__LP64__)
#define MODE64
#define ARCHBITS 64
#else
#define ARCHBITS 32
#endif

typedef signed short i16;
typedef ptrdiff_t signed_t;


#include <emmintrin.h>
#include <immintrin.h>
#include <intrin.h>
#include <sstream>
#include <string>

#include "logger.h"

#include "str_helpers.h"

#define ONEBIT(x) (static_cast<size_t>(1)<<(x))

#define PTR_TO_UNSIGNED( p ) ((size_t)p)
#define INLINE __inline
#define ALIGN(n) __declspec( align( n ) )

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef unsigned __int64 u64;
#ifdef _MSC_VER
#ifdef MODE64
struct u128
{
	u64 low;
	u64 hi;

    u128() {}
	u128(u64 v):low(v), hi(0)
	{
	}

    operator u64() const
    {
        return low;
    }

	bool operator >= (u64 v) const
	{
		return hi > 0 || low >= v;
	}

    u128& operator=(u64 v)
    {
        low = v;
        hi = 0;
        return *this;
    }

    u128& operator-=(u128 v)
    {
        if (low < v.low)
        {
            hi -= v.hi - 1;
            low -= v.low;
        }
        else
        {
			hi -= v.hi;
			low -= v.low;
        }
        return *this;
    }
	u128& operator+=(u64 v)
	{
        _addcarry_u64(_addcarry_u64(0, low, v, &low), hi, 0, &hi);
		return *this;
	}
	u128& operator+=(const u128 &v)
	{
		_addcarry_u64(_addcarry_u64(0, low, v.low, &low), hi, v.hi, &hi);
		return *this;
	}
};
#endif
#endif

#ifdef MODE64
typedef u64 usingle;
typedef u128 udouble;
typedef u32 uhalf;
#else
typedef u16 uhalf;
typedef u32 usingle;
typedef u64 udouble;
#endif



typedef unsigned long Color;

inline bool is_debugger_present()
{
#ifdef _WIN32
	return IsDebuggerPresent() != FALSE;
#endif
    return false;
}


#define DEBUG_BREAK() __debugbreak()
#ifndef _DEBUG
#define SMART_DEBUG_BREAK (is_debugger_present() ? DEBUG_BREAK(), false : false)
#else
#define SMART_DEBUG_BREAK DEBUG_BREAK() // always break in debug
#endif

#define ASSERT(expr,...) NOWARNING(4800, ((expr) || (ERRORM(__FILE__, __LINE__, __VA_ARGS__) ? (SMART_DEBUG_BREAK, false) : false))) // (...) need to make possible syntax: ASSERT(expr, "Message")
//#define ASSERTS(expr, ...) ASSERTO(expr, (std::stringstream() << ""  __VA_ARGS__).str())

bool messagebox(const char *s1, const char *s2, int options);

#define MESSAGE(...) messagebox("#", build_string(__VA_ARGS__).c_str(), MB_OK|MB_ICONINFORMATION)
#define WARNING(...) messagebox("!?", build_string(__VA_ARGS__).c_str(), MB_OK|MB_ICONWARNING)
#define ERRORM(fn, ln, ...) messagebox("!!!", build_string(fn, ln, __VA_ARGS__).c_str(), MB_OK|MB_ICONERROR)
template <typename T> INLINE T * BREAK_ON_NULL(T * ptr, const char *file, int line) { if (ptr == nullptr) { WARNING("nullptr pointer conversion: %s:%i", file, line); } return ptr; }
#define NOT_NULL( x ) BREAK_ON_NULL(x, __FILE__, __LINE__)
template<typename PTRT, typename TF> INLINE PTRT ptr_cast(TF *p) { if (!p) return nullptr; return NOT_NULL(dynamic_cast<PTRT>(p)); }
#define NOWARNING(n,...) __pragma(warning(push)) __pragma(warning(disable:n)) __VA_ARGS__ __pragma(warning(pop))

template<typename Tout, typename Tin> Tout& ref_cast(Tin& t)
{
	static_assert(sizeof(Tout) <= sizeof(Tin), "ref cast fail");
	return (Tout&)t;
}
template<typename Tout, typename Tin> const Tout& ref_cast(const Tin& t) //-V659
{
	static_assert(sizeof(Tout) <= sizeof(Tin), "ref cast fail");
	return *(const Tout*)&t;
}

template<typename Tout, typename Tin> const Tout& ref_cast(const Tin& t1, const Tin& t2)
{
	static_assert(sizeof(Tout) <= (sizeof(Tin) * 2), "ref cast fail");
	ASSERT(((u8*)&t1) + sizeof(Tin) == (u8*)&t2);
	return *(const Tout*)&t1;
}

namespace math
{
    template<typename T> struct is_signed { static const bool value = (((T)-1) < 0); };
    template<> struct is_signed<float> { static const bool value = true; };
    template<> struct is_signed < double > { static const bool value = true; };

    INLINE long int fround(float x)
    {
        return _mm_cvtss_si32(_mm_load_ss(&x));
    }

    INLINE long int dround(double x)
    {
        return _mm_cvtsd_si32(_mm_load_sd(&x));
    }

}

namespace helpers
{
    template<typename IT> u8 INLINE clamp2byte(IT n)
    {
        return n < 0 ? 0 : (n > 255 ? 255 : (u8)n);
    }

    template<typename IT> u8 INLINE clamp2byte_u(IT n)
    {
        return n > 255 ? 255 : (u8)n;
    }

    template<typename RT, typename IT, bool issigned> struct clamper;

    template<> struct clamper < u8, float, true >
    {
        static u8 dojob(float b)
        {
            return clamp2byte<int>(math::fround(b));
        }
    };

    template<> struct clamper < u8, double, true >
    {
        static u8 dojob(double b)
        {
            return clamp2byte<int>(math::dround(b));
        }
    };

    template<typename IT> struct clamper < u8, IT, true >
    {
        static u8 dojob(IT n)
        {
            return clamp2byte<IT>(n);
        }
    };

    template<typename IT> struct clamper < u8, IT, false >
    {
        static u8 dojob(IT n)
        {
            return clamp2byte_u<IT>(n);
        }
    };

    template<typename RT, typename IT> struct clamper< RT, IT, false>
    {
        static RT dojob(IT b)
        {
            return b > maximum<RT>::value ? maximum<RT>::value : (RT)b;
        }
    };
    template<typename RT, typename IT> struct clamper < RT, IT, true >
    {
        static RT dojob(IT b)
        {
            return b < minimum<RT>::value ? minimum<RT>::value : (b > maximum<RT>::value ? maximum<RT>::value : (RT)b);
        }
    };

    template<bool s1, bool s2, typename T1, typename T2> struct getminmax
    {
        typedef T1 type;
        static T1 getmin(T1 t1, T2 t2)
        {
            return t1 < t2 ? t1 : t2;
        }
    };
	template<typename T1, typename T2> struct getminmax<true, false, T1, T2> {

        typedef T1 type;
        static T1 getmin(T1 t1, T2 t2)
		{
			return t1 < 0 || (size_t)t1 < t2 ? t1 : (T1)t2;
		}
    };
	template<typename T1, typename T2> struct getminmax<false, true, T1, T2> {

        typedef T2 type;
		static T2 getmin(T1 t1, T2 t2)
		{
			return t2 < 0 || (size_t)t2 < t1 ? t2 : (T2)t1;
		}
	};


};

namespace math
{
    template < typename T1, typename T2, typename T3 > INLINE T1 clamp(const T1 & a, const T2 & vmin, const T3 & vmax)
    {
        return (T1)(((a) > (vmax)) ? (vmax) : (((a) < (vmin)) ? (vmin) : (a)));
    }

    template < typename RT, typename IT > INLINE RT clamp(IT b)
    {
        return helpers::clamper<RT, IT, is_signed<IT>::value>::dojob(b);
    }

}

namespace tools
{
    template<typename CC> INLINE bool is_space(CC c)
    {
        return c == 32 || c == '\n' || c == '\r' || c == '\t';
    }

	template<typename CC> INLINE void ltrim(std::basic_string<CC>& s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](CC ch) {
			return !is_space(ch);
		}));
	}

	// trim from end (in place)
    template<typename CC> INLINE void rtrim(std::basic_string<CC>& s) {
		//s.erase(std::find_if(s.rbegin(), s.rend(), [](CC ch) {
			//return !is_space(ch);
		//}).base(), s.end());

        signed_t i = s.length() - 1;
        for (; i >= 0 && is_space(s[i]); --i);
        ++i;
        if ((size_t)i < s.length())
            s.resize(i);
	}

	// trim from both ends (in place)
    template<typename CC> INLINE void trim(std::basic_string<CC>& s) {
		rtrim(s);
		ltrim(s);
	}

    template<class T> INLINE void swap(T& first, T& second)
    {
        T temp = std::move(first);
        first = std::move(second);
        second = std::move(temp);
    }

    template <typename CCC> INLINE Color ARGB(CCC r, CCC g, CCC b, CCC a = 255)
    {
        return math::clamp<u8, CCC>(b) | (math::clamp<u8, CCC>(g) << 8) | (math::clamp<u8, CCC>(r) << 16) | (math::clamp<u8, CCC>(a) << 24);
    }

	//u8 INLINE as_byte(signed_t b) { return static_cast<u8>(b & 0xFF); }
    //u8 INLINE as_byte(size_t b) { return static_cast<u8>(b & 0xFF); }
	u8 INLINE as_byte(u64 b) { return static_cast<u8>(b & 0xFF); }
#ifdef MODE64
	u8 INLINE as_byte(u128 b) { return as_byte((u64)b); }
#endif
    wchar_t INLINE as_wchar(size_t x) { return static_cast<wchar_t>(x & 0xFFFF); }

    u8 INLINE RED(Color c) { return as_byte(c >> 16); }
    u8 INLINE GREEN(Color c) { return as_byte(c >> 8); }
    u8 INLINE BLUE(Color c) { return as_byte(c); }
    u8 INLINE ALPHA(Color c) { return as_byte(c >> 24); }


    Color INLINE alpha_blend(Color target, Color source, int constant_alpha = 255) // As Photoshop Normal mode color blending
    {
        u8 oA = ALPHA(target);

        if (oA == 0)
        {
            u32 a = math::dround(double(constant_alpha * ALPHA(source)) * (1.0 / 255.0));
            return (0x00FFFFFF & source) | (math::clamp<u8>(a) << 24);
        }

        u8 oR = RED(target);
        u8 oG = GREEN(target);
        u8 oB = BLUE(target);

        u8 R = RED(source);
        u8 G = GREEN(source);
        u8 B = BLUE(source);


        float A = float(double(constant_alpha * ALPHA(source)) * (1.0 / (255.0 * 255.0)));
        float nA = 1.0f - A;

        unsigned oiA = math::fround(255.0f * A + float(oA) * nA);

        float k = 0;
        if (oiA) k = 1.0f - (A * 255.0f / (float)oiA);

        unsigned oiB = math::fround(float(B) * A + float(oB) * k);
        unsigned oiG = math::fround(float(G) * A + float(oG) * k);
        unsigned oiR = math::fround(float(R) * A + float(oR) * k);

        return ARGB<unsigned>(oiR, oiG, oiB, oiA);
    }

    Color INLINE alpha_blend_pm_no_clamp(Color dst, Color src) // premultiplied alpha blend
    {
        extern u8 ALIGN(256) multbl[256][256];

        u8 not_a = 255 - ALPHA(src);

        return src + ((multbl[not_a][dst & 0xff]) |
            (((unsigned)multbl[not_a][(dst >> 8) & 0xff]) << 8) |
            (((unsigned)multbl[not_a][(dst >> 16) & 0xff]) << 16) |
            (((unsigned)multbl[not_a][(dst >> 24) & 0xff]) << 24));
    }

    bool INLINE is_premultiplied(Color c)
    {
        u8 a = ALPHA(c);
        return RED(c) <= a && GREEN(c) <= a && BLUE(c) <= a;
    }


    Color INLINE alpha_blend_pm(Color dst, Color src) // premultiplied alpha blend
    {
        if (is_premultiplied(src))
            return alpha_blend_pm_no_clamp(dst, src);

        extern u8 ALIGN(256) multbl[256][256];

        u8 not_a = 255 - ALPHA(src);

        unsigned B = multbl[not_a][BLUE(dst)] + BLUE(src);
        unsigned G = multbl[not_a][GREEN(dst)] + GREEN(src);
        unsigned R = multbl[not_a][RED(dst)] + RED(src);
        unsigned A = multbl[not_a][ALPHA(dst)] + ALPHA(src);

        return math::clamp<u8>(B) | (math::clamp<u8>(G) << 8) | (math::clamp<u8>(R) << 16) | (A << 24);
    }

    INLINE Color premultiply(Color c, u8 aa, double &not_a) // premultiply with addition alpha and return not-alpha
    {
        double a = ((double)(ALPHA(c) * aa) * (1.0 / 65025.0));
        not_a = 1.0 - a;

        unsigned oiB = math::dround(double(BLUE(c)) * a);
        unsigned oiG = math::dround(double(GREEN(c)) * a);
        unsigned oiR = math::dround(double(RED(c)) * a);
        unsigned oiA = math::dround(a * 255.0);

        return ARGB<unsigned>(oiR, oiG, oiB, oiA);
    }

    Color INLINE multiply(Color c1, Color c2)
    {
        extern u8 ALIGN(256) multbl[256][256];

        return multbl[c1 & 0xff][c2 & 0xff] |
            ((unsigned)multbl[(c1 >> 8) & 0xff][(c2 >> 8) & 0xff] << 8) |
            ((unsigned)multbl[(c1 >> 16) & 0xff][(c2 >> 16) & 0xff] << 16) |
            ((unsigned)multbl[(c1 >> 24) & 0xff][(c2 >> 24) & 0xff] << 24);

    }


    Color INLINE alpha_blend_pm(Color dst, Color src, u8 calpha) // premultiplied alpha blend with addition constant alpha
    {
        if (calpha == 0) return dst;
        return alpha_blend_pm(dst, multiply(src, ARGB(calpha, calpha, calpha, calpha)));
    }



}


namespace math
{

    template<typename NUM, int shiftval> struct makemaxint
    {
        static const NUM value = (makemaxint<NUM, shiftval - 1>::value << 8) | 0xFF;
    };
    template<typename NUM> struct makemaxint < NUM, 0 >
    {
        static const NUM value = 0x7F;
    };
    template<typename NUM> struct maximum
    {
        static const NUM value = is_signed<NUM>::value ? makemaxint<NUM, sizeof(NUM) - 1>::value : (NUM)(-1);
    };
    template<typename NUM> struct minimum
    {
        static const NUM value = is_signed<NUM>::value ? (-maximum<NUM>::value - 1) : 0;
    };

    template < typename T1 > INLINE T1 abs(const T1 &x)
    {
        return x >= 0 ? x : (-x);
    }

    int INLINE lerp_int(int a, int b, float t)
    {
        float v = static_cast<float>(a) * (1.0f - (t)) + (t) * static_cast<float>(b);
        return fround(v);
    }

	template < typename T1, typename T2 > INLINE typename helpers::getminmax<is_signed<T1>::value, is_signed<T2>::value, T1, T2>::type minv(const T1& x1, const T2& x2)
	{
        return helpers::getminmax<is_signed<T1>::value, is_signed<T2>::value, T1, T2>::getmin(x1, x2);
	}

	template < typename T > INLINE T nmax(const T& x, const T& y)
	{
		return x >= y ? x : y;
	}

    /*
	template < typename T > INLINE T nmin(const T& x, const T& y)
	{
		return x <= y ? x : y;
	}
    */


#ifdef _MSC_VER
#ifdef MODE64
	__forceinline void mul100add(udouble &d, usingle z) // d = d * 100 + z
	{
        __if_exists(_umul128) {

            d.low = _umul128(100ull, d.low, &d.hi);
            d += z;
        }
        __if_not_exists(_umul128) {
            __debugbreak(); // sorry, only vs2019+ supported (no _umul128 intrinsinc in older versions)
		}

	}
    __forceinline usingle div(udouble d, usingle v, usingle *rm = nullptr)
	{
		__if_exists(_udiv128) {

            usingle rm1;
            if (rm == nullptr)
                rm = &rm1;
			return _udiv128(d.hi, d.low, v, rm);
		}

		__if_not_exists(_udiv128) {
			return 0; // sorry, only vs2019+ supported (no _udiv128 intrinsinc in older versions)
		}

	}
    __forceinline void mul(udouble &d, usingle v) // d = d.low * v
	{
		__if_exists(_umul128) {
			d.low = _umul128(v, d.low, &d.hi);
		}

		__if_not_exists(_umul128) {
            __debugbreak(); // sorry, only vs2019+ supported (no _umul128 intrinsinc in older versions)
		}

	}
	__forceinline void mulplus(udouble& d, usingle v1, usingle v2) // d = d + v1 * v2
	{
		__if_exists(_umul128) {
            udouble m;
			m.low = _umul128(v1, v2, &m.hi);
            d += m;
		}

		__if_not_exists(_umul128) {
			__debugbreak(); // sorry, only vs2019+ supported (no _umul128 intrinsinc in older versions)
		}

	}
#else
	__forceinline void mul100add(udouble &d, usingle z) // d = d * 100 + z
	{
		__if_exists(__emulu)
		{
			d = __emulu(d & 0xffffffff, 100) + z;
		}
        __if_not_exists(__emulu)
        {
            _asm
            {
                mov ecx, d
                mov eax, 100
                mul dword ptr[ecx]
                add eax, z
                mov dword ptr[ecx], eax
                adc edx, 0
                mov dword ptr[ecx + 4], edx
            }
        }

	}
	__forceinline usingle div(const udouble &d, usingle v) // 64 bit value (div) 32 bit value using native x86 div
	{
		__if_exists(_udiv64) {

			unsigned int rm1;
			return _udiv64(d, v, &rm1);
		}

		__if_not_exists(_udiv64) {
			_asm {
				mov ecx, d
				mov eax, [ecx]
				mov edx, [ecx + 4]
				div v
			}
		}

	}
	__forceinline usingle div(const udouble &d, usingle v, usingle* rm) // 64 bit value (div) 32 bit value using native x86 div
	{
		__if_exists(_udiv64) {
            unsigned int rm1;
            if (rm == nullptr)
                rm = (usingle *) & rm1;
			return _udiv64(d, v, (unsigned int *)rm);
		}

        __if_not_exists(_udiv64) {

			if (rm == nullptr)
				return div(d, v);

            _asm {
                mov ecx, d
                mov eax, [ecx]
                mov edx, [ecx + 4]
                mov ecx, rm
                div v
                mov[ecx], edx
            }
        }
	}
	__forceinline void mul(udouble& d, usingle v) // d = d.low * v
	{
		__if_exists(__emulu)
		{
			d += __emulu(d &0xffffffff, v);
		}

		__if_not_exists(__emulu)
		{
            _asm {
                mov ecx, d
                mov eax, dword ptr[ecx]
                mul v
                mov[ecx], eax
                mov[ecx + 4], edx
            }
		}
	}
	__forceinline void mulplus(udouble& d, usingle v1, usingle v2) // d += v1 * v2
	{
        __if_exists(__emulu)
        {
            d += __emulu(v1, v2);
        }

		__if_not_exists(__emulu)
		{
			_asm {
				mov ecx, d
				mov eax, v1
				mul v2
				add dword ptr[ecx], eax
				adc dword ptr[ecx + 4], edx
			}
		}
	}
#endif

#endif
}

namespace ptr
{
    /*
        intrusive shared pointer

        example:
        shared_ptr<MyClass> p(new MyClass(...)), p2(p), p3=p;
        . . .
    */

    template <class T> class shared_ptr // T must be public child of shared_object
    {
        T *object = nullptr;

        void unconnect()
        {
            if (object) T::dec_ref(object);
        }

        void connect(T *p)
        {
            object = p;
            if (object) object->add_ref();
        }

    public:
        shared_ptr() {}
        //shared_ptr(const T &obj):object(new T (obj)) {object->ref = 1;}
        shared_ptr(T *p) { connect(p); } // now safe todo: shared_ptr p1(obj), p2(obj);
        shared_ptr(const shared_ptr &p) { connect(p.object); }
        shared_ptr(shared_ptr &&p) :object(p.object) { p.object = nullptr; }

        shared_ptr &operator=(T *p)
        {
            if (p) p->add_ref(); // ref up - to correct self assign
            unconnect();
            object = p;
            return *this;
        }
        shared_ptr &operator=(const shared_ptr &p)
        {
            return *this = p.object;
        }

		shared_ptr& operator=(shared_ptr&& p)
		{
            unconnect();
            object = p.object;
            p.object = nullptr;
            return *this;
		}

        ~shared_ptr() { unconnect(); }

        void swap(shared_ptr &p) { math::swap(*this, p); }

        operator T *() const { return object; }
        T *operator->() const { return object; }

        T *get() { return object; }
        const T *get() const { return object; }
    };

    struct intref
    {
        int value = 0;

        intref& operator++()
        {
            ++value;
            return *this;
        }
		intref& operator--()
		{
			--value;
			return *this;
		}

        bool operator()()
        {
            ASSERT(value > 0);
            return --value == 0;
        }
        bool operator *() const
        {
            return value > 1;
        }
    };

	struct intref_sync
	{
		std::atomic<ptrdiff_t> value = 0;

		intref_sync& operator++()
		{
			++value;
			return *this;
		}

		intref_sync& operator--()
		{
			--value;
			return *this;
		}

		bool operator()()
		{
			ptrdiff_t nv = --value;
			ASSERT(nv >= 0);
			return nv == 0;
		}
		bool operator *() const
		{
			return value > 1;
		}
	};

    struct DELETER
    {
        template<typename T> static void kill(T* o)
        {
            delete o;
        }
    };

	struct RELEASER
	{
		template<typename T> static void kill(T* o)
		{
			o->release();
		}
	};

    template<typename REF, typename OKILLER = DELETER> class shared_object_t
    {
        mutable REF ref;

        shared_object_t(const shared_object_t &) = delete;
        void operator=(const shared_object_t &) = delete;

    public:
        shared_object_t() {}

        bool is_multi_ref() const { return *ref; }
        void add_ref() { ++ref; }
		void dec_ref_no_check() const { --ref; }
        template <class T> static void dec_ref(T *object)
        {
            if (object->ref())
                OKILLER::kill(object);
        }
    };

    using shared_object = shared_object_t<intref>;
	using sync_shared_object = shared_object_t<intref_sync>;
    template <typename KILLER> using sync_shared_object_ck = shared_object_t<intref_sync, KILLER>; // with custom killer

	// intrusive UNMOVABLE weak pointer
    // UNMOVABLE means that you cannot use memcpy to copy this pointer

	template<class OO> struct eyelet_s;
	template<class OO, class OO1 = OO> struct iweak_ptr
	{
		friend struct eyelet_s<OO>;
	private:
		iweak_ptr* prev = nullptr;
		iweak_ptr* next = nullptr;
		OO* oobject = nullptr;

	public:

		iweak_ptr() {}
		iweak_ptr(const iweak_ptr& hook)
		{
			if (hook.get()) const_cast<OO*>(static_cast<const OO*>(hook.get()))->hook_connect(this);
		}

		iweak_ptr(OO1* ob)
		{
			if (ob) ((OO*)ob)->OO::hook_connect(this);
		}
		~iweak_ptr()
		{
			unconnect();
		}

		void unconnect()
		{
			if (oobject) oobject->hook_unconnect(this);
		}

		iweak_ptr& operator = (const iweak_ptr& hook)
		{
			if (hook.get() != get())
			{
				unconnect();
				if (hook.get()) const_cast<OO*>(hook.get())->hook_connect(this);
			}
			return *this;
		}

		iweak_ptr& operator = (OO1* obj)
		{
			if (obj != get())
			{
				unconnect();
				if (obj) obj->OO::hook_connect(this);
			}
			return *this;
		}

		explicit operator bool() { return get() != nullptr; }

		template<typename OO2> bool operator==(const OO2* obj) const { return oobject == ptr_cast<const OO2*>(obj); }

		OO1* operator()() { return static_cast<OO1*>(oobject); }
		const OO1* operator()() const { return static_cast<const OO1*>(oobject); }

		operator OO1* () const { return static_cast<OO1*>(oobject); }
		OO1* operator->() const { return static_cast<OO1*>(oobject); }

		OO1* get() { return static_cast<OO1*>(oobject); }
		const OO1* get() const { return static_cast<OO1*>(oobject); }

		bool expired() const { return get() == nullptr; }
	};

	template<class OO> struct eyelet_s
	{
		iweak_ptr<OO>* first = nullptr;

		eyelet_s() {}
		~eyelet_s()
		{
			iweak_ptr<OO>* f = first;
			for (; f;)
			{
				iweak_ptr<OO>* next = f->next;

				f->oobject = nullptr;
				f->prev = nullptr;
				f->next = nullptr;

				f = next;
			}
		}

		void connect(OO* object, iweak_ptr<OO, OO>* hook)
		{
			if (hook->get()) hook->get()->hook_unconnect(hook);
			hook->oobject = object;
			hook->prev = nullptr;
			hook->next = first;
			if (first) first->prev = hook;
			first = hook;
		}

		void    unconnect(iweak_ptr<OO, OO>* hook)
		{
#ifdef _DEBUG
			iweak_ptr<OO>* f = first;
			for (; f; f = f->next)
			{
				if (f == hook) break;
			}
			ASSERT(f == hook, "foreigner hook!!!");

#endif
			if (first == hook)
			{
				ASSERT(first->prev == nullptr);
				first = hook->next;
				if (first)
				{
					first->prev = nullptr;
				}
				hook->next = nullptr;
			}
			else
			{
				ASSERT(hook->prev != nullptr);
				hook->prev->next = hook->next;
				if (hook->next) { hook->next->prev = hook->prev; hook->next = nullptr; };
				hook->prev = nullptr;
			}
			hook->oobject = nullptr;
		}
	};

}

#define DECLARE_EYELET( obj ) private: ptr::eyelet_s<obj> _ptr_eyelet; public: \
	template<class OO1> void hook_connect( ptr::iweak_ptr<obj, OO1> * hook ) { _ptr_eyelet.connect(this, reinterpret_cast<ptr::iweak_ptr<obj>*>(hook)); } \
	template<class OO1> void hook_unconnect( ptr::iweak_ptr<obj, OO1> * hook ) { _ptr_eyelet.unconnect(reinterpret_cast<ptr::iweak_ptr<obj>*>(hook)); } private:


#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)

#define __STR3W__(x) L ## x
#define __STR2W__(x) __STR3W__( #x )
#define __STR1W__(x) __STR2W__(x)

#define LIST_ADD(el,first,last,prev,next) {if((last)!=nullptr) {(last)->next=el;} (el)->prev=(last); (el)->next=nullptr;  last=(el); if(first==nullptr) {first=(el);}}
#define LIST_DEL(el,first,last,prev,next) \
    {if((el)->prev!=0) (el)->prev->next=el->next;\
        if((el)->next!=0) (el)->next->prev=(el)->prev;\
        if((last)==(el)) last=(el)->prev;\
    if((first)==(el)) (first)=(el)->next;}

#include "vec.h"
#include "surface.h"
#include "canvas.h"
#include "sts.h"
#include "fsys.h"

namespace str
{
	template<typename CH> std::basic_string<CH> replace_all_copy(const std::basic_string<CH>& source, const std::basic_string_view<CH>&what, const std::basic_string_view<CH>& to)
	{
		std::string new_string;
        new_string.reserve(source.length());

		std::string::size_type lastPos = 0;
		std::string::size_type findPos;

		while (std::string::npos != (findPos = source.find(from, lastPos)))
		{
            new_string.append(source, lastPos, findPos - lastPos);
            new_string += to;
			lastPos = findPos + from.length();
		}

		// Care for the rest after last occurrence
        new_string += source.substr(lastPos);

        return new_string;
	}
}