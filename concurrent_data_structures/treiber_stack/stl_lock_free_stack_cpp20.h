#pragma once

#include <atomic>
#include <memory>
#include <optional>

namespace lock_free
{
    template <typename T>
    struct Stack
    {
        struct Node
        {
            T t;
            std::shared_ptr<Node> next;
            Node(T elem, std::shared_ptr<Node> ptr) : t{std::move(elem)}, next{std::move(ptr)} {}
        };
        std::atomic<std::shared_ptr<Node>> head;

        void push(T t)
        {
            auto p = make_shared<Node>(std::move(t), head.load());
            while (!head.compare_exchange_weak(p->next, p))
                ;
        }

        std::optional<T> pop()
        {
            auto p = head.load();
            while (p != nullptr && !head.compare_exchange_weak(p, p->next))
                ;
            if (p != nullptr)
                return {std::move(p->t)};
            return {};
        }
    };
}
