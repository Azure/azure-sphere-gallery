#include "GetDeviceHash.h"
#include "applibs/log.h"

#include <stdint.h>
#include <unistd.h>

// Jenkins Hash Function (WikiPedia) - https://en.wikipedia.org/wiki/Jenkins_hash_function

uint32_t GetDeviceIdHash(const uint8_t* key, size_t length) 
{
	size_t i = 0;
	uint32_t hash = 0;
	while (i != length) {
		hash += key[i++];
		hash += hash << 10;
		hash ^= hash >> 6;
	}
	hash += hash << 3;
	hash ^= hash >> 11;
	hash += hash << 15;
	return hash;
}