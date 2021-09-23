/*
 WebServe
 Copyright 2019 Peter Pearson.
 Originally taken from:
 Imagine
 Copyright 2011-2012 Peter Pearson.

 Licensed under the Apache License, Version 2.0 (the "License");
 You may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 ---------
*/

#include "memory.h"

#include <cstdlib>

#ifndef _MSC_VER
#ifndef __APPLE__
#include <malloc.h>
#endif
#endif

#define kL1_CACHE_LINE_SIZE 64

void mallocTrim()
{
#ifdef __linux__
	malloc_trim(0);
#endif
}

void* allocAligned(size_t size)
{
#ifdef _MSC_VER
	return _aligned_malloc(size, kL1_CACHE_LINE_SIZE);
#elif __APPLE__
	// have to do it the hard way
	void* mem = malloc(size + (kL1_CACHE_LINE_SIZE - 1) + sizeof(void*));
	char* alignedMem = ((char*)mem) + sizeof(void*);
	alignedMem += kL1_CACHE_LINE_SIZE - ((uint64_t)alignedMem) & (kL1_CACHE_LINE_SIZE - 1);
	((void**)alignedMem)[-1] = mem;
	return alignedMem;
#else
	return memalign(kL1_CACHE_LINE_SIZE, size);
#endif
}

void freeAligned(void* ptr)
{
	if (!ptr)
		return;

#ifdef _MSC_VER
	return _aligned_free(ptr);
#elif __APPLE__
	free(((void**)ptr)[-1]);
#else
	free(ptr);
#endif
}
