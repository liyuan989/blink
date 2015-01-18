#ifndef __BLINK_EXCEPTION_H__
#define __BLINK_EXCEPTION_H__

#include <exception>
#include <string>

namespace blink
{

class Exception : public std::exception
{
public:
	explicit Exception(std::string& message);
	explicit Exception(const char* message);
	virtual ~Exception() throw();

	virtual const char* what() const throw();
	const char* stackTrace() const throw();

private:
	void fillStackTrace();

	std::string message_;
	std::string stack_;
};

}  // namespace blink

#endif
