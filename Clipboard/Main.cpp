#include <tchar.h>
#include <windows.h>

int _tmain()
{
	if ( !OpenClipboard(NULL) )
	{//�򿪼��а�
		return -1;
	}
	if( !EmptyClipboard() )
	{//��ռ��а�
		return -1;
	}
	//�����ڴ�
	HGLOBAL hGlob = GlobalAlloc(GMEM_FIXED, 64);
	strcpy_s((char*)hGlob, 64, "������ݽ������õ����а�\r\n");
	//�����ݷ��õ����а�
	if ( SetClipboardData( CF_TEXT, hGlob ) == NULL )
	{
		CloseClipboard();
		GlobalFree(hGlob);
		return -1;
	}
	//�رռ��а�
	CloseClipboard();
	return 0;
}