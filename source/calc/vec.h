#pragma once

template<typename T, int sz> class vec_t;
template <class T> struct vec_t < T, 2 >
{
    union { T x, r, s, r0, width, left; };
    union { T y, g, t, r1, height, top; };
    typedef T TYPE;
    vec_t():x(0), y(0) {}
    vec_t(T a, T b) : x(a), y(b) {}
    vec_t(T z) : x(z), y(z) {}

    bool operator ==(const vec_t<T, 2> &v) const
    {
        return x == v.x && y == v.y;
    }
    bool operator !=(const vec_t<T, 2> &v) const
    {
        return x != v.x || y != v.y;
    }
    vec_t<T, 2> operator-(vec_t<T, 2> m) const
    {
        return vec_t<T, 2>(x-m.x, y-m.y);
    }
};
using int2 = vec_t<int, 2>;

template <class T> struct vec_t < T, 3 > : public vec_t<T, 2>
{
    union { T z, b, p, r2, depth, right; };
    vec_t() : z(0) {}
    vec_t(T a, T b, T c) :vec_t<T, 2>(a, b), z(c)
    {
    }
    bool operator ==(const vec_t<T, 3> &v) const
    {
        return x == v.x && y == v.y && z == v.z;
    }
    bool operator !=(const vec_t<T, 3> &v) const
    {
        return x != v.x || y != v.y || z != v.z;
    }
};
using int3 = vec_t<int, 3>;

template <class T> struct vec_t < T, 4 > : public vec_t<T, 3>
{
    union { T w, a, q, r3, bottom; };
    vec_t() : w(0) {}
    vec_t(T a, T b, T c, T d):vec_t<T,3>(a,b,c), w(d) {}
    vec_t(T ab, vec_t<T,2> cd) :vec_t<T, 3>(ab, ab, cd.r0), w(cd.r1) {}
    bool operator ==(const vec_t<T, 4> &v) const
    {
        return x == v.x && y == v.y && z == v.z && w == v.w;
    }
    bool operator !=(const vec_t<T, 4> &v) const
    {
        return x != v.x || y != v.y || z != v.z || w != v.w;
    }

    T rect_width() const
    {
        return right - left;
    }
    T rect_height() const
    {
        return bottom - top;
    }
    vec_t<T, 2> rect_size() const
    {
        return vec_t<T, 2>(rect_width(), rect_height());
    }
    vec_t<T, 2> lt() const
    {
        return vec_t<T, 2>(left,top);
    }
	vec_t<T, 2> rb() const
	{
		return vec_t<T, 2>(right, bottom);
	}
	vec_t<T, 2> lb() const
	{
		return vec_t<T, 2>(left, bottom);
	}
	vec_t<T, 2> rt() const
	{
		return vec_t<T, 2>(right, top);
	}
};


using int4 = vec_t<int, 4>;
