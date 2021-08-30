#include <cstdint>
#include <cstdlib>
#include <buffer>
#include <iostream>
#include <iomanip>


int main()
{
    ipc::buffer b;
    /**
     * all translate requests should be with reference to the
     * data buffer, so b->data. 
     */
    const auto ptr = ipc::buffer::translate( (void*) &b.data, 32 );

    if( ptr == nullptr )
    {
        std::cerr << "pointer should not be null with this test case\n";
        exit( EXIT_FAILURE );
    }
    const auto ptr_val          = (std::uintptr_t)(ptr );
    const auto expected_ptr_val = (std::uintptr_t)(&b.data) + 32;

    if( ptr_val == expected_ptr_val )
    {
        std::cout << "success\n";
        return( EXIT_SUCCESS );
    }
    //debug helpers
    std::cout << std::hex << ptr_val << " - " << expected_ptr_val << "\n";
    std::cout << std::dec<< ptr_val << " - " << expected_ptr_val << "\n";
    return( EXIT_FAILURE );
}
