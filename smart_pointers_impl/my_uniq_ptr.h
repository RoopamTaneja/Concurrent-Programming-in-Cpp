// A trimmed-down unique_ptr impl
// Does NOT support T[]

#pragma once

#include <utility>
namespace my_uniq
{
    template <typename T>
    class my_uniq_ptr
    {
    public:
        using pointer = T *;
        using element_type = T;

        my_uniq_ptr() = default;
        explicit my_uniq_ptr(pointer ptr) : ptr_{ptr} {}
        my_uniq_ptr(const my_uniq_ptr &) = delete;
        my_uniq_ptr &operator=(const my_uniq_ptr &) = delete;
        my_uniq_ptr(my_uniq_ptr &&other) noexcept : ptr_{std::exchange(other.ptr_, nullptr)} {}
        my_uniq_ptr &operator=(my_uniq_ptr &&other) noexcept
        {
            if (this != &other)
            {
                delete ptr_;
                ptr_ = std::exchange(other.ptr_, nullptr);
            }
            return *this;
        }
        ~my_uniq_ptr()
        {
            delete ptr_;
        };

        pointer get() const
        {
            return ptr_;
        }
        void reset(pointer ptr = nullptr)
        {
            delete ptr_;
            ptr_ = ptr;
        }
        element_type &operator*() const
        {
            return *ptr_;
        };
        pointer operator->() const
        {
            return ptr_;
        }

    private:
        pointer ptr_{nullptr};
    };
}