#ifndef BUFFER_DEBUG
#define BUFFER_DEBUG 1


#include <atomic>

namespace ipc
{

struct buffer_debug
{
    std::atomic<int> critical_count = 0;
    int spin_lock                   = 0;
};

} /** end namespace ipc **/


#endif
