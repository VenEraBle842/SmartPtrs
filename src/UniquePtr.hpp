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

    UniquePtr(UniquePtr&& other) noexcept : ptr(other.Release()) {}

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        Reset(other.Release());
        return *this;
    }

    template <typename U>
        requires std::derived_from<U, T>
    UniquePtr(UniquePtr<U>&& other) noexcept : ptr(other.Release()) {}

    template <typename U>
        requires std::derived_from<U, T>
    UniquePtr& operator=(UniquePtr<U>&& other) noexcept {
        Reset(other.Release());
        return *this;
    }

    T& operator*() const noexcept { return *ptr; }
    T* operator->() const noexcept { return ptr; }

    T* Get() const noexcept { return ptr; }

    T* Release() noexcept {
        T* tmp = ptr;
        ptr = nullptr;
        return tmp;
    }

    void Reset(T* p = nullptr) noexcept {
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

    UniquePtr(UniquePtr&& other) noexcept : ptr(other.Release()) {}

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        Reset(other.Release());
        return *this;
    }

    T& operator[](int index) const {
        return ptr[index];
    }

    T* Get() const noexcept { return ptr; }

    T* Release() noexcept {
        T* tmp = ptr;
        ptr = nullptr;
        return tmp;
    }

    void Reset(T* p = nullptr) noexcept {
        if (ptr != p) {
            delete[] ptr;
            ptr = p;
        }
    }

    explicit operator bool() const noexcept {
        return ptr != nullptr;
    }
};
