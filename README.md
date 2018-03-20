# afl-simulate
Simulate afl-fuzz

If you program your own instrumentation into binaries for AFL/afl-fuzz
you make small changes and have no idea if they make the performance
better or worse.

This tool is exactly for that.

It shows the runtime, the stability and buckets/fills and gives an average.

Just do:
  afl-simulate -i 100 instrumented_program -with -options

The -i 100 runs the AFL forkserver 100 times against the instrumented program.

You can use afl-gcc/afl-g++, afl-clang-fast/afl-clang++-fast, afl-dyninst or
your own like a Pin module to produce the instrumented binary

Example:
```
$ afl-simulate -i 10 pin -t obj-intel64/afl-pin.so -forkserver -- ./unrar-unmodified t test.rar > /dev/null
pin run=1 time=0.489706 result=0 buckets=995 fills=5392
pin run=2 time=0.494072 result=0 buckets=995 fills=5392
pin run=3 time=0.491258 result=0 buckets=995 fills=5392
pin run=4 time=0.487705 result=0 buckets=995 fills=5392
pin run=5 time=0.483401 result=0 buckets=995 fills=5392
pin run=6 time=0.503360 result=0 buckets=995 fills=5392
pin run=7 time=0.507124 result=0 buckets=995 fills=5392
pin run=8 time=0.507898 result=0 buckets=995 fills=5392
pin run=9 time=0.483421 result=0 buckets=995 fills=5392
pin run=10 time=0.487264 result=0 buckets=995 fills=5392
Error reading fork server	<= this message actually comes from the instrumented binary
End: client finished
Average=0.493521 min=0.483401 max=0.507898
```

Have fun!

Greets,
Marc "van Hauser" Heuse <vh(at)thc(dot)org> | <mh(at)mh-sec(dot)de>
