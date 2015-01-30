# blink
-------
* A C++ network library imitate muduo.
* Add some features for personal preference.

#Evironment
-----------

* **Linux only**.
    * It runs on Linux with kernel 2.6.28 at least for epoll, new syscall, new flag.
    * eg. eventfd, timerfd, O_CLOEXEC.

* **It also need boost library**.

#Compiler
---------

* **g++ only**.

# Build
-------

**It must be built by cmake >= 2.6 at lease**

**To compile release library into /lib/release, release examples and test into bin/release:**

    % ./build.sh 
    
    or
    
    % BUILD_TYPE=release ./build.sh

**To compile debug library into /lib/debug, debug examples and test into bin/debug:**

    % BUILD_TYPE=debug ./build.sh 

**You can also add BUILD_NO_EXAMPLES or BUILD_NO_TEST in head, if you don't wanna build exmaples and test.**

**Such as:**

    % BUILD_TYPE=debug BUILD_NO_EXAMPLES=true ./build.sh
    % BUILD_NO_TEST=true ./build.sh
    % BUILD_TYPE=debug BUILD_NO_EXAMPLES=true BUILD_NO_TEST=true ./build.sh
