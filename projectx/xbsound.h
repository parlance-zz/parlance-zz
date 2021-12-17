#ifndef _H_XBSOUND
#define _H_XBSOUND

#include <xtl.h>


struct RIFFHEADER
{
    FOURCC  fccChunkId;
    DWORD   dwDataSize;
};

#define RIFFCHUNK_FLAGS_VALID   0x00000001

class CRiffChunk
{
private:

    FOURCC            m_fccChunkId;       // Chunk identifier
    const CRiffChunk* m_pParentChunk;     // Parent chunk
    HANDLE            m_hFile;
    DWORD             m_dwDataOffset;     // Chunk data offset
    DWORD             m_dwDataSize;       // Chunk data size
    DWORD             m_dwFlags;          // Chunk flags

public:

    CRiffChunk();

    // Initialization
    VOID    Initialize( FOURCC fccChunkId, const CRiffChunk* pParentChunk,
                        HANDLE hFile );
    HRESULT Open();
    BOOL    IsValid()     { return !!(m_dwFlags & RIFFCHUNK_FLAGS_VALID); }

    // Data
    HRESULT ReadData( LONG lOffset, VOID* pData, DWORD dwDataSize );

    // Chunk information
    FOURCC  GetChunkId()  { return m_fccChunkId; }
    DWORD   GetDataSize() { return m_dwDataSize; }
};

class CWaveFile
{
private:

    HANDLE      m_hFile;            // File handle
    CRiffChunk  m_RiffChunk;        // RIFF chunk
    CRiffChunk  m_FormatChunk;      // Format chunk
    CRiffChunk  m_DataChunk;        // Data chunk
    CRiffChunk  m_WaveSampleChunk;  // Wave Sample chunk
    
public:

    CWaveFile();
    ~CWaveFile();

    // initialization
    HRESULT Open( const CHAR* strFileName );
    VOID    Close();

    // file format
    HRESULT GetFormat( WAVEFORMATEXTENSIBLE* pwfxFormat );

    // file data
    HRESULT ReadSample( DWORD dwPosition, VOID* pBuffer, DWORD dwBufferSize, 
                        DWORD* pdwRead );

    // loop region
    HRESULT GetLoopRegion( DWORD* pdwStart, DWORD* pdwLength );

    // file properties
    VOID    GetDuration( DWORD* pdwDuration ) { *pdwDuration = m_DataChunk.GetDataSize(); }
};

#endif