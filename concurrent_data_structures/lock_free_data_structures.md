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

**Next target**

## Concurrent hash table

**A small survey of some existing concurrent hash tables:**

### Intel TBB

They have a lock-based hash table based on separate chaining where a lock is held per bucket.

### libcuckoo

It is a concurrent hash table developed by CMU based on optimistic cuckoo hashing which allows concurrent reads without locking.

Below I present my summary of their original design from their paper : [NSDI 2013](https://www.cs.cmu.edu/~dga/papers/memc3-nsdi2013.pdf) 

They improved naive cuckoo hashing and made a 4-way set associative hash table, ie each bucket can store pointers to 4 (key, value) pairs to allow more occupancy before a rehash.

To reduce no of pointer dereferences, they use the metadata trick. It stores one byte hash of key as a metadata tag beside the pointer. Only those pointers with matching tag are dereferenced hence reducing the no of main memory accesses (on average it turns out to be a little over one access for a successful lookup). The 4-way buckets anyway fit into a single cache line. 

For concurrent access :

They simplify things by allowing only one writer at a time. We have a lock on insert() which serializes multiple writers. Thus it is a multi-reader, single-writer table with brilliant speedup for read-heavy workloads.

insert() is broken into a series of displacements. Instead of making whole sequence atomic (with a global lock), we can choose to make individual displacements atomic. During forward propagation of keys, one key is "floating" (not part of any bucket) between two displacements and a find() will return false. The paper has an ingenious solution for these false misses. It separates searching for cuckoo path of displacements from propagation. Once a hole is found, displacements are done in reverse ie hole is propagated backwards. 

This has two benefits : pulls searching for path out of critical section; and there are no floating keys so no issue of false misses.

find() interleaving with insert() can also be used with locks on two buckets involved in an atomic displacement. However paper chooses to use optimistic concurrency control in form of versioning where find() will check version numbers before and after reading to detect any concurrent displacement. Another optimization by paper is to use lock striping ie instead of a version counter with each (key, value) pair, a separate smaller array of counters is maintained to which different keys will hash to. The size should be small enough for cache locality and low overhead but large enough to minimize false positives due to collisions. The counters must be updated atomically.

Later they adapted the design for multiple readers/writers as well. Details : [EuroSys 2014](https://www.cs.princeton.edu/~mfreed/docs/cuckoo-eurosys14.pdf)

### parallel-hashmap

Developed by Gregory Popovitch. It builds from [Google's Abseil containers](https://abseil.io/docs/cpp/guides/container) hence uses open addressing along with SSE instructions. 

It modifies the aforementioned containers by internally maintaining an array of hashmaps (submaps) of size power of two. This allows lower peak memory usage and also supports concurrent access.

Concurrent access : 

Submaps support intrinsic parallelism and allow more fine-grained control than a lock on the whole table. The library chooses to have an internal mutex for each submap and benchmarks reveal a speedup compared to single-threaded insertion.

### Lock-free?

Planning to prototype a lock-free hashtable using atomic shared pointers based on :

- A.A and M.M's paper (saved in directory)
- https://ssteinberg.xyz/2015/09/28/designing-a-lock-free-wait-free-hash-map/

**Next target**