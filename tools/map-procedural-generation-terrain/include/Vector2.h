#ifndef _VECTOR2_H_
#define _VECTOR2_H_

#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <sstream>

class Vector2 {
public:
    Vector2();
    Vector2(const Vector2& v);
    Vector2(double _x, double _y);
  
    Vector2& operator=(const Vector2& a);

    const double &operator[](int n) const;
    double &operator[](int n);

	Vector2& operator+=(const Vector2& a);
    Vector2& operator-=(const Vector2& a);
    Vector2& operator*=(double s);
    Vector2 operator-() const;
    Vector2 operator+() const;
    Vector2 operator+(const Vector2 &v) const;
    Vector2 operator-(const Vector2 &v) const;
    Vector2 operator/(const double s) const;
    Vector2 operator*(const double s) const;
    //dot product
    double operator*(const Vector2 &v) const;
    double length() const;
    double lengthSquared() const;
    void normalize();
    bool operator==(const Vector2 &v) const;
    bool operator!=(const Vector2 &v) const;
    void print() const;

    static inline double signedAngle(Vector2 v1, Vector2 v2){
        double angle = atan2(v2[1], v2[0]) - atan2(v1[1], v1[0]);
        if (angle < 0) angle += 2 * M_PI;
        
        return angle;
    };

    double x, y;
};

inline Vector2 operator*(const double s, const Vector2 &v) {
    return Vector2( v[0] * s, v[1] * s);
}

inline double dot(const Vector2 &w, const Vector2 &v) {
    return w * v;
}

inline double length(const Vector2 &v) {
	return v.length();
}

inline Vector2 unit(const Vector2 &v) {
	const double len = v.length();
	return v / len;
}

inline std::ostream& operator<<(std::ostream& os, const Vector2& v) {
    os << "(" << v[0] << ", " << v[1] << ")";
    return os;
}

#endif
