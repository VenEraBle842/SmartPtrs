#pragma once
#include <concepts>
#include <type_traits>

template <typename T>
class UniquePtr {
public:
    using ElementType = std::remove_extent_t<T>;
    static constexpr bool is_array = std::is_array_v<T>;

private:
    ElementType* ptr;

    template <typename U>
    friend class UniquePtr;

    void DestroyInternal() noexcept {
        if (ptr) {
            if constexpr (is_array) {
                delete[] ptr;
            } else {
                delete ptr;
            }
            ptr = nullptr;
        }
    }

public:
    explicit UniquePtr(ElementType* p = nullptr) noexcept : ptr(p) {}

    ~UniquePtr() {
        DestroyInternal();
    }

    UniquePtr(const UniquePtr&) = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;

    UniquePtr(UniquePtr&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this != &other) {
            DestroyInternal();
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    template <typename U>
        requires (!is_array && std::derived_from<U, ElementType>)
    UniquePtr(UniquePtr<U>&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }

    template <typename U>
        requires (!is_array && std::derived_from<U, ElementType>)
    UniquePtr& operator=(UniquePtr<U>&& other) noexcept {
        if (ptr != other.ptr) {
            DestroyInternal();
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    ElementType* Release() noexcept {
        ElementType* tmp = ptr;
        ptr = nullptr;
        return tmp;
    }

    void Reset(ElementType* p = nullptr) noexcept {
        if (ptr != p) {
            DestroyInternal();
            ptr = p;
        }
    }

    ElementType& operator*() const noexcept requires (!is_array) { return *ptr; }
    ElementType* operator->() const noexcept requires (!is_array) { return ptr; }

    ElementType& operator[](int index) const noexcept requires (is_array) { return ptr[index]; }

    ElementType* Get() const noexcept { return ptr; }

    explicit operator bool() const noexcept {
        return ptr != nullptr;
    }
};
