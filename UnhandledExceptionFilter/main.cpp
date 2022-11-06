#include <tchar.h>
#include <windows.h>

int _tmain()
{
	int i = 0;
	__try
	{
		SetErrorMode(SEM_NOGPFAULTERRORBOX);

		i /= i;

	}
	__except(UnhandledExceptionFilter(GetExceptionInformation()))
	{

	}
	return 0;
}