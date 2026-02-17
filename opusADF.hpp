#pragma once

#include <filesystem>
#include "adf_typed_list.hh"

namespace adf {
struct AdfDeviceDeleter {
    void operator()(AdfDevice *device) const;
};

struct AdfVolumeDeleter {
    void operator()(AdfVolume *volume) const;
};
}  // namespace internal

class cADFFindData {
public:
    cADFFindData(AdfTypedList<AdfEntry>&& directory) : mDirectoryList(std::move(directory)), mCurrentEntry(mDirectoryList.begin()) {}

    AdfTypedList<AdfEntry> mDirectoryList;
    AdfTypedIterator<AdfEntry> mCurrentEntry;
    std::regex mFindMask;
};

class cADFPluginData {
    HINSTANCE				mAdfDll{};
    size_t                  mLastError{};
    std::filesystem::path   mPath;

    std::unique_ptr<AdfDevice, adf::AdfDeviceDeleter> mAdfDevice;
    std::unique_ptr<AdfVolume, adf::AdfVolumeDeleter> mAdfVolume;

protected:
    LPVFSFILEDATAHEADER GetVFSforEntry(const AdfEntry *pEntry, HANDLE pHeap);
    void GetWfdForEntry(const AdfEntry &entry, LPWIN32_FIND_DATA pData);

    FILETIME            GetFileTime(const AdfEntry &entry);
    void                SetEntryTime(AdfFile *pFile, FILETIME pFT);

    bool LoadFile(std::wstring_view pAfPath);
    AdfTypedList<AdfEntry> GetCurrentDirectoryList();

public:
    bool AdfChangeToPath(std::wstring_view pPath, bool pIgnoreLast = false);

    bool ReadDirectory(LPVFSREADDIRDATAW lpRDD);
    bool ReadFile(AdfFile* pFile, std::span<uint8_t> buffer, LPDWORD readSize);

    AdfFile* OpenFile(std::wstring pPath);
    void CloseFile(AdfFile* pFile);
 
    size_t GetAvailableSize(const std::wstring& pFile);
    size_t GetTotalSize(const std::wstring& pFile);

    int Delete(LPVFSBATCHDATAW lpBatchData, std::wstring_view path, std::wstring_view pFile, bool pAll = false);

    cADFFindData *FindFirstFile(LPWSTR lpszPath, LPWIN32_FIND_DATA lpwfdData, HANDLE hAbortEvent);
    bool FindNextFile(cADFFindData* lpRAF, LPWIN32_FIND_DATA lpwfdData);
    void FindClose(cADFFindData* lpRAF);

    int Import(LPVFSBATCHDATAW lpBatchData, std::wstring_view pFile, std::wstring_view pPath);
    int ImportFile(LPVFSBATCHDATAW lpBatchData, std::wstring_view pFile, std::wstring_view pPath);
    int ImportPath(LPVFSBATCHDATAW lpBatchData, std::wstring_view pFile, std::wstring_view pPath);

    int Extract(LPVFSBATCHDATAW lpBatchData, std::wstring_view pFile, std::wstring_view pDest);
    int ExtractFile(LPVFSBATCHDATAW lpBatchData, const AdfEntry* pEntry, const std::wstring& pDest);
    int ExtractPath(LPVFSBATCHDATAW lpBatchData, std::wstring_view pPath, std::wstring_view pDest);

    int ContextVerb(LPVFSCONTEXTVERBDATAW lpVerbData);
    uint32_t BatchOperation(std::wstring_view lpszPath, LPVFSBATCHDATAW lpBatchData);
    bool PropGet(vfsProperty propId, LPVOID lpPropData, LPVOID lpData1, LPVOID lpData2, LPVOID lpData3);
};
