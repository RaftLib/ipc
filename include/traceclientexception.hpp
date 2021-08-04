#ifndef TRACECLIENTEXCEPTION_HPP
#define TRACECLIENTEXCEPTION_HPP  1
#include <exception>
#include <string>

namespace ipc
{

class traceexception : public std::exception
{
public:
   
    traceexception( const std::string message );
   
    virtual const char* what() const noexcept;
private:
    const char *message = nullptr;
};

} /** end namespace ipc **/

/** 
 * TO MAKE AN EXCEPTION 
 * 1) MAKE AN EMPTY DERIVED CLASSS OF traceexception
 * 2) MAKE ONE OF THESE TEMPLATES WHICH DERIVES FROM THAT
 *    EMPTY CLASS
 * THEN ADD A using MYNEWEXCEPTION = Template< # >
 */
#if 0
template < int N > class Templatetraceexception : public traceexception 
{
public:
    Templatetraceexception(  const std::string &message ) : traceexception( message ){};
};
#endif 

#endif /* END TRACECLIENTEXCEPTION_HPP */
