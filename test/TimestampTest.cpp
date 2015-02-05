#include "Timestamp.h"

#include <sys/time.h>

#include <string>
#include <time.h>
#include <stdio.h>

using namespace blink;

int main(int argc, char const *argv[])
{
	Timestamp tm_stamp(Timestamp::now());
	string s = tm_stamp.toString();
	string s_fmt = tm_stamp.toFormattedString(true);
	printf("%s\n", s.c_str());
	printf("%s\n", s_fmt.c_str());
	struct timeval tv;
  	gettimeofday(&tv, 0);
  	printf("%ld\n", tv.tv_sec);
	return 0;
}
