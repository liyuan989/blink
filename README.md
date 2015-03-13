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

**It must be built by cmake >= 2.6 at least**

**To compile release library into /lib/release, release examples and test into bin/release:**

```
./build.sh
BUILD_TYPE=release ./build.sh
```

**To compile library into /lib/debug(release), examples and test into bin/debug(release):**

```
BUILD_TYPE=debug ./build.sh
```

**To install libraries and headers into the specified directory, such as:**

```
INSTALL_DIR=/usr/local ./build.sh install
BUILD_TYPE=debug INSTALL_DIR=/usr ./build.sh install
```

Libraries and headers will be installed into `debug(release)-install` default, if you haven't specified any directory.

**You can also add BUILD_NO_EXAMPLES or BUILD_NO_TEST in head, if you don't wanna build exmaples or test. Such as:**

```
BUILD_TYPE=debug BUILD_NO_EXAMPLES=true ./build.sh
BUILD_NO_TEST=true ./build.sh
BUILD_TYPE=debug BUILD_NO_EXAMPLES=true BUILD_NO_TEST=true ./build.sh
```
