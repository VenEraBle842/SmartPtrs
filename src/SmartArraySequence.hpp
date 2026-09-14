#pragma once
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "UniquePtr.hpp"

template <typename T>
class Sequence {
public:
    virtual ~Sequence() = default;

    virtual int GetLength() const noexcept = 0;
    virtual bool IsEmpty() const noexcept = 0;

    virtual const T& Get(int index) const = 0;
    virtual T& Get(int index) = 0;

    virtual void Append(const T& item) = 0;
    virtual void Append(T&& item) = 0;

    virtual void Prepend(const T& item) = 0;
    virtual void Prepend(T&& item) = 0;

    virtual void InsertAt(const T& item, int index) = 0;
    virtual void InsertAt(T&& item, int index) = 0;

    virtual void RemoveAt(int index) = 0;

    virtual T& operator[](int index) = 0;
    virtual const T& operator[](int index) const = 0;
};

template <typename T>
class SmartArraySequence : public Sequence<T> {
private:
    UniquePtr<T[]> buffer;
    int size;
    int capacity;

    void CheckIndex(int index) const {
        if (index < 0 || index >= size) {
            throw std::out_of_range("Index out of range");
        }
    }

    void Reserve(int new_capacity) {
        if (new_capacity <= capacity) {
            return;
        }

        UniquePtr<T[]> new_buffer(new T[new_capacity]);

        for (int i = 0; i < size; ++i) {
            new_buffer[i] = std::move(buffer[i]);
        }

        buffer = std::move(new_buffer);
        capacity = new_capacity;
    }

    void PrepareInsert(int index) {
        if (index < 0 || index > size) {
            throw std::out_of_range("Index out of range");
        }

        if (size == capacity) {
            Reserve(capacity == 0 ? 4 : capacity * 2);
        }

        for (int i = size; i > index; --i) {
            buffer[i] = std::move(buffer[i - 1]);
        }
        ++size;
    }

public:
    SmartArraySequence() noexcept : buffer(nullptr), size(0), capacity(0) {}

    explicit SmartArraySequence(int init_capacity)
        : buffer(init_capacity > 0 ? new T[init_capacity] : nullptr),
          size(0),
          capacity(init_capacity > 0 ? init_capacity : 0) {}

    ~SmartArraySequence() override = default;

    SmartArraySequence(const SmartArraySequence& other)
        : buffer(0), size(0), capacity(0)
    {
        if constexpr (std::is_copy_assignable_v<T>) {
            if (other.capacity > 0) {
                buffer = UniquePtr<T[]>(new T[other.capacity]);
                capacity = other.capacity;
                size = other.size;
                for (int i = 0; i < size; ++i) {
                    buffer[i] = other.buffer[i];
                }
            }
        } else {
            throw std::runtime_error("Copying is not supported for move-only types");
        }
    }

    SmartArraySequence& operator=(const SmartArraySequence& other) {
        if constexpr (std::is_copy_assignable_v<T>) {
            if (this != &other) {
                SmartArraySequence tmp(other);
                *this = std::move(tmp);
            }
            return *this;
        } else {
            throw std::runtime_error("Copying is not supported for move-only types");
        }
    }

    SmartArraySequence(SmartArraySequence&& other) noexcept
        : buffer(std::move(other.buffer)),
          size(other.size),
          capacity(other.capacity)
    {
        other.size = 0;
        other.capacity = 0;
    }

    SmartArraySequence& operator=(SmartArraySequence&& other) noexcept {
        if (this != &other) {
            buffer = std::move(other.buffer);
            size = other.size;
            capacity = other.capacity;

            other.size = 0;
            other.capacity = 0;
        }
        return *this;
    }


    int GetLength() const noexcept override { return size; }
    bool IsEmpty() const noexcept override { return size == 0; }

    const T& Get(int index) const override {
        CheckIndex(index);
        return buffer[index];
    }
    T& Get(int index) override {
        CheckIndex(index);
        return buffer[index];
    }

    T& operator[](int index) override { return buffer[index]; }
    const T& operator[](int index) const override { return buffer[index]; }


    void Append(const T& item) override { InsertAt(item, size); }
    void Append(T&& item) override { InsertAt(std::move(item), size); }

    void Prepend(const T& item) override { InsertAt(item, 0); }
    void Prepend(T&& item) override { InsertAt(std::move(item), 0); }

    void InsertAt(const T& item, int index) override {
        if constexpr (std::is_copy_assignable_v<T>) {
            PrepareInsert(index);
            buffer[index] = item;
        } else {
            throw std::runtime_error("Copying is not supported for move-only types");
        }
    }
    void InsertAt(T&& item, int index) override {
        PrepareInsert(index);
        buffer[index] = std::move(item);
    }

    void RemoveAt(int index) override {
        CheckIndex(index);
        for (int i = index; i < size - 1; ++i) {
            buffer[i] = std::move(buffer[i + 1]);
        }
        buffer[size - 1] = T();
        --size;
    }

    void Clear() noexcept {
        for (int i = 0; i < size; ++i) {
            buffer[i] = T();
        }
        size = 0;
    }
};
