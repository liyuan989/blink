# blink
-------
A C++ network library imitate muduo.

Add some features for personal preference.

#Evironment
-----------

Linux only.

It need Linux kernel 2.6.28 at lease for epoll, new syscall, new flag.

eg. eventfd, timerfd, O_CLOEXEC.

It also need boost library.

#Compiler
---------

g++ only.

# Build
-------

To compile debug library into /lib/debug:

    % make

To compile release library into /lib/release:

    % make release

To compile example (need debug library) into /bin:

    % make example

To compile all:

    % make all

To clean library:

    % make clean

To clean example:

    % make clean-example

To clean all:

    % make clean-all
