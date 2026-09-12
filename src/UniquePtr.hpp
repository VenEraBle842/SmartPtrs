#pragma once
#include <concepts>

template <typename T>
class UniquePtr {
private:
    T* ptr;

    template <typename U>
    friend class UniquePtr;

public:
    explicit UniquePtr(T* p = nullptr) noexcept : ptr(p) {}

    ~UniquePtr() {
        delete ptr;
    }

    UniquePtr(const UniquePtr&) = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;

    UniquePtr(UniquePtr&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this != &other) {
            delete ptr;
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    template <typename U>
        requires std::derived_from<U, T>
    UniquePtr(UniquePtr<U>&& other) noexcept : ptr(other.release()) {}

    template <typename U>
        requires std::derived_from<U, T>
    UniquePtr& operator=(UniquePtr<U>&& other) noexcept {
        reset(other.release());
        return *this;
    }

    T& operator*() const noexcept { return *ptr; }
    T* operator->() const noexcept { return ptr; }

    T* get() const noexcept { return ptr; }

    T* release() noexcept {
        T* tmp = ptr;
        ptr = nullptr;
        return tmp;
    }

    void reset(T* p = nullptr) noexcept {
        if (ptr != p) {
            delete ptr;
            ptr = p;
        }
    }

    explicit operator bool() const noexcept {
        return ptr != nullptr;
    }
};

template <typename T>
class UniquePtr<T[]> {
private:
    T* ptr;

public:
    explicit UniquePtr(T* p = nullptr) noexcept : ptr(p) {}

    ~UniquePtr() {
        delete[] ptr;
    }

    UniquePtr(const UniquePtr&) = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;

    UniquePtr(UniquePtr&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this != &other) {
            delete[] ptr;
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    T& operator[](int index) const {
        return ptr[index];
    }

    T* get() const noexcept { return ptr; }

    T* release() noexcept {
        T* tmp = ptr;
        ptr = nullptr;
        return tmp;
    }

    void reset(T* p = nullptr) noexcept {
        if (ptr != p) {
            delete[] ptr;
            ptr = p;
        }
    }

    explicit operator bool() const noexcept {
        return ptr != nullptr;
    }
};
