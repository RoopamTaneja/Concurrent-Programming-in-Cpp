#pragma once

#include <atomic>
#include <memory>
#include <utility>
#include <folly/synchronization/Hazptr.h>

namespace asp
{
    template <typename T>
    struct basic_control_block : public folly::hazptr_obj_base<basic_control_block<T>>
    {
        std::atomic<int64_t> ref_count{1};
        T *ptr{};

        basic_control_block() = default;
        basic_control_block(T *ptr_) : ptr{ptr_} {}
        basic_control_block(const basic_control_block &) = delete;
        basic_control_block &operator=(const basic_control_block &) = delete;
        ~basic_control_block() = default;

        // Increment the reference count.  The reference count must not be zero
        void increment_count() noexcept
        {
            ref_count.fetch_add(1);
        }

        // Increment the reference count if it is not zero.
        bool increment_if_nonzero() noexcept
        {
            auto cnt = ref_count.load();
            while (cnt > 0 && !ref_count.compare_exchange_weak(cnt, cnt + 1))
                ;
            return cnt > 0;
        }

        // Release a reference to the object.
        void decrement_count() noexcept
        {
            if (ref_count.fetch_sub(1) == 1)
            {
                delete ptr;
                this->retire();
            }
        }
    };

    template <typename T>
    class shared_ptr
    {
    public:
        basic_control_block<T> *control_block{nullptr};
        shared_ptr() = default;
        explicit shared_ptr(basic_control_block<T> *control_block_) : control_block(control_block_) {}
        explicit shared_ptr(T *p)
        {
            std::unique_ptr<T> up(p); // Hold inside a unique_ptr so that p is deleted if the allocation throws
            control_block = new basic_control_block<T>(p);
            up.release();
        }
        shared_ptr(const shared_ptr &other) noexcept : control_block(other.control_block)
        {
            if (control_block)
            {
                control_block->increment_count();
            }
        }
        shared_ptr &operator=(const shared_ptr &other) noexcept
        {
            shared_ptr(other).swap(*this);
            return *this;
        }
        shared_ptr(shared_ptr &&other) noexcept : control_block(std::exchange(other.control_block, nullptr)) {}
        shared_ptr &operator=(shared_ptr &&other) noexcept
        {
            shared_ptr(std::move(other)).swap(*this);
            return *this;
        }
        ~shared_ptr()
        {
            if (control_block)
            {
                control_block->decrement_count();
            }
        }
        void swap(shared_ptr &other) noexcept
        {
            std::swap(control_block, other.control_block);
        }
    };

    template <typename T>
    class atomic_shared_ptr
    {

    private:
        std::atomic<basic_control_block<T> *> control_block;

    public:
        atomic_shared_ptr() = default;
        atomic_shared_ptr(shared_ptr<T> desired)
        {
            control_block.store(std::exchange(desired.control_block, nullptr));
        }
        atomic_shared_ptr(const atomic_shared_ptr &) = delete;
        atomic_shared_ptr &operator=(const atomic_shared_ptr &) = delete;
        ~atomic_shared_ptr()
        {
            store(shared_ptr<T>{});
        }
        shared_ptr<T> load()
        {
            folly::hazptr_holder hp = folly::make_hazard_pointer();
            basic_control_block<T> *current_control_block = nullptr;
            do
            {
                current_control_block = hp.protect(control_block);
            } while (current_control_block != nullptr && !current_control_block->increment_if_nonzero());
            // done to avoid resurrecting zombie objects (obj deleted but ctrl block exists with 0 ref count)
            return shared_ptr<T>(current_control_block);
        }

        void store(shared_ptr<T> desired)
        {
            auto new_control_block = std::exchange(desired.control_block, nullptr);
            auto old_control_block = control_block.exchange(new_control_block);
            if (old_control_block)
            {
                old_control_block->decrement_count();
            }
        }
    };
}