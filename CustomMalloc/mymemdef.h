#pragma once

#ifdef _WIN32
#define malloc(s)	HeapAlloc(GetProcessHeap(),0,s)
#define calloc(c,s)	HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(c)*(s))
#define free(p)		HeapFree(GetProcessHeap(),0,p)
#endif