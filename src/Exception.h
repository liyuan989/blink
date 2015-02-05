#ifndef __BLINK_EXCEPTION_H__
#define __BLINK_EXCEPTION_H__

#include "Types.h"

#include <exception>

namespace blink
{

class Exception : public std::exception
{
public:
	explicit Exception(const string& message);
	explicit Exception(const char* message);
	virtual ~Exception() throw();

	virtual const char* what() const throw();
	const char* stackTrace() const throw();

private:
	void fillStackTrace();

	string  message_;
	string  stack_;
};

}  // namespace blink

#endif
