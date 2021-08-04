# Description
Easy SHM library for C++, still a work in progress but it's 
a easier way to code up an anonymous SHM mapping, protect
the last page and generate a key to access it....I've used
similar code quite a bit and was tired of re-coding it.  If
I find time I'll make it a bit more advanced, like adding
a monitor process to cleanup if for some reason the code
crashes (you know, things happen right?).  I've tested the
code on OS X and Linux, but as usual your mileage may vary
greatly.

# Build Status
[![Build Status](https://travis-ci.org/RaftLib/shm.svg?branch=master)](https://travis-ci.org/RaftLib/shm)

# Important Notes
So I've checked the functions found in the testsuite.  I've
used the code quite a bit in various forms so it should work
rather well...however if you find bugs please submit a pull
request and I'll get it fixed ASAP.

# Compilation Notes
To build this library on OS X you'll need to run:
```bash
mkdir build
cd build
cmake ../build -G<build system of your choice> -DCMAKE_BUILD_TYPE=Release
[make | ninja]
[make | ninja] test
```

# Install
* Just run ```[make | ninja] install```


# Usage
To use this library, simply:
```cpp
#include <shm>
```

When compiling your code that uses it, link with -lshm.  On 
Linux you'll have to compile with the -lrt, -lpthread and 
-lnuma in order to fully utilize all the features.  In 
the future I might add a flag to compile without the NUMA
features so that you can use the library without having 
to install libnuma.
