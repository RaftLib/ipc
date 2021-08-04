#include <string>
#include "allocationexception.hpp"



ipc::allocationexception::allocationexception(  const std::string message ) : 
    ipc::traceexception( message )
{

}
