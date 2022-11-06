#include <tchar.h>
#include <stdio.h>
#include <windows.h>
#include <eh.h>

void SEFunc();
void trans_func( unsigned int, EXCEPTION_POINTERS* );

class SE_Exception
{
private:
	unsigned int nSE;
public:
	SE_Exception() {}
	SE_Exception( unsigned int n ) : nSE( n ) {}
	~SE_Exception() {}
	unsigned int getSeNumber() { return nSE; }
};

int main( void )
{
	try
	{
		_set_se_translator( trans_func );
		SEFunc();
	}
	catch( SE_Exception e )
	{
		printf( "Caught a __try exception with SE_Exception.\n" );
	}

	_tsystem(_T("PAUSE"));
}

void SEFunc()
{
	__try
	{
		int x, y=0;
		x = 5 / y;
	}
	__finally
	{
		printf( "In finally\n" );
	}
}

void trans_func( unsigned int u, EXCEPTION_POINTERS* pExp )
{
	printf( "In trans_func.\n" );
	throw SE_Exception();
}