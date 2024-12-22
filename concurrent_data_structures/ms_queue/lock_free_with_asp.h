#pragma once

#include <memory>
#include <atomic>
#include <optional>

namespace ms_queue
{
    template <typename T>
    class lf_queue
    {
        struct node
        {
            T data{};
            std::atomic<std::shared_ptr<node>> next{};
            node() = default;
            node(T t) : data{std::move(t)} {}
            node(T t, std::shared_ptr<node> ptr) : data{std::move(t)}, next{std::move(ptr)} {}
        };
        std::atomic<std::shared_ptr<node>> head{std::make_shared<node>()};
        std::atomic<std::shared_ptr<node>> tail{head.load()};

    public:
        lf_queue() = default;
        lf_queue(const lf_queue &) = delete;
        lf_queue &operator=(const lf_queue &) = delete;
        ~lf_queue() = default;
        void enqueue(T elem)
        {
            std::shared_ptr<node> p = std::make_shared<node>(std::move(elem));
            std::shared_ptr<node> old_tail;
            while (true)
            {
                old_tail = tail.load();
                auto old_next = old_tail->next.load();
                if (old_next) // tail was not pointing to last node
                {             // swing the tail to next node
                    tail.compare_exchange_weak(old_tail, old_next);
                    continue;
                }
                // old_next is nullptr
                if (old_tail->next.compare_exchange_strong(old_next, p))
                    break; // linked node to next of tail successfully
            }
            tail.compare_exchange_strong(old_tail, p); // swing tail to new node
        }

        std::optional<T> dequeue()
        {
            T result;
            while (true)
            {
                auto old_head = head.load();
                auto old_tail = tail.load();
                auto old_next = old_head->next.load();
                if (!old_next) // empty queue
                    return {};
                if (old_head == old_tail) // tail is falling behind
                {                         // try to advance tail : helps enqueue
                    tail.compare_exchange_strong(old_tail, old_next);
                    continue;
                }
                result = old_next->data; // read value before moving head, bcoz another dequeue might free old_next
                if (head.compare_exchange_strong(old_head, old_next))
                    break; // moved head to next node, dequeue successful
            }
            return result;
        }
    };
}