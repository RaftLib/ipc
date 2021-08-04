#include <utility>
#include <cassert>
#include <cstring>
#include "traceclientexception.hpp"

ipc::traceexception::traceexception( const std::string message ) : 
    message( 
#if (defined _WIN64 ) || (defined _WIN32) 
    /**
     * fix for warning C4996: 'strdup': The POSIX name for this item is
     * a bit annoying given it is a POSIX function, but this is the best
     * we can do, see: https://bit.ly/2TH0Jci
     */
    _strdup( message.c_str() )
#else //everybody else
    strdup( message.c_str() ) 
#endif
    )
{
}

const char*
ipc::traceexception::what() const noexcept
{
    assert( message != nullptr );
    return( message );
}
