#define _FILE_OFFSET_BITS 64 
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#define MAX_BUFFER 128
unsigned char buffer[MAX_BUFFER];

/**
 *  Very nice hex dump function.
 *
 *  @author David Poole
 *  @version 1.0.0
 *  @param ptr
 *  @param size
 *
 */

void hex_dump( const char *label, const unsigned char *ptr, int size )
{
    static char hex_ascii[] =
       { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
    int i;
    unsigned char line[80];
    unsigned char *ascii, *hex;
    const unsigned char *endptr;
    unsigned long offset=0;

   endptr = ptr + size;
   memset( line, ' ', 80 );
   line[69] = 0;
   while( ptr != endptr ) {
      hex = &line[2];
      ascii = &line[52];
      for( i=0 ; i<16 ; i++ ) {
         if( isprint(*ptr) )
            *ascii++ = *ptr;
         else
            *ascii++ = '.';
         *hex++ = hex_ascii[ *ptr>>4 ];
         *hex++ = hex_ascii[ *ptr&0x0f ];
         *hex++ = ' ';
         ptr++;
         if( ptr == endptr ) {
            /* clean out whatever is left from the last line */
            memset( hex, ' ', (15-i)*3 );
            memset( ascii, ' ', 15-i );
            break;
         }
      }
      printf( "0x%08lx %s %s\n", offset, label, line );
//      printf( "%d %p %p %s\n", i, ptr, ptr-i, line );
      offset += 16;
   }
}
