#include <unistd.h>
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <sys/errno.h>
#include "sem.hpp"


int main( int argc, char **argv )
{
    //we're the parent
    auto key   = ipc::sem::generate_key( 32 );
    
    auto semid = ipc::sem::open( key, ipc::sem::sem_create, ipc::sem::file_rdwr ); 
    
    if( semid == ipc::sem::sem_error )
    {
        std::perror( "sem open in parent returned failure." );
        exit( EXIT_FAILURE );
    }


    if( ipc::sem::main_init( semid ) == -1 )
    {
        std::perror( "Failed to initialize semaphore, exiting!!\n" );
        exit( EXIT_FAILURE );
    }
    
    int count = atoi( argv[ 1 ] );
    bool parent = false;
    std::vector< pid_t > children;
    for( auto i( 0 ); i < count; i++ )
    {
        auto child = fork();
        switch( child )
        {
            case( 0 /** child **/ ):
            {   
                //child
                auto semid = ipc::sem::open( key,  
                                             0x0 /** flags **/,
                                             ipc::sem::file_rdwr );
                if( semid == ipc::sem::sem_error )
                {
                    std::perror ("child failed to get the semaphore, exiting." );
                    exit( EXIT_FAILURE );

                }
                //else we wait for it to be initialized
                if( ipc::sem::sub_init( semid ) == -1 )
                {
                    std::perror( "failed to initialize semaphore in child" );
                    exit( EXIT_FAILURE );
                }
                //now let's use the semaphore
                int iterations = 10;
                while( --iterations )
                {
                    ipc::sem::wait( semid );
                    //should be the same value for thie kid
                    std::cout << "child: " << i << "\n";
                    ipc::sem::post( semid  );
                    sleep( 1 );
                }
                exit( EXIT_SUCCESS );
            }
            break;
            case( -1 /** error, back to parent **/ ):
            {
                exit( EXIT_FAILURE );
            }
            break;
            default:
            {
                children.push_back( child );
                parent = true;
            }
        }
    }
        
    if( parent )
    {
        int iterations = 10;
        while( --iterations )
        {
            ipc::sem::wait( semid );
            std::cout << "parent\n";
            ipc::sem::post( semid );
            sleep( 1 );
        }


        pid_t   wpid    = 0;
        int     status  = 0;
        for( auto p : children )
        {
            std::cout << p << "\n";
        }
        while( (wpid = wait( &status )) > 0 );
        //parent removes sem
        ipc::sem::main_close( key ); 
        ipc::sem::free_key( key );
    }
    return( EXIT_SUCCESS );
}
