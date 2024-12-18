// A trimmed-down shared_ptr impl
// Does NOT support T[]

#pragma once
#include <utility>

namespace my_sh_ptr
{
    template <typename T>
    class my_shared_ptr
    {
    public:
        using pointer = T *;
        using element_type = T;

        my_shared_ptr() = default;
        explicit my_shared_ptr(pointer ptr) : ptr_{ptr}
        {
            if (ptr_)
                ref_cnt_ = new int{1};
        }
        my_shared_ptr(const my_shared_ptr &other) : ptr_{other.ptr_}, ref_cnt_{other.ref_cnt_}
        {
            if (ref_cnt_)
                (*ref_cnt_)++;
        }
        my_shared_ptr &operator=(const my_shared_ptr &other)
        {
            if (this != &other)
            {
                this->reset();
                ptr_ = other.ptr_;
                ref_cnt_ = other.ref_cnt_;
                if (ref_cnt_)
                    (*ref_cnt_)++;
            }
            return *this;
        }
        my_shared_ptr(my_shared_ptr &&other) noexcept : ptr_(std::exchange(other.ptr_, nullptr)), ref_cnt_(std::exchange(other.ref_cnt_, nullptr)) {}
        my_shared_ptr &operator=(my_shared_ptr &&other) noexcept
        {
            if (this != &other)
            {
                this->reset();
                ptr_ = std::exchange(other.ptr_, nullptr);
                ref_cnt_ = std::exchange(other.ref_cnt_, nullptr);
            }
            return *this;
        }
        ~my_shared_ptr()
        {
            this->reset();
        }

        void reset()
        {
            if (ptr_ && ref_cnt_)
            {
                (*ref_cnt_)--;
                if (*ref_cnt_ == 0)
                {
                    delete ptr_;
                    delete ref_cnt_;
                }
                ptr_ = nullptr;
                ref_cnt_ = nullptr;
            }
        }
        pointer get() const
        {
            return ptr_;
        }
        int use_count() const
        {
            if (!ref_cnt_)
                return 0;
            return *ref_cnt_;
        }
        element_type &operator*() const
        {
            return *ptr_;
        }
        pointer operator->() const
        {
            return ptr_;
        }

    private:
        pointer ptr_ = nullptr;
        int *ref_cnt_ = nullptr;
    };
}
