// A unique_ptr impl with custom deleter

// The implementation assumes that a deleter is a callable with a
// data pointer as a parameter.

// Here we use Empty Base Optimization (EBO) :
// An empty class is one that contain no nonstatic data members, either directly or indirectly
// (i.e., inherited from a base class).
// An object of empty class will have non-zero size (no virtual -> size 1 but
// virtual will introduce vptr and increase size)
// But when you inherit from such a class, instead of making a subobject of empty base class (like normal cases)
// no bytes are alloted for that subobject
// This saves memory (1 or even more bytes per object saved due to padding reasons)

// We specialize our template on the boolean that if passed deleter is empty then
// we can avoid storing it altogether if we can inherit from it (we already get the () if we inherit)
// For that the passed deleter must NOT be final, these two conditions are checked by boolean
// and if true we have a partial template specialization for it.

// The DefaultDeleter is an empty non-final class so it is benefitted from this.

// C++20 onwards, we can use [[no_unique_address]] attribute to avoid storing empty member subobject
// This allows us to avoid the inheritance and code duplication while still saving memory
// This is shown in comments below.

#pragma once

#include <utility>
namespace my_uniq
{
    // pre-C++20:
    template <typename T, typename Del, bool hasEmptyBase = std::is_empty_v<Del> && !std::is_final_v<Del>>
    struct compressed_pair
    {
        T *ptr{};
        Del deleter{};
        compressed_pair() = default;
        compressed_pair(T *p, Del d) : ptr{p}, deleter{std::move(d)} {}
        T *first() { return ptr; }
        Del &second() { return deleter; }
    };

    template <typename T, typename Del>
    struct compressed_pair<T, Del, true> : public Del
    {
        T *ptr{};
        compressed_pair() = default;
        compressed_pair(T *p) : ptr{p} {}
        T *first() { return ptr; }
        Del &second() { return *this; }
    };

    // C++20 onwards:
    // template <typename T, typename Del>
    // struct compressed_pair
    // {
    //     T *ptr{};
    //     [[no_unique_address]] Del deleter{};
    //     compressed_pair() = default;
    //     compressed_pair(T *p, Del d = Del{}) : ptr{p}, deleter{std::move(d)} {}
    //     T *first() { return ptr; }
    //     Del &second() { return deleter; }
    // };

    template <typename T>
    struct DefaultDeleter
    {
        void operator()(T *ptr)
        {
            delete ptr;
        }
    };

    template <typename T, typename Deleter = DefaultDeleter<T>>
    class uniq_ptr
    {
    private:
        compressed_pair<T, Deleter> uptr{};

    public:
        uniq_ptr() = default;
        explicit uniq_ptr(T *ptr) : uptr{ptr} {}
        uniq_ptr(T *ptr, Deleter del) : uptr{ptr, std::move(del)} {}
        uniq_ptr(const uniq_ptr &) = delete;
        uniq_ptr &operator=(const uniq_ptr &) = delete;
        uniq_ptr(uniq_ptr &&other) noexcept : uptr{other.uptr.first(), std::move(other.uptr.second())}
        {
            other.uptr.first() = nullptr;
        }
        uniq_ptr &operator=(uniq_ptr &&other) noexcept
        {
            if (this != &other)
            {
                if (uptr.first())
                    uptr.second()(uptr.first());
                uptr.first() = other.uptr.first();
                uptr.second() = std::move(other.uptr.second());
                other.uptr.first() = nullptr;
            }
            return *this;
        }
        ~uniq_ptr()
        {
            if (uptr.first())
                uptr.second()(uptr.first());
        }
        T *operator->() { return uptr.first(); }
        T &operator*() { return *(uptr.first()); }
    };
}
