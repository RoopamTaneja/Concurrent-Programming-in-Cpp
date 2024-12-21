# Lock-free Concurrent Data Structures

Because there aren’t any locks, deadlocks are impossible with lock-free data structures, although there is the possibility of live locks instead.  By  definition,  wait-free
code can’t suffer from live lock because there’s always an upper limit on the number
of  steps  needed  to  perform  an  operation.  