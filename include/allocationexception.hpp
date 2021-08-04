#ifndef ALLOCATIONEXCEPTION_HPP
#define ALLOCATIONEXCEPTION_HPP  1
#include <string>

#include "traceclientexception.hpp"

namespace ipc
{

class allocationexception : public ipc::traceexception
{
public:
    allocationexception(  const std::string message ); 

};


template < int N > class allocationexceptionBase : public allocationexception 
{
public:
    allocationexceptionBase(  const std::string message ) : 
        allocationexception( message ){};
    
};


using tls_local_buffer_allocate_exception = allocationexceptionBase< 0 >;
using buffer_allocate_exception = allocationexceptionBase< 1 >;

} /** end traceclient namespace **/

#endif /* END ALLOCATIONEXCEPTION_HPP */
