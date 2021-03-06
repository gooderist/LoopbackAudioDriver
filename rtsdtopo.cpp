/*
Module Name:
  rtsdtopo.cpp

Abstract:
  Implementation of topology miniport.
*/

#include "rtsdaudio.h"
#include "common.h"
#include "rtsdwave.h"
#include "rtsdtopo.h"
#include "toptable.h"


/*********************************************************************
* Topology/Wave bridge connection                                    *
*                                                                    *
*              +------+    +------+                                  *
*              | Wave |    | Topo |                                  *
*              |      |    |      |                                  *
*  Capture <---|0    1|<===|4    1|<--- Synth                        *
*              |      |    |      |                                  *
*   Render --->|2    3|===>|0     |                                  *
*              +------+    |      |                                  *
*                          |     2|<--- Mic                          *
*                          |      |                                  *
*                          |     3|---> Line Out                     *
*                          +------+                                  *
*********************************************************************/
PHYSICALCONNECTIONTABLE TopologyPhysicalConnections =
{
  KSPIN_TOPO_WAVEOUT_SOURCE,  // TopologyIn
  KSPIN_TOPO_WAVEIN_DEST,     // TopologyOut
  KSPIN_WAVE_CAPTURE_SOURCE,  // WaveIn
  KSPIN_WAVE_RENDER_SOURCE    // WaveOut
};

#pragma code_seg("PAGE")

//=============================================================================
NTSTATUS CreateMiniportTopology( 
  OUT PUNKNOWN *              Unknown,
  IN  REFCLSID,
  IN  PUNKNOWN                UnknownOuter OPTIONAL,
  IN  POOL_TYPE               PoolType 
)
/*
Routine Description:
    Creates a new topology miniport.

Arguments:
  Unknown - 
  RefclsId -
  UnknownOuter -
  PoolType - 

Return Value:
  NT status code.
*/
{
  PAGED_CODE();
  ASSERT(Unknown);
  STD_CREATE_BODY(CMiniportTopology, Unknown, UnknownOuter, PoolType);
}

//=============================================================================
NTSTATUS PropertyHandler_Topology ( 
    IN PPCPROPERTY_REQUEST      PropertyRequest 
)
/*
Routine Description:
  Redirects property request to miniport object

Arguments:
  PropertyRequest - 

Return Value:
  NT status code.
*/
{
  PAGED_CODE();
  ASSERT(PropertyRequest);
  DPF_ENTER(("[PropertyHandler_Topology]"));

  // PropertryRequest structure is filled by portcls. 
  // MajorTarget is a pointer to miniport object for miniports.
  return ((PCMiniportTopology)(PropertyRequest->MajorTarget))->PropertyHandlerGeneric(PropertyRequest);
}

//=============================================================================
CMiniportTopology::~CMiniportTopology(void)
/*
Routine Description:
  Topology miniport destructor

Arguments:

Return Value:
  NT status code.
*/
{
  PAGED_CODE();
  DPF_ENTER(("[CMiniportTopology::~CMiniportTopology]"));
  if (m_AdapterCommon) {
    m_AdapterCommon->Release();
  }
}

//=============================================================================
NTSTATUS CMiniportTopology::DataRangeIntersection( 
  IN  ULONG                   PinId,
  IN  PKSDATARANGE            ClientDataRange,
  IN  PKSDATARANGE            MyDataRange,
  IN  ULONG                   OutputBufferLength,
  OUT PVOID                   ResultantFormat     OPTIONAL,
  OUT PULONG                  ResultantFormatLength 
)
/*
Routine Description:
  The DataRangeIntersection function determines the highest quality 
  intersection of two data ranges.

Arguments:
  PinId - Pin for which data intersection is being determined. 
  ClientDataRange - Pointer to KSDATARANGE structure which contains the data range 
                    submitted by client in the data range intersection property 
                    request. 
  MyDataRange - Pin's data range to be compared with client's data range. 
  OutputBufferLength - Size of the buffer pointed to by the resultant format 
                       parameter. 
  ResultantFormat - Pointer to value where the resultant format should be 
                    returned. 
  ResultantFormatLength - Actual length of the resultant format that is placed 
                          at ResultantFormat. This should be less than or equal 
                          to OutputBufferLength. 

Return Value:
  NT status code.
*/
{
  PAGED_CODE();
  DPF_ENTER(("[CMiniportTopology::DataRangeIntersection]"));
  return (STATUS_NOT_IMPLEMENTED);
}

//=============================================================================
STDMETHODIMP CMiniportTopology::GetDescription( 
  OUT PPCFILTER_DESCRIPTOR *  OutFilterDescriptor 
)
/*
Routine Description:
  The GetDescription function gets a pointer to a filter description. 
  It provides a location to deposit a pointer in miniport's description 
  structure. This is the placeholder for the FromNode or ToNode fields in 
  connections which describe connections to the filter's pins. 

Arguments:
  OutFilterDescriptor - Pointer to the filter description. 

Return Value:
  NT status code.
*/
{
    PAGED_CODE();
    ASSERT(OutFilterDescriptor);
    DPF_ENTER(("[CMiniportTopology::GetDescription]"));

    *OutFilterDescriptor = m_FilterDescriptor;
    return (STATUS_SUCCESS);
}

//=============================================================================
STDMETHODIMP CMiniportTopology::Init( 
  IN PUNKNOWN                 UnknownAdapter,
  IN PRESOURCELIST            ResourceList,
  IN PPORTTOPOLOGY            Port_ 
)
/*
Routine Description:
  The Init function initializes the miniport. Callers of this function 
  should run at IRQL PASSIVE_LEVEL

Arguments:
  UnknownAdapter - A pointer to the Iuknown interface of the adapter object. 
  ResourceList - Pointer to the resource list to be supplied to the miniport 
                 during initialization. The port driver is free to examine the 
                 contents of the ResourceList. The port driver will not be 
                 modify the ResourceList contents. 
  Port - Pointer to the topology port object that is linked with this miniport. 

Return Value:
  NT status code.
*/
{
  PAGED_CODE();
  ASSERT(UnknownAdapter);
  ASSERT(Port_);

  DPF_ENTER(("[CMiniportTopology::Init]"));

  // aus dem Konstruktor
  m_AdapterCommon = NULL;
  m_FilterDescriptor = NULL;

  NTSTATUS ntStatus;

  ntStatus = UnknownAdapter->QueryInterface( 
    IID_IAdapterCommon,
    (PVOID *) &m_AdapterCommon
  );
  if (NT_SUCCESS(ntStatus)) {
    m_AdapterCommon->MixerReset();
  }

  if (!NT_SUCCESS(ntStatus)) {
    // clean up AdapterCommon
    if (m_AdapterCommon) {
      m_AdapterCommon->Release();
      m_AdapterCommon = NULL;
    }
  }

  if (NT_SUCCESS(ntStatus)) {
    m_FilterDescriptor = &MiniportFilterDescriptor;
    m_AdapterCommon->MixerMuxWrite(KSPIN_TOPO_MIC_SOURCE);
  }

  return ntStatus;
} // Init

//=============================================================================
STDMETHODIMP CMiniportTopology::NonDelegatingQueryInterface( 
    IN  REFIID                  Interface,
    OUT PVOID                   * Object 
)
/*
Routine Description:
  QueryInterface for MiniportTopology

Arguments:
  Interface - GUID of the interface
  Object - interface object to be returned.

Return Value:
  NT status code.
*/
{
  PAGED_CODE();
  ASSERT(Object);

  if (IsEqualGUIDAligned(Interface, IID_IUnknown)) {
    *Object = PVOID(PUNKNOWN(this));
  } else if (IsEqualGUIDAligned(Interface, IID_IMiniport)) {
    *Object = PVOID(PMINIPORT(this));
  } else if (IsEqualGUIDAligned(Interface, IID_IMiniportTopology)) {
    *Object = PVOID(PMINIPORTTOPOLOGY(this));
  }  else {
    *Object = NULL;
  }

  if (*Object) {
    // We reference the interface for the caller.
    PUNKNOWN(*Object)->AddRef();
    return(STATUS_SUCCESS);
  }
  return(STATUS_INVALID_PARAMETER);
} // NonDelegatingQueryInterface

//=============================================================================
NTSTATUS  CMiniportTopology::PropertyHandlerGeneric(
    IN  PPCPROPERTY_REQUEST     PropertyRequest
)
/*
Routine Description:
  Handles all properties for this miniport.

Arguments:
  PropertyRequest - property request structure

Return Value:
  NT status code.
*/
{
  PAGED_CODE();
  
  NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;
  switch (PropertyRequest->PropertyItem->Id)
  {
    case KSPROPERTY_AUDIO_VOLUMELEVEL:
      ntStatus = PropertyHandlerVolume(PropertyRequest);
      break;
    
    case KSPROPERTY_AUDIO_CPU_RESOURCES:
      ntStatus = PropertyHandlerCpuResources(PropertyRequest);
      break;

    case KSPROPERTY_AUDIO_MUTE:
      ntStatus = PropertyHandlerMute(PropertyRequest);
      break;

    case KSPROPERTY_AUDIO_MUX_SOURCE:
      ntStatus = PropertyHandlerMuxSource(PropertyRequest);
      break;

    default:
      DPF(D_TERSE, ("[PropertyHandlerGeneric: Invalid Device Request]"));
  }

  return ntStatus;
} // PropertyHandlerGeneric

//=============================================================================
NTSTATUS CMiniportTopology::PropertyHandlerBasicSupportVolume(
    IN  PPCPROPERTY_REQUEST PropertyRequest
)
/*
Routine Description:
  Handles BasicSupport for Volume nodes.

Arguments:  
  PropertyRequest - property request structure

Return Value:
  NT status code.
*/
{
  PAGED_CODE();

  NTSTATUS ntStatus = STATUS_SUCCESS;
  ULONG cbFullProperty = sizeof(KSPROPERTY_DESCRIPTION) + sizeof(KSPROPERTY_MEMBERSHEADER) + sizeof(KSPROPERTY_STEPPING_LONG);

  if(PropertyRequest->ValueSize >= (sizeof(KSPROPERTY_DESCRIPTION))) {
    PKSPROPERTY_DESCRIPTION PropDesc = PKSPROPERTY_DESCRIPTION(PropertyRequest->Value);

    PropDesc->AccessFlags       = KSPROPERTY_TYPE_ALL;
    PropDesc->DescriptionSize   = cbFullProperty;
    PropDesc->PropTypeSet.Set   = KSPROPTYPESETID_General;
    PropDesc->PropTypeSet.Id    = VT_I4;
    PropDesc->PropTypeSet.Flags = 0;
    PropDesc->MembersListCount  = 1;
    PropDesc->Reserved          = 0;

    // if return buffer can also hold a range description, return it too
    if(PropertyRequest->ValueSize >= cbFullProperty) {
      // fill in the members header
      PKSPROPERTY_MEMBERSHEADER Members = PKSPROPERTY_MEMBERSHEADER(PropDesc + 1);

       Members->MembersFlags   = KSPROPERTY_MEMBER_STEPPEDRANGES;
       Members->MembersSize    = sizeof(KSPROPERTY_STEPPING_LONG);
       Members->MembersCount   = 1;
       Members->Flags          = 0;

      // fill in the stepped range
      PKSPROPERTY_STEPPING_LONG Range = PKSPROPERTY_STEPPING_LONG(Members + 1);

      // BUGBUG these are from SB16 driver.
      // Are these valid.
      Range->Bounds.SignedMaximum = 0xE0000;      // 14  (dB) * 0x10000
      Range->Bounds.SignedMinimum = 0xFFF20000;   // -14 (dB) * 0x10000
      Range->SteppingDelta        = 0x20000;      // 2   (dB) * 0x10000
      Range->Reserved             = 0;

      // set the return value size
      PropertyRequest->ValueSize = cbFullProperty;
    } else {
       PropertyRequest->ValueSize = 0;
       ntStatus = STATUS_BUFFER_TOO_SMALL;
    }
  } else if(PropertyRequest->ValueSize >= sizeof(ULONG)) {
    // if return buffer can hold a ULONG, return the access flags
    PULONG AccessFlags = PULONG(PropertyRequest->Value);

    PropertyRequest->ValueSize = sizeof(ULONG);
    *AccessFlags = KSPROPERTY_TYPE_ALL;
  } else if (PropertyRequest->ValueSize == 0) {
    // Send the caller required value size.
    PropertyRequest->ValueSize = cbFullProperty;
    ntStatus = STATUS_BUFFER_OVERFLOW;
  } else {
    PropertyRequest->ValueSize = 0;
    ntStatus = STATUS_BUFFER_TOO_SMALL;
  }
  return ntStatus;
} // PropertyHandlerBasicSupportVolume

//=============================================================================
NTSTATUS CMiniportTopology::PropertyHandlerCpuResources( 
    IN  PPCPROPERTY_REQUEST PropertyRequest 
)
/*
Routine Description:
  Processes KSPROPERTY_AUDIO_CPURESOURCES

Arguments:  
  PropertyRequest - property request structure

Return Value:
  NT status code.
*/
{
  PAGED_CODE();
  DPF_ENTER(("[CMiniportTopology::PropertyHandlerCpuResources]"));

  NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

  if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
    ntStatus = ValidatePropertyParams(PropertyRequest, sizeof(ULONG));
    if (NT_SUCCESS(ntStatus)) {
      *(PLONG(PropertyRequest->Value)) = KSAUDIO_CPU_RESOURCES_NOT_HOST_CPU;
       PropertyRequest->ValueSize = sizeof(LONG);
    }
  } else if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT) {
    ntStatus = PropertyHandler_BasicSupport( 
      PropertyRequest, 
      KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT,
      VT_ILLEGAL
    );
  }
  return ntStatus;
} // PropertyHandlerCpuResources

//=============================================================================
NTSTATUS CMiniportTopology::PropertyHandlerMute(
    IN  PPCPROPERTY_REQUEST PropertyRequest
)
/*
Routine Description:
  Property handler for KSPROPERTY_AUDIO_MUTE

Arguments:
  PropertyRequest - property request structure

Return Value:
  NT status code.
*/
{
  PAGED_CODE();

  DPF_ENTER(("[CMiniportTopology::PropertyHandlerMute]"));

  NTSTATUS                    ntStatus;
  LONG                        lChannel;
  PBOOL                       pfMute;

  if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT) {
    ntStatus = PropertyHandler_BasicSupport(
     PropertyRequest,
     KSPROPERTY_TYPE_ALL,
     VT_BOOL
    );
  } else {
    ntStatus = ValidatePropertyParams(   
      PropertyRequest, 
      sizeof(BOOL), 
      sizeof(LONG)
    );
    if (NT_SUCCESS(ntStatus)) {
      lChannel = * PLONG (PropertyRequest->Instance);
      pfMute   = PBOOL (PropertyRequest->Value);

      if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
        *pfMute = m_AdapterCommon->MixerMuteRead(PropertyRequest->Node);
        PropertyRequest->ValueSize = sizeof(BOOL);
        ntStatus = STATUS_SUCCESS;
      } else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET) {
        m_AdapterCommon->MixerMuteWrite(PropertyRequest->Node, *pfMute);
        ntStatus = STATUS_SUCCESS;
      }
    } else {
      DPF(D_TERSE, ("[PropertyHandlerMute - Invalid parameter]"));
      ntStatus = STATUS_INVALID_PARAMETER;
    }
  }
  return ntStatus;
} // PropertyHandlerMute

//=============================================================================
NTSTATUS  CMiniportTopology::PropertyHandlerMuxSource(
    IN  PPCPROPERTY_REQUEST     PropertyRequest
)
/*
Routine Description:
  PropertyHandler for KSPROPERTY_AUDIO_MUX_SOURCE.

Arguments:
  PropertyRequest - property request structure

Return Value:
  NT status code.
*/
{
  PAGED_CODE();
  DPF_ENTER(("[CMiniportTopology::PropertyHandlerMuxSource]"));

  NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

  // Validate node
  // This property is only valid for WAVEIN_MUX node.
  //
  // TODO if (WAVEIN_MUX == PropertyRequest->Node)
  {
    if (PropertyRequest->ValueSize >= sizeof(ULONG)) {
      PULONG pulMuxValue = PULONG(PropertyRequest->Value);
        
      if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
        *pulMuxValue = m_AdapterCommon->MixerMuxRead();
        PropertyRequest->ValueSize = sizeof(ULONG);
        ntStatus = STATUS_SUCCESS;
      } else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET) {
         m_AdapterCommon->MixerMuxWrite(*pulMuxValue);
         ntStatus = STATUS_SUCCESS;
      } else if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT) {
         ntStatus = PropertyHandler_BasicSupport(PropertyRequest,  KSPROPERTY_TYPE_ALL, VT_I4);
      }
    } else {
      DPF(D_TERSE, ("[PropertyHandlerMuxSource - Invalid parameter]"));
      ntStatus = STATUS_INVALID_PARAMETER;
    }
  }
  return ntStatus;
} // PropertyHandlerMuxSource

//=============================================================================
NTSTATUS CMiniportTopology::PropertyHandlerVolume(
    IN  PPCPROPERTY_REQUEST     PropertyRequest     
)
/*
Routine Description:
  Property handler for KSPROPERTY_AUDIO_VOLUMELEVEL

Arguments:
  PropertyRequest - property request structure

Return Value:
  NT status code.
*/
{
  PAGED_CODE();
  DPF_ENTER(("[CMiniportTopology::PropertyHandlerVolume]"));

  NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;
  LONG lChannel;
  PULONG pulVolume;

  if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT) {
      ntStatus = PropertyHandlerBasicSupportVolume(PropertyRequest);
  } else {
    ntStatus = ValidatePropertyParams(PropertyRequest, sizeof(ULONG), sizeof(KSNODEPROPERTY_AUDIO_CHANNEL)-sizeof(KSNODEPROPERTY));
    if (NT_SUCCESS(ntStatus)) {
      lChannel = * (PLONG (PropertyRequest->Instance));
      pulVolume = PULONG (PropertyRequest->Value);

      if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET) {
        *pulVolume = m_AdapterCommon->MixerVolumeRead(PropertyRequest->Node, lChannel);
        PropertyRequest->ValueSize = sizeof(ULONG);                
        ntStatus = STATUS_SUCCESS;
      } else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET) {
        m_AdapterCommon->MixerVolumeWrite(PropertyRequest->Node, lChannel, *pulVolume);
        ntStatus = STATUS_SUCCESS;
      }
    } else {
      DPF(D_TERSE, ("[PropertyHandlerVolume - Invalid parameter]"));
      ntStatus = STATUS_INVALID_PARAMETER;
    }
  }
  return ntStatus;
} // PropertyHandlerVolume

#pragma code_seg()

