/*
Module Name:
    rtsdwave.h

Abstract:
    Definition of wavecyclic miniport class.
*/

#ifndef __RTSDWAVESTREAM_H_
#define __RTSDWAVESTREAM_H_

#include "rtsdwave.h"

///////////////////////////////////////////////////////////////////////////////
// CMiniportWaveCyclicStream 
//   

class CMiniportWaveCyclicStream : public IMiniportWaveCyclicStream, public IDmaChannel, public CUnknown {
protected:
  PCMiniportWaveCyclic      m_pMiniport;        // Miniport that created us  
  BOOLEAN                   m_fCapture;         // Capture or render.
  BOOLEAN                   m_fFormat16Bit;     // 16- or 8-bit samples.
  BOOLEAN                   m_fFormatStereo;    // Two or one channel.
  KSSTATE                   m_ksState;          // Stop, pause, run.
  ULONG                     m_ulPin;            // Pin Id.

  PRKDPC                    m_pDpc;             // Deferred procedure call object
  PKTIMER                   m_pTimer;           // Timer object

  BOOLEAN                   m_fDmaActive;       // Dma currently active? 
  ULONG                     m_ulDmaPosition;    // Position in Dma
  PVOID                     m_pvDmaBuffer;      // Dma buffer pointer
  ULONG                     m_ulDmaBufferSize;  // Size of dma buffer
  ULONG                     m_ulDmaMovementRate;// Rate of transfer specific to system
  ULONGLONG                 m_ullDmaTimeStamp;  // Dma time elasped 
  
public:
    DECLARE_STD_UNKNOWN();
    DEFINE_STD_CONSTRUCTOR(CMiniportWaveCyclicStream);
    ~CMiniportWaveCyclicStream();

	IMP_IMiniportWaveCyclicStream;
    IMP_IDmaChannel;

    NTSTATUS Init
    ( 
        IN  PCMiniportWaveCyclic Miniport,
        IN  ULONG               Channel,
        IN  BOOLEAN             Capture,
        IN  PKSDATAFORMAT       DataFormat
    );

    // Friends
    friend class CMiniportWaveCyclic;
};
typedef CMiniportWaveCyclicStream *PCMiniportWaveCyclicStream;

#endif

