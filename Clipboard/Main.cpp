#include <tchar.h>
#include <windows.h>

int _tmain()
{
	if ( !OpenClipboard(NULL) )
	{//打开剪切板
		return -1;
	}
	if( !EmptyClipboard() )
	{//清空剪切板
		return -1;
	}
	//分配内存
	HGLOBAL hGlob = GlobalAlloc(GMEM_FIXED, 64);
	strcpy_s((char*)hGlob, 64, "这段数据将被放置到剪切板\r\n");
	//将数据放置到剪切板
	if ( SetClipboardData( CF_TEXT, hGlob ) == NULL )
	{
		CloseClipboard();
		GlobalFree(hGlob);
		return -1;
	}
	//关闭剪切板
	CloseClipboard();
	return 0;
}