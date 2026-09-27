#pragma once

#include <stdint.h>
#include <functional>

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

namespace fre
{
	inline void hashCombine(size_t& seed, size_t value)
	{
		seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	template<typename T>
	inline void hashCombine(size_t& seed, const T& value)
	{
		hashCombine(seed, std::hash<T>{}(value));
	}

	template<typename T>
	inline void hashRange(size_t& seed, const std::vector<T>& values)
	{
		for(const auto& v : values)
			hashCombine(seed, v);
	}
}