// SampleDll.cpp : ���� DLL Ӧ�ó���ĵ���������
//

#include "stdafx.h"
#include "SampleDll.h"


// ���ǵ���������һ��ʾ��
SAMPLEDLL_API int nSampleDll=0;

// ���ǵ���������һ��ʾ����
SAMPLEDLL_API int fnSampleDll(void)
{
	return 42;
}

// �����ѵ�����Ĺ��캯����
// �й��ඨ�����Ϣ������� SampleDll.h
CSampleDll::CSampleDll()
{
	return;
}
