#pragma once

template <typename T>
class SharedPtr {
private:
    T* ptr;
    int* ref_cnt;

    void release_internal() noexcept {
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

public:
    explicit SharedPtr(T* p = nullptr)
        : ptr(p), ref_cnt(p ? new int(1) : nullptr) {}

    ~SharedPtr() {
        release_internal();
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

        release_internal();

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
            release_internal();

            ptr = other.ptr;
            ref_cnt = other.ref_cnt;

            other.ptr = nullptr;
            other.ref_cnt = nullptr;
        }
        return *this;
    }

    void reset(T* p = nullptr) {
        if (ptr == p) {
            return;
        }

        release_internal();

        ptr = p;
        ref_cnt = p ? new int(1) : nullptr;
    }

    T& operator*() const noexcept { return *ptr; }
    T* operator->() const noexcept { return ptr; }
    T* get() const noexcept { return ptr; }

    int use_cnt() const noexcept {
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

    void release_internal() noexcept {
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
        release_internal();
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

        release_internal();

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
            release_internal();

            ptr = other.ptr;
            ref_cnt = other.ref_cnt;

            other.ptr = nullptr;
            other.ref_cnt = nullptr;
        }
        return *this;
    }

    void reset(T* p = nullptr) {
        if (ptr == p) {
            return;
        }

        release_internal();

        ptr = p;
        ref_cnt = p ? new int(1) : nullptr;
    }

    T& operator[](int index) const {
        return ptr[index];
    }

    T* get() const noexcept { return ptr; }

    int use_cnt() const noexcept {
        return ref_cnt ? *ref_cnt : 0;
    }

    explicit operator bool() const noexcept {
        return ptr != nullptr;
    }
};
