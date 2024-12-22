# Lock-free Concurrent Data Structures

Designing lock-free data structures is a difficult task, and it’s easy to make mistakes,
but such data structures have scalability properties that are important in some situa-
tions.

## Some guidelines while writing lock-free data structures

1. Use std::memory_order_seq_cst for prototyping, try relaxing memory order only in the end
2. Never forget about memory reclamation and ABA problem.
3. Identify busy-wait loops and collaborate with other threads

## Treiber Stack

The simplest stack algorithm : 

Pushing an item to the stack is done by first taking the top of the stack (old head) and placing it after your new item to create a new head. Then using a CAS loop to correctly replace the old head with the new one.

For popping an item, a CAS loop gives you the current head node and places the head pointer to the next node. The node can then be returned.

Where the algorithm gets tricky is safe memory reclamation for which I redirect you to `atomic_shared_pointers/notes.md`.

Atomic shared pointers (once ensured to be lock-free) make the implementation highly comprehensible since they encapsulate memory reclamation. 

Check out the two implementations in treiber_stack folder. Just replace the std::atomic< shared_ptr > used there with any lock-free implementation and you're done.

## Michael-Scott Queue

Non-blocking concurrent queue as mentioned in paper saved in ms_queue folder. Unbounded queue which can handle multiple simultaneous enqueue() and dequeue().

Check out implementation in ms_queue/lock_free_with_asp.h. Implementated with atomic shared pointers for automatic memory reclamation, replace them with any lock-free implementation.

**Working in brief :**

`head`, `tail` along with `next` pointer of all nodes are atomic since they may be modified by multiple threads concurrently.

The queue is initialised with both `head` and `tail` pointing to a dummy node => ensures neither `head` nor `tail` is ever NULL.

The first element will continue to act as a dummy node. Any nodes enqueued come after last node in the list. On dequeuing, the first (dummy) is marked to free, value in second node (next to dummy) is returned and thus the previously second node now acts as the first and the dummy node of the list. 

`head` always points to first node in list. `tail` points to some node in list (but definitely not before `head`).

## Lock-free ring buffer

Next target.

## Concurrent hashmap

A rabbit-hole for some other day.
