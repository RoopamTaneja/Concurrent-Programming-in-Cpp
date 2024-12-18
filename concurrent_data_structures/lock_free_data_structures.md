# Lock-free Concurrent Data Structures

Because there aren’t any locks, deadlocks are impossible with lock-free data structures, although there is the possibility of live locks instead.  By  definition,  wait-free
code can’t suffer from live lock because there’s always an upper limit on the number
of  steps  needed  to  perform  an  operation.  

Although none of these examples use mutex locks directly, it’s worth bearing
in mind that only std::atomic_flag is guaranteed not to use locks in the implemen-
tation. On some platforms what appears to be lock-free code might actually be using
locks  internal  to  the  C++  Standard  Library  implementation.
