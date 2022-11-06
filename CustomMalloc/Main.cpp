#include <tchar.h>
#include <windows.h>

#include "mymemdef.h"

int _tmain()
{
	void* p = malloc(1024);
	free(p);

	p = calloc(40,sizeof(int));
	free(p);

#undef malloc
#undef calloc
#undef free

	p = malloc(1024);
	free(p);

	p = calloc(40,sizeof(int));
	free(p);
	return 0;
}