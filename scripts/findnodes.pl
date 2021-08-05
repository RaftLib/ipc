#!/usr/bin/env perl
use strict;
use warnings;

my $os = `uname`;
chomp( $os );
$os = lc( $os );
if( $os eq "linux" )
{
    my $cmdstr =  "grep -oP \"(?<=Node\\s)\\d\" /proc/zoneinfo";
    my $nodelist = `$cmdstr`;
    chomp( $nodelist );
    my @arr = split/\n/,$nodelist;
    my $first = shift( @arr );
    foreach my $val ( @arr )
    {
        if( $val != $first )
        {
            print( "1" );
            exit( 0 );
        }
    }
}
##
# else 
##
print( "0" );
exit( 0 );
