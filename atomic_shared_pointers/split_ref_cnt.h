#pragma once

#include <atomic>

namespace asp
{
    template <typename T>
    struct ctrl_blk
    {
        ctrl_blk() = default;
        ctrl_blk(T *p) : ptr{p} {}
        void add_ref_cnt(int64_t x)
        {
            ref_cnt.fetch_add(x);
        }
        void sub_ref_cnt(int64_t x)
        {
            if (ref_cnt.fetch_sub(x) == x)
            {
                delete this;
            }
        }
        std::atomic<int64_t> ref_cnt{1};
        T *ptr{};
    };

    template <typename T>
    class shd_ptr
    {

    public:
        ctrl_blk<T> *cb{};
        shd_ptr() = default;
        shd_ptr(T *p) : cb{new ctrl_blk<T>{p}} {}
        shd_ptr(ctrl_blk<T> *cptr) : cb{cptr} {}
        ~shd_ptr()
        {
            if (cb)
                cb->sub_ref_cnt(1);
        }
    };

    template <typename T>
    class atomic_sp
    {
        struct counted_ptr // for 16 byte atomics
        {
            ctrl_blk<T> *cb{};
            int64_t local_ref_cnt{0};
            counted_ptr(ctrl_blk<T> *p) : cb{p} {}
            counted_ptr() = default;
        };
        // or use packed pointer struct for other solution
        std::atomic<counted_ptr> ccb{};

        counted_ptr incr_local_ref_cnt()
        {
            counted_ptr old_ccb = ccb.load();
            counted_ptr new_ccb;
            do
            {
                new_ccb = old_ccb;
                new_ccb.local_ref_cnt++;
            } while (!ccb.compare_exchange_weak(old_ccb, new_ccb));
            return new_ccb;
        }

        void decr_local_ref_cnt(counted_ptr prev_ccb)
        {
            counted_ptr old_ccb = ccb.load();
            counted_ptr new_ccb;
            do
            {
                new_ccb = old_ccb;
                new_ccb.local_ref_cnt--;
            } while (prev_ccb.cb == old_ccb.cb && !ccb.compare_exchange_weak(old_ccb, new_ccb));
            if (prev_ccb.cb != old_ccb.cb)
            {
                // if ctrl block ptr moved => store ran and moved my local_ref_cnt to global_ref_cnt
                // thus go ahead and remove global ref_cnt
                prev_ccb.cb->sub_ref_cnt(1);
            }
        }

    public:
        atomic_sp() = default;
        atomic_sp<T>(T *p) : ccb{new ctrl_blk<T>{p}} {}
        shd_ptr<T> load()
        {
            // read the control block and simultaneously increment local ref_cnt to secure it
            auto new_ccb = incr_local_ref_cnt();
            // since control block is securely there, increment global ref_cnt
            new_ccb.cb->add_ref_cnt(1);
            // generate result
            auto result = shd_ptr<T>(new_ccb.cb);
            // decrement local ref_cnt since load complete
            decr_local_ref_cnt(ccb.load());
            return result;
        }

        void store(shd_ptr<T> desired)
        {
            counted_ptr new_ccb{desired.cb, 0};
            // my ptr will now point to supplied ctrl block and 0 local ref count
            auto old_ccb = ccb.exchange(new_ccb);
            // local_ref_cnt is moved to global_ref_cnt
            // => old ctrl block not deleted if any in-flight loads
            old_ccb.cb->add_ref_cnt(old_ccb.local_ref_cnt);
            // my reference to old ctrl block is over
            old_ccb.cb->sub_ref_cnt(1);
            desired.cb = nullptr;
        }
    };
}