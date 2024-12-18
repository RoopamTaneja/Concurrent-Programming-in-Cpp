# Lock-Free Data Structures and Atomic std::shared_ptr

## Thread-safe reference-counting in C++

Well, reference-counting in control block of shared_ptr is guaranteed to be thread-safe.

Since std. library already provides us the RAII compliant shared_ptr it makes sense to use its machinery for our purpose.

Although remember that for std::shared_ptr, updating the managed object from different threads is not thread safe. Also updating the same shared_ptr object (changing what it points to) from different threads is not thread safe.

This leads us to atomic shared pointers.

## Atomic Shared Ptr in C++ STL

std::atomic<std::shared_ptr < T >> was introduced in C++20. 

Prior to this, atomic free functions were to be used for atomically accessing shared pointers. They are not lock-free for most data structures not part of std::atomic already. That can be checked by std::atomic_is_lock_free anyway (turns out to be false). Look at an example in *stl_stack_before_cpp20.h* for how conveniently shared_ptr API allows implementing lock-free stack *only if it's lock-free*. (Data can be stored as shared ptrs to provide exception safety while returning in pop as shown in this example.)

But std::atomic<std::shared_ptr < T >> brought the hope of making them lock-free. Look at example in *stl_lock_free_stack_cpp20.h*.  

But any of its current implementations in compilers is **not lock-free** (false on .is_lock_free()).

Why insist on lock-free atomic shared pointers? What's the deal?

Thread-safe concurrenct access and memory reclamation is necessary for lock-free data structures. Memory reclamation along with ABA problem is especially tricky to handle. Lock-free atomic shared pointers will encapsulate memory reclamation (RAII) while exposing a simple API.

So no one thought about this earlier?

Many have implemented lock-free atomic shared pointers. We will try understanding techniques used by them and gain knowledge on aspects of lock-free programming.

## Memory reclamation in lock-free data structures

Once you are done with a node (say popped it from stack) you need to free its heap memory. What if some other node was still accessing it at the moment you free it, it will be left with a dangling reference.

Easy way out : Never free memory of used nodes - No problem if in a garbage collected language but this solution won't work for C++.

How you reclaim memory is crux of the solution. Two common methods are : reference counting (and split reference counting in particular) and deferred reclamation (using hazard pointers, rcu etc)

## ABA Problem

https://lumian2015.github.io/lockFreeProgramming/aba-problem.html

Clearly, one way to avoid this problem is by not freeing memory in use by some other thread (deferred reclamation).

Also, tag field alongside gives a peek into the problem of storing a counter and manipulating it atomically.

## Split Reference Counting

Reference counting tackles the problem by storing a count of the number of threads accessing each node. Thread-safe reference counting and automatic subsequent reclamation provided by shared pointers was what prompted us to implement our lock-free structures with atomic shared pointers in the first place. And what stopped us was the STL implementation not being lock-free.

So we can either choose to implement manual lock-free reference counting solely for our data structure or implement it as part of a new atomic shared_ptr once which can be then used in multiple concurrent structures. Of course we go with the latter.

Lock-free reference counting turns out to be not so easy (that's why it is not in the STL yet) and involves certain tradeoffs. Let's understand :

**Problem** : We wish to update the data and the counter together but no modern CPU supports an atomic instruction to modify two different memory locations simultaneously. This problem will also occur with storing tag in above solution of ABA.

**Solution 1.** : Store the data and counter adjacent to each other and use Double-Word Compare-and-Swap (DWCAS) instructions available on most modern CPUs. Such instructions can update a double word lock-free.

Tradeoffs : 
    - Not all compilers have an std::atomic implementation which produces DWCAS instructions. (It would instead use locks in the situation). This hinders portability. One solution is to use boost::atomic.
    - DWCAS since operate on double the word, take double the time.

**Solution 2.** : Use some bits of the data pointer for counting. If you have 64 bit pointers but virtual addresses are only 48 bits so upper 16 bits are free to be used. AKA Pointer packing. 

## Hazard Pointers

Like a global bulletin board where each thread has a marker which it uses to mark a node it is going to access and doesn't want to get deleted.

When a thread wishes to delete an object, it must first check the hazard pointers
belonging to the other threads in the system. If none of the hazard pointers reference
the object, it can safely be deleted. Otherwise, it must be left until later. Periodically,
the list of objects that have been left until later is checked to see if any of them can
now be deleted.

Multiple hazard pointer implementations exist. They can be implemented as a global array, linked list etc. They may also become part of C++26.

A look at API of Folly's hazard pointer implementation : 

- hazptr_holder: Class that owns and manages a hazard pointer. Raw hazard pointers are not exposed to users. Instead, each instance of the class hazptr_holder owns and manages at most one hazard pointer.
- protect: Member function of hazptr_holder. Protects an object pointed to by an atomic source (if not null). Safe to access object as long as protected. T* protect(const atomic< T*>& src);
- hazptr_obj_base< T>: Base class for protected objects. Class of objects T will typically derive from hazptr_obj_base< T> (CRTP).
- retire: Member function of hazptr_obj_base that automatically reclaims the object when safe. void retire();