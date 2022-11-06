#include <tchar.h>
#include <windows.h>

#pragma pack(show)
struct BYTE1
{
	char ch1;
	int	i1;
};
#pragma push()
#pragma pack(1)
#pragma pack(show)
struct BYTE2
{
	char ch2;
	int	i2;
};
#pragma pop()

struct __declspec(align(1)) BYTE3
{
	char ch3;
	int  i3;
};

int _tmain()
{
	size_t sz1 = sizeof(BYTE1);
	size_t sz2 = sizeof(BYTE2);
	size_t sz3 = sizeof(BYTE3);

	sz1 = __alignof(BYTE1);
	sz2 = __alignof(BYTE2);
	sz3 = __alignof(BYTE3);

	return 0;
}