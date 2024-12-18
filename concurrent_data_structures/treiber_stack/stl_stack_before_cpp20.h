#pragma once

#include <atomic>
#include <memory>

namespace lock_free
{
    template <typename T>
    class lf_stack
    {
    private:
        struct node
        {
            std::shared_ptr<T> data{};
            std::shared_ptr<node> next{};
            explicit node(T data_) : data{std::make_shared<T>(std::move(data_))} {}
        };

        std::shared_ptr<node> head{};

    public:
        void push(T data_)
        {
            std::shared_ptr<node> new_node = std::make_shared<node>(std::move(data_));
            new_node->next = std::atomic_load(&head);
            while (!atomic_compare_exchange_weak(&head, &new_node->next, new_node))
                ;
        }
        std::shared_ptr<T> pop()
        {
            std::shared_ptr<node> old_head = std::atomic_load(&head);
            while (old_head && !atomic_compare_exchange_weak(&head, &old_head, old_head->next))
                ;
            return old_head ? old_head->data : nullptr;
        }
    };
}