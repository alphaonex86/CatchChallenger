#include "../include/Point2.h"

#include <math.h>

Point2::Point2() : x(0), y(0) {}

Point2::Point2(const Point2& p) : x(p[0]), y(p[1]) {}

Point2::Point2(double _x, double _y) : x(_x), y(_y) {}

Point2& Point2::operator=(const Point2& a) {
    x = a[0];
    y = a[1];
    return *this;
}

const double& Point2::operator[](int n) const {
	return (&x)[n];
}

double& Point2::operator[](int n) {
	return (&x)[n];
}

Point2& Point2::operator+=(const Vector2& v) {
    x += v[0];
    y += v[1];
    return *this;
}

Point2& Point2::operator-=(const Vector2& v) {
    x -= v[0];
    y -= v[1];
    return *this;
}

Point2& Point2::operator*=(double s) {
    x *= s;
    y *= s;
    return *this;
}

Vector2 Point2::operator-(const Point2 & p) const {
    return Vector2(x - p.x, y - p.y);
}

Point2 Point2::operator+(const Vector2 & v) const {
    return Point2(x + v[0], y + v[1]);
}

Point2 Point2::operator-(const Vector2 & v) const {
    return Point2(x - v[0], y - v[1]);
}

double Point2::distanceTo(const Point2& p) const {
    return sqrt((p[0] - x) * (p[0] - x) + 
                (p[1] - y) * (p[1] - y));
}

double Point2::distanceToSquared(const Point2& p) const {
    return ((p[0] - x) * (p[0] - x) +
            (p[1] - y) * (p[1] - y));
}

double Point2::distanceFromOrigin() const {
    return sqrt(x * x + y * y);
}

double Point2::distanceFromOriginSquared() const {
    return x * x + y * y;
}

bool Point2::operator==( const Point2 &p ) const {
    return x == p.x && y == p.y;
}

bool Point2::operator!=( const Point2 &p ) const {
    return x != p.x || y != p.y;
}

void Point2::print() const {
    std::cout << x << " " << y << "\n";
}
