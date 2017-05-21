#include "../include/Vector2.h"

Vector2::Vector2() : x(0), y(0) {}

Vector2::Vector2(const Vector2& v) : x(v[0]), y(v[1]) {}

Vector2::Vector2(double _x, double _y) : x(_x), y(_y) {}

Vector2& Vector2::operator=(const Vector2& a) {
    x = a[0];
    y = a[1];
    return *this;
}

const double& Vector2::operator[](int n) const {
	return (&x)[n];
}

double& Vector2::operator[](int n) {
	return (&x)[n];
}

Vector2& Vector2::operator+=(const Vector2& a) {
    x += a[0];
    y += a[1];
    return *this;
}

Vector2& Vector2::operator-=(const Vector2& a) {
    x -= a[0];
    y -= a[1];
    return *this;
}

Vector2& Vector2::operator*=(double s) {
    x *= s;
    y *= s;
    return *this;
}

Vector2 Vector2::operator-() const {
    return Vector2(-x, -y);
}

Vector2 Vector2::operator+() const {
    return *this;
}

Vector2 Vector2::operator+(const Vector2 &v) const {
    return Vector2( x + v.x, y + v.y);
}

Vector2 Vector2::operator-(const Vector2 &v) const {
    return Vector2( x - v.x, y - v.y);
}

Vector2 Vector2::operator/(const double s) const {
    return Vector2( x / s, y / s);
}

Vector2 Vector2::operator*(const double s) const {
    return Vector2( x * s, y * s);
}

// Dot Product
double Vector2::operator*(const Vector2 &v) const {
    return x * v.x + y * v.y;
}

double Vector2::length() const {
    return (double) sqrt(x * x + y * y);
}

double Vector2::lengthSquared() const {
    return x * x + y * y;
}

void Vector2::normalize() {
    double s = 1.0 / (double) sqrt(x * x + y * y);
    x *= s;
    y *= s;
}

bool Vector2::operator==(const Vector2 &v) const {
    return x == v.x && y == v.y;
}

bool Vector2::operator!=(const Vector2 &v) const {
    return x != v.x || y != v.y;
}

void Vector2::print() const {
    std::cout << x << " " << y << "\n";
}

