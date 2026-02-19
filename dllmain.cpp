#include <strsafe.h>

#include "dopus_wstring_view_span.hh"
#include "stdafx.h"

// 8040a29e-0754-4ed7-a532-3688e08fff41
static constexpr GUID GUIDPlugin_ADF = {
    0x8040a29e,
    0x0754,
    0x4ed7,
    {0xa5, 0x32, 0x36, 0x88, 0xe0, 0x8f, 0xff, 0x41}};
HINSTANCE g_module_instance{};

extern "C" {
__declspec(dllexport) bool VFS_IdentifyW(LPVFSPLUGININFOW lpVFSInfo);
__declspec(dllexport) bool VFS_ReadDirectoryW(cADFPluginData* hData,
                                              LPVFSFUNCDATA lpVFSData,
                                              LPVFSREADDIRDATAW lpRDD);

__declspec(dllexport) cADFPluginData* VFS_Create(LPGUID pGuid);
__declspec(dllexport) void VFS_Destroy(cADFPluginData* hData);

__declspec(dllexport) AdfFile* VFS_CreateFileW(cADFPluginData* hData,
                                               LPVFSFUNCDATA lpVFSData,
                                               LPWSTR lpszPath,
                                               DWORD dwMode,
                                               DWORD dwFileAttr,
                                               DWORD dwFlags,
                                               LPFILETIME lpFT);

__declspec(dllexport) bool VFS_ReadFile(cADFPluginData* hData,
                                        LPVFSFUNCDATA lpVFSData,
                                        AdfFile* hFile,
                                        LPVOID lpData,
                                        DWORD dwSize,
                                        LPDWORD lpdwReadSize);
__declspec(dllexport) void VFS_CloseFile(cADFPluginData* hData,
                                         LPVFSFUNCDATA lpVFSData,
                                         AdfFile* hFile);

__declspec(dllexport) int VFS_ContextVerbW(cADFPluginData* hData,
                                           LPVFSFUNCDATA lpVFSData,
                                           LPVFSCONTEXTVERBDATAW lpVerbData);
__declspec(dllexport) UINT WINAPI
VFS_BatchOperationW(cADFPluginData* hData,
                    LPVFSFUNCDATA lpVFSData,
                    LPWSTR lpszPath,
                    LPVFSBATCHDATAW lpBatchData);
__declspec(dllexport) bool VFS_PropGetW(cADFPluginData* hVFSData,
                                        vfsProperty propId,
                                        LPVOID lpPropData,
                                        LPVOID lpData1,
                                        LPVOID lpData2,
                                        LPVOID lpData3);
__declspec(dllexport) long VFS_GetLastError(cADFPluginData* data);

__declspec(dllexport) bool VFS_GetFreeDiskSpaceW(
    cADFPluginData* hData,
    LPVFSFUNCDATA lpFuncData,
    LPWSTR lpszPath,
    unsigned __int64* piFreeBytesAvailable,
    unsigned __int64* piTotalBytes,
    unsigned __int64* piTotalFreeBytes);

__declspec(dllexport) cADFFindData* WINAPI
VFS_FindFirstFileW(cADFPluginData* hData,
                   LPVFSFUNCDATA lpVFSData,
                   LPWSTR lpszPath,
                   LPWIN32_FIND_DATA lpwfdData,
                   HANDLE hAbortEvent);
__declspec(dllexport) bool WINAPI
VFS_FindNextFileW(cADFPluginData* hData,
                  LPVFSFUNCDATA lpVFSData,
                  cADFFindData* hFind,
                  LPWIN32_FIND_DATA lpwfdData);
__declspec(dllexport) void WINAPI VFS_FindClose(cADFPluginData* hData,
                                                cADFFindData hFind);

__declspec(dllexport) BOOL WINAPI
VFS_ExtractFilesW(cADFPluginData* hData,
                  LPVFSFUNCDATA lpFuncData,
                  LPVFSEXTRACTFILESDATA lpExtractData);

__declspec(dllexport) bool VFS_USBSafe(LPOPUSUSBSAFEDATA pUSBSafeData);
__declspec(dllexport) bool VFS_Init(LPVFSINITDATA pInitData);
__declspec(dllexport) void VFS_Uninit();
__declspec(dllexport) LPVFSFILEDATAHEADER WINAPI
VFS_GetFileInformationW(cADFPluginData* hData,
                        LPVFSFUNCDATA lpVFSData,
                        LPWSTR lpszPath,
                        HANDLE hHeap,
                        DWORD dwFlags);
};

extern "C" int WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID) {
  g_module_instance = hInstance;
  return 1;
}

// Initialise plugin
bool VFS_Init(LPVFSINITDATA pInitData) {
  adfEnvInitDefault();
  adfLibInit();
  return true;
}

void VFS_Uninit() {
  adfLibCleanUp();
  adfEnvCleanUp();
}

bool VFS_IdentifyW(LPVFSPLUGININFOW lpVFSInfo) {
  // Initialise plugin information
  lpVFSInfo->idPlugin = GUIDPlugin_ADF;
  lpVFSInfo->dwFlags = VFSF_CANCONFIGURE | VFSF_NONREENTRANT;
  lpVFSInfo->dwCapabilities = VFSCAPABILITY_CASESENSITIVE |
                              VFSCAPABILITY_POSTCOPYREREAD |
                              VFSCAPABILITY_READONLY;

  StringCchCopyW(lpVFSInfo->lpszHandleExts, lpVFSInfo->cchHandleExtsMax,
                 L".adf;.hdf");
  StringCchCopyW(lpVFSInfo->lpszName, lpVFSInfo->cchNameMax, L"Amiga ADF/HDF");
  StringCchCopyW(lpVFSInfo->lpszDescription, lpVFSInfo->cchDescriptionMax,
                 L"Amiga Dos disk support");
  StringCchCopyW(lpVFSInfo->lpszCopyright, lpVFSInfo->cchCopyrightMax,
                 L"(c) Copyright 2018 Robert Crossfield");
  StringCchCopyW(lpVFSInfo->lpszURL, lpVFSInfo->cchURLMax,
                 L"github.com/segrax");

  return true;
}

bool VFS_USBSafe(LPOPUSUSBSAFEDATA pUSBSafeData) {
  return true;
}

cADFPluginData* VFS_Create(LPGUID pGuid) {
  return new cADFPluginData();
}

void VFS_Destroy(cADFPluginData* hData) {
  delete hData;
}

bool VFS_ReadFile(cADFPluginData* hData,
                  LPVFSFUNCDATA lpVFSData,
                  AdfFile* hFile,
                  LPVOID lpData,
                  DWORD dwSize,
                  LPDWORD lpdwReadSize) {
  if (!hData || !hFile || !lpData || !lpdwReadSize)
    return false;

  return hData->ReadFile(
      hFile, std::span<uint8_t>(static_cast<uint8_t*>(lpData), dwSize),
      lpdwReadSize);
}

void VFS_CloseFile(cADFPluginData* hData,
                   LPVFSFUNCDATA lpVFSData,
                   AdfFile* hFile) {
  hData->CloseFile(hFile);
}

AdfFile* VFS_CreateFileW(cADFPluginData* hData,
                         LPVFSFUNCDATA lpVFSData,
                         LPWSTR lpszPath,
                         DWORD dwMode,
                         DWORD dwFileAttr,
                         DWORD dwFlags,
                         LPFILETIME lpFT) {
  return (hData) ? hData->OpenFile(lpszPath) : nullptr;
}

bool VFS_ReadDirectoryW(cADFPluginData* hData,
                        LPVFSFUNCDATA lpFuncData,
                        LPVFSREADDIRDATAW lpRDD) {
  return hData->ReadDirectory(lpRDD);
}

int VFS_ContextVerbW(cADFPluginData* hData,
                     LPVFSFUNCDATA lpVFSData,
                     LPVFSCONTEXTVERBDATAW lpVerbData) {
  return hData->ContextVerb(lpVerbData);
}

UINT VFS_BatchOperationW(cADFPluginData* hData,
                         LPVFSFUNCDATA lpVFSData,
                         LPWSTR lpszPath,
                         LPVFSBATCHDATAW lpBatchData) {
  return hData->BatchOperation(lpszPath, lpBatchData);
}

bool VFS_PropGetW(cADFPluginData* hData,
                  vfsProperty propId,
                  LPVOID lpPropData,
                  LPVOID lpData1,
                  LPVOID lpData2,
                  LPVOID lpData3) {
  return hData->PropGet(propId, lpPropData, lpData1, lpData2, lpData3);
}

bool VFS_GetFreeDiskSpaceW(cADFPluginData* hData,
                           LPVFSFUNCDATA lpFuncData,
                           LPWSTR lpszPath,
                           uint64_t* piFreeBytesAvailable,
                           uint64_t* piTotalBytes,
                           uint64_t* piTotalFreeBytes) {
  if (piFreeBytesAvailable)
    *piFreeBytesAvailable = hData->GetAvailableSize(lpszPath);
  if (piTotalFreeBytes)
    *piTotalFreeBytes = hData->GetAvailableSize(lpszPath);
  if (piTotalBytes)
    *piTotalBytes = hData->GetTotalSize(lpszPath);

  return true;
}

cADFFindData* VFS_FindFirstFileW(cADFPluginData* hData,
                                 LPVFSFUNCDATA lpVFSData,
                                 LPWSTR lpszPath,
                                 LPWIN32_FIND_DATA lpwfdData,
                                 HANDLE hAbortEvent) {
  return (hData) ? hData->FindFirstFile(lpszPath, lpwfdData, hAbortEvent)
                 : (cADFFindData*)INVALID_HANDLE_VALUE;
}

bool VFS_FindNextFileW(cADFPluginData* hData,
                       LPVFSFUNCDATA lpVFSData,
                       cADFFindData* hFind,
                       LPWIN32_FIND_DATA lpwfdData) {
  return (hData && hFind) ? hData->FindNextFile(hFind, lpwfdData) : false;
}

void VFS_FindClose(cADFPluginData* hData, cADFFindData* hFind) {
  if (hData && hFind)
    hData->FindClose(hFind);
}

BOOL VFS_ExtractFilesW(cADFPluginData* hData,
                       LPVFSFUNCDATA lpFuncData,
                       LPVFSEXTRACTFILESDATA lpExtractData) {
  return hData->ExtractEntries(
      lpFuncData, dopus::wstring_view_span(lpExtractData->lpszFiles),
      lpExtractData->lpszDestPath);
}

long VFS_GetLastError(cADFPluginData* data) {
  return data->GetLastError();
}

LPVFSFILEDATAHEADER VFS_GetFileInformationW(cADFPluginData* data,
                                            LPVFSFUNCDATA lpVFSData,
                                            LPWSTR lpszPath,
                                            HANDLE hHeap,
                                            DWORD dwFlags) {
  return data->GetfileInformation(lpszPath, hHeap);
}
