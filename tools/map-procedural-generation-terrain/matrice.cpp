template <typename T, size_t R, size_t C>
struct Matrix {
    Matrix() = default;
    void fill(const T &t) { arr.fill(t); }
    T& operator()(size_t r, size_t c) { return arr[R * c + r]; }
    const T& operator()(size_t r, size_t c) const { return arr[R * c + r]; }
private:
    std::array<T, C*R> arr;
};

template <typename T, size_t R, size_t C>
std::ostream & operator<<(std::ostream &os, const Matrix<T, R, C> &m) {
    for(size_t r = 0; r < R; ++r) {
        for(size_t c = 0; c < C; ++c) {
            os << m(r, c) << " ";
        }
        os << "\n";
    }
    return os;
}

int main() {
    Matrix<int, 6, 4> m;
    m.fill(0);
    m(1,1) = 2;
    m(4,3) = 5;
    std::cout << m << std::endl;
} 
