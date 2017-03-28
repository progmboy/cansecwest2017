# Description of win32k 0day on windows 10

## root case
This vulnerability is an instance of double free vulnerability. In particular, the vulnerability is caused by DirectComposition compoment of windows 10, the trigger syscall is:
```cpp
STATUS NTAPI NtDCompositionProcessChannelBatchBuffer(
	IN DWORD hchannel,     
	IN DWORD batchBufSize,    
	OUT DWORD* dwRet1     
	OUT DWORD* dwRet2);
```
This syscall is general entry of DirectComposition(such as Creating resource, inputSink ... and other operation on them),identify by class in bachBuffer of channel, bachBuffer is associate with channel, return from Creating channel by NtDCompositionCreateChannel. the batchBuffer general format is as below:

```cpp
typedef struct _BATCHBUFFER_INFO
{
	DWORD dwCommandId;
    HANDLE hResource;
    DWORD dwProperty;
    DWORD dwBufferSize;
    UCHAR szBuffer[1];
}BATCHBUFFER_INFO, *BATCHBUFFER_INFO;
```

and the command Id in this list:

```cpp
enum DCPROCESSCOMMANDID
{
	nCmdProcessCommandBufferIterator,
	nCmdCreateResource,
	nCmdOpenSharedResource,
	nCmdReleaseResource,
	nCmdGetAnimationTime,
	nCmdCapturePointer,
	nCmdOpenSharedResourceHandle,
	nCmdSetResourceCallbackId,
	nCmdSetResourceIntegerProperty,
	nCmdSetResourceFloatProperty,
	nCmdSetResourceHandleProperty,
	nCmdSetResourceBufferProperty,
	nCmdSetResourceReferenceProperty,
	nCmdSetResourceReferenceArrayProperty,
	nCmdSetResourceAnimationProperty,
	nCmdSetResourceDeletedNotificationTag,
	nCmdAddVisualChild,
	nCmdRedirectMouseToHwnd,
	nCmdSetVisualInputSink,
	nCmdRemoveVisualChild
};
```
When we call the syscall with dwCommandId=nCmdSetResourceBufferProperty, it finnal reached here:
```asm
kd> k  # ChildEBP RetAddr
00 926c2aac 95a877cd win32kbase!DirectComposition::CBaseExpressionMarshaler::SetBufferProperty
01 926c2ad4 95a802dd win32kbase!DirectComposition::CKeyframeAnimationMarshaler::SetBufferProperty+0x29
02 926c2b40 95ab5a19 win32kbase!DirectComposition::CApplicationChannel::ProcessCommandBufferIterator+0x1e1
03 926c2b98 95a7f9e6 win32kbase!DirectComposition::CApplicationChannel::ProcessCommandBuffer+0x69
04 926c2bfc 81fa5387 win32kbase!NtDCompositionProcessChannelBatchBuffer+0xf6
05 926c2bfc 779e38b0 nt!KiSystemServicePostCall
WARNING: Stack unwind information not available. Following frames may be wrong.
06 012ffb44 002e632e ntdll!KiFastSystemCallRet
```

```cpp
DWORD DirectComposition::CBaseExpressionMarshaler::SetBufferProperty(void *resource, int a2) {   
  //...
  //alloc buffer and then store it in resource   
  bufPtr=Win32AllocPoolWithQuota(*(DWORD*)(a2+0x14),'ndCD');   *((DWORD*)resource+13)=bufPtr;   if (!bufPtr)     return 0xC0000017;
  //copy data to 
  v14=*(DWORD*)(a2+16);   
  if (StringCbLengthW((WORD*)resource+28,v33,v34)>=0)   {     
      v15=*(unsigned __int16 **)(a2+16);     
      v16=*((_DWORD *)resource+13);     
      *((_DWORD *)resource+14)+=2;     
      if (StringCbCopyW(v15,v35,v36)>=0)     {      
          *((_DWORD *)resource+2)&=0xFFFFFDFF;      
          goto LABEL_7;     
      }   
  }
  //if fail, got here   
  //free the bufferPtr,but forgot to clear it in resource   
  bufPtr_1=*((DWORD*)resource+13);   
  v8=0xC000000D;   
  if (IsWin32FreePoolImplSupported()>=0)     
  	Win32FreePoolImpl(bufPtr_1);   
    return v8; 
}
```

As show above implement of DirectComposition::CBaseExpressionMarshaler::SetBufferProperty, it first alloc a buffer and then store it in resource, if (StringCbLengthW((WORD*)resource+28,v33,v34)<0||...), then it free the buffer but forgot clear it in resource. when the resource being drestroyed, the buffer will be free again.

```cpp
kd> k  # ChildEBP RetAddr 09 a1677a94 9b8b1b5a win32kfull!Win32FreePoolImpl+0x3b
0a a1677ab0 9b85fbb3 win32kbase!DirectComposition::CBaseExpressionMarshaler::~CBaseExpressionMarshaler+0x4d9ce
0b a1677ac8 9b861594 win32kbase!DirectComposition::CApplicationChannel::ReleaseResource+0x153
0c a1677ae4 9b8603e5 win32kbase!DirectComposition::CApplicationChannel::ReleaseResource+0x44
0d a1677b40 9b895a19 win32kbase!DirectComposition::CApplicationChannel::ProcessCommandBufferIterator+0x2e9
0e a1677b98 9b85f9e6 win32kbase!DirectComposition::CApplicationChannel::ProcessCommandBuffer+0x69
0f a1677bfc 81b41387 win32kbase!NtDCompositionProcessChannelBatchBuffer+0xf6
10 a1677bfc 774f38b0 nt!KiSystemServicePostCall
```

```cpp
void DirectComposition::CBaseExpressionMarshaler::~CBaseExpressionMarshaler(void* resource) {   
    int v1=this;   
    int v2=*((DWORD*)this+0xA);   
    *(DWORD *)this=&DirectComposition::CBaseExpressionMarshaler::vftable;   
    if (v2&&IsWin32FreePoolImplSupported()>=0)     
        Win32FreePoolImpl(v5, v4, v2);
    // free again   
    bufferPtr=*((DWORD*)v1+13);   
    if (bufferPtr)   {     
        if (IsWin32FreePoolImplSupported()>=0)       
            Win32FreePoolImpl(bufferPtr);     
        *((DWORD *)v1+14)=0;   
    } 
}
```




