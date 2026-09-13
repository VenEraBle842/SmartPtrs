#pragma once
#include <concepts>

template <typename T>
class SharedPtr {
private:
    T* ptr;
    int* ref_cnt;

    void ReleaseInternal() noexcept {
        if (ref_cnt) {
            --(*ref_cnt);
            if (*ref_cnt == 0) {
                delete ptr;
                delete ref_cnt;
            }
            ptr = nullptr;
            ref_cnt = nullptr;
        }
    }

    template <typename U>
    friend class SharedPtr;

public:
    explicit SharedPtr(T* p = nullptr)
        : ptr(p), ref_cnt(p ? new int(1) : nullptr) {}

    ~SharedPtr() {
        ReleaseInternal();
    }

    SharedPtr(const SharedPtr& other) noexcept
        : ptr(other.ptr), ref_cnt(other.ref_cnt) {
        if (ref_cnt) {
            ++(*ref_cnt);
        }
    }

    SharedPtr& operator=(const SharedPtr& other) noexcept {
        if (this == &other || ref_cnt == other.ref_cnt) {
            return *this;
        }

        ReleaseInternal();

        ptr = other.ptr;
        ref_cnt = other.ref_cnt;

        if (ref_cnt) {
            ++(*ref_cnt);
        }

        return *this;
    }

    SharedPtr(SharedPtr&& other) noexcept
        : ptr(other.ptr), ref_cnt(other.ref_cnt) {
        other.ptr = nullptr;
        other.ref_cnt = nullptr;
    }

    SharedPtr& operator=(SharedPtr&& other) noexcept {
        if (this != &other) {
            ReleaseInternal();

            ptr = other.ptr;
            ref_cnt = other.ref_cnt;

            other.ptr = nullptr;
            other.ref_cnt = nullptr;
        }
        return *this;
    }

    // Копирующий конструктор от подтипа: SharedPtr<base> b = d;
    template <typename U>
        requires std::derived_from<U, T>
    SharedPtr(const SharedPtr<U>& other) noexcept
        : ptr(other.ptr), ref_cnt(other.ref_cnt) {
        if (ref_cnt) {
            ++(*ref_cnt);
        }
    }

    // Копирующий оператор присваивания от подтипа: b = d;
    template <typename U>
        requires std::derived_from<U, T>
    SharedPtr& operator=(const SharedPtr<U>& other) noexcept {
        if (ref_cnt == other.ref_cnt) {
            return *this;
        }

        ReleaseInternal();

        ptr = other.ptr;
        ref_cnt = other.ref_cnt;

        if (ref_cnt) {
            ++(*ref_cnt);
        }

        return *this;
    }

    // Перемещающий конструктор от подтипа: SharedPtr<Base> b = std::move(d);
    template <typename U>
        requires std::derived_from<U, T>
    SharedPtr(SharedPtr<U>&& other) noexcept
        : ptr(other.ptr), ref_cnt(other.ref_cnt) {
        other.ptr = nullptr;
        other.ref_cnt = nullptr;
    }

    // Перемещающий оператор присваивания от подтипа: b = std::move(d);
    template <typename U>
        requires std::derived_from<U, T>
    SharedPtr& operator=(SharedPtr<U>&& other) noexcept {
        ReleaseInternal();

        ptr = other.ptr;
        ref_cnt = other.ref_cnt;

        other.ptr = nullptr;
        other.ref_cnt = nullptr;
        return *this;
    }

    void Reset(T* p = nullptr) {
        if (ptr == p) {
            return;
        }

        ReleaseInternal();

        ptr = p;
        ref_cnt = p ? new int(1) : nullptr;
    }

    T& operator*() const noexcept { return *ptr; }
    T* operator->() const noexcept { return ptr; }
    T* Get() const noexcept { return ptr; }

    int UseCnt() const noexcept {
        return ref_cnt ? *ref_cnt : 0;
    }

    explicit operator bool() const noexcept {
        return ptr != nullptr;
    }
};

template <typename T>
class SharedPtr<T[]> {
private:
    T* ptr;
    int* ref_cnt;

    void ReleaseInternal() noexcept {
        if (ref_cnt) {
            --(*ref_cnt);
            if (*ref_cnt == 0) {
                delete[] ptr;
                delete ref_cnt;
            }
            ptr = nullptr;
            ref_cnt = nullptr;
        }
    }

public:
    explicit SharedPtr(T* p = nullptr)
        : ptr(p), ref_cnt(p ? new int(1) : nullptr) {}

    ~SharedPtr() {
        ReleaseInternal();
    }

    SharedPtr(const SharedPtr& other) noexcept
        : ptr(other.ptr), ref_cnt(other.ref_cnt) {
        if (ref_cnt) {
            ++(*ref_cnt);
        }
    }

    SharedPtr& operator=(const SharedPtr& other) noexcept {
        if (this == &other || ref_cnt == other.ref_cnt) {
            return *this;
        }

        ReleaseInternal();

        ptr = other.ptr;
        ref_cnt = other.ref_cnt;

        if (ref_cnt) {
            ++(*ref_cnt);
        }

        return *this;
    }

    SharedPtr(SharedPtr&& other) noexcept
        : ptr(other.ptr), ref_cnt(other.ref_cnt) {
        other.ptr = nullptr;
        other.ref_cnt = nullptr;
    }

    SharedPtr& operator=(SharedPtr&& other) noexcept {
        if (this != &other) {
            ReleaseInternal();

            ptr = other.ptr;
            ref_cnt = other.ref_cnt;

            other.ptr = nullptr;
            other.ref_cnt = nullptr;
        }
        return *this;
    }

    void Reset(T* p = nullptr) {
        if (ptr == p) {
            return;
        }

        ReleaseInternal();

        ptr = p;
        ref_cnt = p ? new int(1) : nullptr;
    }

    T& operator[](int index) const {
        return ptr[index];
    }

    T* Get() const noexcept { return ptr; }

    int UseCnt() const noexcept {
        return ref_cnt ? *ref_cnt : 0;
    }

    explicit operator bool() const noexcept {
        return ptr != nullptr;
    }
};
