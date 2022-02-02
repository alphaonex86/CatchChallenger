// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_CORE_VECTOR_HPP_
#define CLIENT_V3_CORE_VECTOR_HPP_
template <typename T>
class Vector {
  std::size_t capacity;
  std::size_t length;
  T* buffer;

  struct Deleter {
    void operator()(T* buffer) const { ::operator delete(buffer); }
  };

 public:
  Vector(int capacity = 10)
      : capacity(capacity),
        length(0),
        buffer(static_cast<T*>(::operator new(sizeof(T) * capacity))) {}

  ~Vector() {
    // Make sure the buffer is deleted even with exceptions
    // This will be called to release the pointer at the end
    // of scope.
    std::unique_ptr<T, Deleter> deleter(buffer, Deleter());

    clearElements<T>();
  }

  Vector(Vector const& copy)
      : capacity(copy.length),
        length(0),
        buffer(static_cast<T*>(::operator new(sizeof(T) * capacity))) {
    try {
      for (int loop = 0; loop < copy.length; ++loop) {
        push_back(copy.buffer[loop]);
      }
    } catch (...) {
      std::unique_ptr<T, Deleter> deleter(buffer, Deleter());
      clearElements<T>();

      // Make sure the exceptions continue propagating after
      // the cleanup has completed.
      throw;
    }
  }

  Vector& operator=(Vector const& copy) {
    copyAssign<T>(copy);
    return *this;
  }

  Vector(Vector&& move) noexcept : capacity(0), length(0), buffer(nullptr) {
    move.swap(*this);
  }

  Vector& operator=(Vector&& move) noexcept {
    move.swap(*this);
    return *this;
  }

  void swap(Vector& other) noexcept {
    using std::swap;
    swap(capacity, other.capacity);
    swap(length, other.length);
    swap(buffer, other.buffer);
  }

  void push_back(T const& value) {
    resizeIfRequire();
    pushBackInternal(value);
  }

  void pop_back() {
    --length;
    buffer[length].~T();
  }

  void reserve(std::size_t capacityUpperBound) {
    if (capacityUpperBound > capacity) {
      reserveCapacity(capacityUpperBound);
    }
  }

 private:
  void resizeIfRequire() {
    if (length == capacity) {
      std::size_t newCapacity = std::max(2.0, capacity * 1.5);
      reserveCapacity(newCapacity);
    }
  }

  void reserveCapacity(std::size_t newCapacity) {
    Vector<T> tmpBuffer(newCapacity);

    simpleCopy<T>(tmpBuffer);

    tmpBuffer.swap(*this);
  }

  void pushBackInternal(T const& value) {
    new (buffer + length) T(value);
    ++length;
  }

  void moveBackInternal(T&& value) {
    new (buffer + length) T(std::move(value));
    ++length;
  }

  template <typename X>
  typename std::enable_if<std::is_nothrow_move_constructible<X>::value ==
                          false>::type
  simpleCopy(Vector<T>& dst) {
    std::for_each(buffer, buffer + length,
                  [&dst](T const& v) { dst.pushBackInternal(v); });
  }

  template <typename X>
  typename std::enable_if<std::is_nothrow_move_constructible<X>::value ==
                          true>::type
  simpleCopy(Vector<T>& dst) {
    std::for_each(buffer, buffer + length,
                  [&dst](T& v) { dst.moveBackInternal(std::move(v)); });
  }

  template <typename X>
  typename std::enable_if<std::is_trivially_destructible<X>::value ==
                          false>::type
  clearElements() {
    // Call the destructor on all the members in reverse order
    for (int loop = 0; loop < length; ++loop) {
      // Note we destroy the elements in reverse order.
      buffer[length - 1 - loop].~T();
    }
  }

  template <typename X>
  typename std::enable_if<std::is_trivially_destructible<X>::value ==
                          true>::type
  clearElements() {
    // Trivially destructible objects can be reused without using the
    // destructor.
  }

  template <typename X>
  typename std::enable_if<(std::is_nothrow_copy_constructible<X>::value &&
                           std::is_nothrow_destructible<X>::value) ==
                          true>::type
  copyAssign(Vector<X>& copy) {
    if (this == &copy) {
      return;
    }

    if (capacity <= copy.length) {
      clearElements<T>();
      length = 0;
      for (int loop = 0; loop < copy.length; ++loop) {
        pushBackInternal(copy[loop]);
      }
    } else {
      // Copy and Swap idiom
      Vector<T> tmp(copy);
      tmp.swap(*this);
    }
  }
  template <typename X>
  typename std::enable_if<(std::is_nothrow_copy_constructible<X>::value &&
                           std::is_nothrow_destructible<X>::value) ==
                          false>::type
  copyAssign(Vector<X>& copy) {
    // Copy and Swap idiom
    Vector<T> tmp(copy);
    tmp.swap(*this);
  }
};
#endif  // CLIENT_V3_CORE_VECTOR_HPP_
