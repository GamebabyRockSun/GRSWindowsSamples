#include "StdAfx.h"
#include "IoBuffer.h"
#include "IoDefine.h"

CIoBuffer::CIoBuffer()
{
	Init();
}

CIoBuffer::~CIoBuffer()
{

}

void CIoBuffer::Init()
{
    m_vtBuffer      = NULL;
	m_nType         = -1;
	m_nIoSize       = 0;
	m_bKeepAlive    = true;
	m_nLen          = 0;
    m_nMaxLen       = 0;

	ZeroMemory(&m_ol, sizeof(OVERLAPPED));
}

CIoBuffer::CIoBuffer(int nIoType)
{
	Init();
	SetType(nIoType);
}

void CIoBuffer::SetType(int nIoType)
{
	m_nType = nIoType;
}

void CIoBuffer::AddData(const char * pData, UINT nSize)
{
    if( nSize > 0 && pData != NULL )
    {
        if( NULL == m_vtBuffer )
        {
            m_vtBuffer = (char*)GRS_CALLOC(nSize);
            m_nMaxLen = nSize;
        }
        else
        {
            if( m_nLen + nSize > m_nMaxLen )
            {
                m_vtBuffer = (char*)GRS_CREALLOC(m_vtBuffer,m_nLen + nSize);
                m_nMaxLen = m_nLen + nSize;
            }
        }

        for( UINT i = 0; i < nSize; i++ )
        {
            m_vtBuffer[m_nLen + i] = pData[i];
        }

        m_nLen += nSize;
    }
}

void CIoBuffer::AddData(CIoBuffer & IoBuffer)
{
	AddData( IoBuffer.GetBuffer(), IoBuffer.GetLen() );
}

void CIoBuffer::AddData(const string & str)
{
	AddData( str.c_str(), str.size() );
}

void CIoBuffer::AddData(const void * pData, UINT nSize)
{
	AddData((const char*)pData, nSize);
}

void CIoBuffer::AddData(UINT data)
{
	AddData(reinterpret_cast<const char*>(&data), sizeof(UINT));
}


void CIoBuffer::AddData(int data)
{
	AddData(reinterpret_cast<const char*>(&data), sizeof(int));
}

void CIoBuffer::AddData(char data)
{
	AddData(reinterpret_cast<char*>(&data), sizeof(char));
}

void CIoBuffer::InsertData(UINT nPos, const char * pData, UINT nSize)
{
	if( nSize > 0 && pData != NULL )
	{
        if( NULL == m_vtBuffer )
        {
            m_vtBuffer = (char*)GRS_CALLOC( nPos + nSize );
            m_nMaxLen = nPos + nSize;
        }
        else
        {
            if( (max(m_nLen,nPos) + nSize) > m_nMaxLen)
            {
                m_vtBuffer = (char*)GRS_CREALLOC(m_vtBuffer, max(m_nLen,nPos) + nSize );   
                m_nMaxLen = max(m_nLen,nPos) + nSize;
            }                    
        }
        
        if( nPos < m_nLen )
        {
            CopyMemory(&m_vtBuffer[nPos + nSize],&m_vtBuffer[nPos],m_nLen - nPos);
        }
        
        for( UINT i = 0; i < nSize; i++ )
        {
            m_vtBuffer[nPos + i] = pData[i];
        }

		m_nLen += nSize;
	}	
}

void CIoBuffer::Reserve(UINT len)
{
    if( NULL == m_vtBuffer )
    {
        m_vtBuffer = (char*)GRS_CALLOC( len + 1 );
    }
    else
    {
        m_vtBuffer = (char*)GRS_CREALLOC(m_vtBuffer, len + 1 );            
    }

    //m_nLen = len + 1;
    //m_vtBuffer.SetCount( len + 1 );
    //m_vtBuffer.Add('\0');
	//m_vtBuffer.reserve(len+1);
    m_nMaxLen = len + 1;

	m_wsabuf.len = len;
	m_wsabuf.buf = m_vtBuffer;
}

void CIoBuffer::IncreaseCapacity( UINT LenInc )
{
	Reserve( LenInc + m_nLen - 1 );
}

void CIoBuffer::Clear()
{
	GRS_SAFEFREE(m_vtBuffer);
	Init();
}