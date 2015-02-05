#include "Date.h"

#include <sys/time.h>

#include <time.h>
#include <stdio.h>

using namespace blink;

int main(int argc, char const *argv[])
{
    struct timeval tm;
 	gettimeofday(&tm, NULL);

 	time_t second = tm.tv_sec;
 	struct tm tm_time;
 	gmtime_r(&second, &tm_time);

 	Date date2(tm_time);
    string s =  date2.toString();
 	printf("%s\n", s.c_str());

 	Date date3(2014, 12, 21);
 	s = date3.toString();
 	printf("%s\n", s.c_str());
	return 0;
}
