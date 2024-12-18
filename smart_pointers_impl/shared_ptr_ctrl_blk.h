#pragma once
#include <atomic>

namespace roopam
{
    struct ctrl_blk_base
    {
        std::atomic<int64_t> ref_cnt{1};
        void acquire_shared()
        {
            ref_cnt.fetch_add(1, std::memory_order_relaxed);
        }
        auto decrement()
        {
            return ref_cnt.fetch_sub(1, std::memory_order_acq_rel);
        }
        virtual void release_shared() = 0;
        virtual ~ctrl_blk_base() = default;
    };

    // Used when creating shared_ptr from raw pointer
    // Control block having pointer to the heap allocated object elsewhere
    // Need to delete both object and ctrl block when ref_cnt becomes zero
    template <typename T>
    struct ctrl_blk : ctrl_blk_base
    {
        T *data;
        explicit ctrl_blk(T *p) : ctrl_blk_base{}, data(p) {}
        void release_shared() override
        {
            if (decrement() == 1)
            {
                delete data;
                delete this;
            }
        }
    };

    // Used when shared_ptr created from make_shared
    // Standard optimization done by most libraries :
    // Allocate the controlled object with the ctrl block in the same heap allocation
    // One heap allocation instead of two
    // Thus make_shared is generally more optimized than shared_ptr from new
    // No need of pointer inside ctrl block
    // Just destroy this heap object when ref_cnt becomes zero
    template <typename T>
    struct ctrl_blk_with_storage : ctrl_blk_base
    {
        T in_place{};

        // Perfect forwarding of args to ctor of T
        template <typename... Args>
        explicit ctrl_blk_with_storage(Args &&...args) : ctrl_blk_base{}, in_place{std::forward<Args>(args)...} {}

        T *get()
        {
            return &in_place;
        }
        void release_shared() override
        {
            if (decrement() == 1)
            {
                delete this;
            }
        }
    };

    template <typename T>
    class shd_ptr
    {
        T *data{};
        ctrl_blk_base *control_block{};

        shd_ptr(T *p, ctrl_blk_base *cb) : data{p}, control_block{cb} {}

        shd_ptr(ctrl_blk_with_storage<T> *cb) : shd_ptr{cb->get(), cb} {}

    public:
        shd_ptr() = default;
        shd_ptr(T *p) : shd_ptr{p, new ctrl_blk<T>{p}} {}
        shd_ptr(const shd_ptr &other) : data{other.data}, control_block{other.control_block}
        {
            if (control_block)
                control_block->acquire_shared();
        }
        shd_ptr &operator=(const shd_ptr &other)
        {
            if (this != &other)
            {
                if (control_block)
                    control_block->release_shared();
                data = other.data;
                control_block = other.control_block;
                if (control_block)
                    control_block->acquire_shared();
            }
            return *this;
        }
        shd_ptr(shd_ptr &&other) noexcept : data{std::move(other.data)}, control_block{std::move(other.control_block)}
        {
            other.data = nullptr;
            other.control_block = nullptr;
        }
        shd_ptr &operator=(shd_ptr &&other) noexcept
        {
            if (this != &other)
            {
                if (control_block)
                    control_block->release_shared();
                data = std::move(other.data);
                control_block = std::move(other.control_block);
                other.data = nullptr;
                other.control_block = nullptr;
            }
            return *this;
        }
        ~shd_ptr()
        {
            if (control_block)
                control_block->release_shared();
        }
        T *operator->()
        {
            return data;
        }

        template <typename U, typename... Args>
        friend shd_ptr<U> make_shared(Args &&...args);
    };

    template <typename T, typename... Args>
    shd_ptr<T> make_shared(Args &&...args)
    {
        auto *cb = new ctrl_blk_with_storage<T>{std::forward<Args>(args)...};
        return shd_ptr<T>(cb);
    }
}