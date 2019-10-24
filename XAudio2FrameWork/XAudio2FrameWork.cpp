// XAudio2FrameWork.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>
#include <iostream>
#include <fstream>
#include <conio.h>



#include <memory>
#include <stdexcept>
#include <unordered_set>
#include <algorithm>
#include <vector>

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <sdkddkver.h>
#include <windows.h>

#include <wrl.h>
//#include <exception>
//

// to include in compiler include directories (both debug and release): $(DXSDK_DIR)\include
#include "C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include\XAudio2.h"
#include "C:\Users\Erland-i5\Documents\Visual Studio 2015\Projects\Mod_to_WAV\Mod_to_WAV\virtualfile.h"


#define LITTLE_CHINA_WAV    "D:\\Erland Backup\\C_SCHIJF\\erland\\BP7\\BIN\\EXE\\DEFAULT.WAV"

//#pragma(push)
#pragma pack(1)

struct RiffChunk {
public:
    char            chunkId[4];
    unsigned        chunkSize;
    char            format[4];

};

struct FmtChunk {
public:
    char            subChunk1Id[4];
    unsigned        subChunk1Size;
    unsigned short  audioFormat;
    unsigned short  numChannels;
    unsigned        sampleRate;
    unsigned        byteRate;
    unsigned short  blockAlign;
    unsigned short  bitsPerSample;
};

struct DataChunk {
public:
    char            subChunk2Id[4];
    unsigned        subChunk2Size;
    // followed by data
};

//#pragma(pop)



class WaveSound {
public:
    WaveSound() {}
    WaveSound( std::string& fileName ) 
    {
        loadFile( fileName );
    }
    ~WaveSound()
    {
        if ( data != nullptr )
            delete [] data;
    }

    void loadFile( std::string& fileName )
    {
        VirtualFile waveFile( std::string( LITTLE_CHINA_WAV ) );
        if ( waveFile.getIOError() != VIRTFILE_NO_ERROR )
            return;

        RiffChunk   riffChunk;
        FmtChunk    fmtChunk;
        DataChunk   dataChunk;
        waveFile.read( &riffChunk,sizeof( RiffChunk ) );
        waveFile.read( &fmtChunk,sizeof( FmtChunk ) );
        waveFile.read( &dataChunk,sizeof( DataChunk ) );

        std::cout
            << "\nReading riff Chunk:"
            << "\nChunkID           : " 
            << riffChunk.chunkId[0] << riffChunk.chunkId[1]
            << riffChunk.chunkId[2] << riffChunk.chunkId[3]
            << "\nChunk size        : " << riffChunk.chunkSize
            << "\nChunk format      : "
            << riffChunk.format[0] << riffChunk.format[1]
            << riffChunk.format[2] << riffChunk.format[3];
        std::cout << "\n"
            << "\nSubChunk 1 ID     : "
            << fmtChunk.subChunk1Id[0]
            << fmtChunk.subChunk1Id[1]
            << fmtChunk.subChunk1Id[2]
            << fmtChunk.subChunk1Id[3]
            << "\nSubChunk 1 size   : " << fmtChunk.subChunk1Size
            << "\nSub1 audioFormat  : " << fmtChunk.audioFormat
            << "\nSub1 numChannels  : " << fmtChunk.numChannels
            << "\nSub1 sampleRate   : " << fmtChunk.sampleRate
            << "\nSub1 byteRate     : " << fmtChunk.byteRate
            << "\nSub1 blockAlign   : " << fmtChunk.blockAlign
            << "\nSub1 bitsPerSample: " << fmtChunk.bitsPerSample;
        std::cout << "\n"
            << "\nSubChunk 2 ID     : "
            << dataChunk.subChunk2Id[0]
            << dataChunk.subChunk2Id[1]
            << dataChunk.subChunk2Id[2]
            << dataChunk.subChunk2Id[3]
            << "\nSubChunk 2 size   : " << dataChunk.subChunk2Size;

        // todo: file integrity checking

        // ref: https://docs.microsoft.com/en-us/previous-versions/windows/desktop/ee419019(v=vs.85)?redirectedfrom=MSDN
        waveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
        waveFormatEx.nChannels = fmtChunk.numChannels;
        waveFormatEx.nSamplesPerSec = fmtChunk.sampleRate;
        waveFormatEx.nAvgBytesPerSec = fmtChunk.byteRate;
        waveFormatEx.nBlockAlign = fmtChunk.blockAlign;
        waveFormatEx.wBitsPerSample = fmtChunk.bitsPerSample;
        waveFormatEx.cbSize = 0;

        dataSize = dataChunk.subChunk2Size;
        data = new short[dataSize / 2]; // assume 16bit data here
        waveFile.read( data,dataSize );

        isLoaded = true;
    }
public:
    unsigned        dataSize = 0;
    WAVEFORMATEX    waveFormatEx = { 0 };
    bool            isLoaded = false;
    signed short    *data = nullptr;
};






int main()
{
    // open .wav file for testing
    WaveSound   waveSound( std::string( LITTLE_CHINA_WAV ) );

    // without this XAudio2 will not work
    CoInitialize( NULL,COINIT_MULTITHREADED );

    // open XAudio2 COM system
    IXAudio2* pXAudio2 = NULL;
    HRESULT hr;
    if ( FAILED( hr = XAudio2Create( &pXAudio2,0,XAUDIO2_DEFAULT_PROCESSOR ) ) ) {
        std::cout << "\nError " << hr << " initializing XAudio2";
        _getch();
        return hr;
    }

    // create master voice
    IXAudio2MasteringVoice* pMasterVoice = NULL;
    if ( FAILED( hr = pXAudio2->CreateMasteringVoice( &pMasterVoice ) ) ) {
        std::cout << "\nError " << hr << " creating XAudio2 mastering voice";
        _getch();
        return hr;
    }

    // https://docs.microsoft.com/en-us/windows/win32/api/xaudio2/ns-xaudio2-xaudio2_buffer
    XAUDIO2_BUFFER xAudio2Buffer;
    xAudio2Buffer.Flags = 0; // or XAUDIO2_END_OF_STREAM
    xAudio2Buffer.AudioBytes = waveSound.dataSize;
    xAudio2Buffer.LoopBegin = 0;
    xAudio2Buffer.LoopCount = 0;
    xAudio2Buffer.LoopLength = 0;
    xAudio2Buffer.pAudioData = (BYTE *)waveSound.data;
    xAudio2Buffer.pContext = NULL;
    xAudio2Buffer.PlayBegin = 0;
    xAudio2Buffer.PlayLength = 0; // play the entire buffer

    // create source voice:
    IXAudio2SourceVoice* pSourceVoice;
    if ( FAILED( hr = pXAudio2->CreateSourceVoice( 
        &pSourceVoice,(WAVEFORMATEX*)&waveSound.waveFormatEx ) ) ) {
        std::cout << "\nError " << hr << " creating XAudio2 source voice";
        _getch();
        return hr;
    }

    // Submit an XAUDIO2_BUFFER to the source voice using the function SubmitSourceBuffer.
    if ( FAILED( hr = pSourceVoice->SubmitSourceBuffer( &xAudio2Buffer ) ) ) {
        std::cout << "\nError " << hr << " submitting XAudio2 buffer to source voice";
        _getch();
        return hr;
    }

    // start replaying the voice:
    if ( FAILED( hr = pSourceVoice->Start( 0 ) ) ) {
        std::cout << "\nError " << hr << " starting XAudio2 voice replay";
        _getch();
        return hr;
    }

    




    // Hugs: https://www.youtube.com/watch?v=ck2XpEZYLYs
    // https://docs.microsoft.com/en-us/windows/win32/xaudio2/how-to--play-a-sound-with-xaudio2
    // https://docs.microsoft.com/en-us/windows/win32/xaudio2/how-to--load-audio-data-files-in-xaudio2


    _getch();

    // release XAudio2 system
    pXAudio2->Release();
    CoUninitialize();

    return 0;
}

