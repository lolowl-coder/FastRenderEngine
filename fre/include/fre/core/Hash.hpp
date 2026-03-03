#pragma once

#include <stdint.h>

static unsigned int FNV1a32Hash ( void* data, unsigned int size )
{
	register uint32_t hval = 0;
	register unsigned char* bp = (unsigned char*)data;
	register unsigned char* be = bp + size;
	while ( bp < be )
	{
		hval ^= ( uint32_t ) * bp++;
		hval += ( hval << 1 ) + ( hval << 4 ) + ( hval << 7 ) + ( hval << 8 )
				+ ( hval << 24 );
	}
	return hval;
}
