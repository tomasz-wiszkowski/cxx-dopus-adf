#pragma once

#include <filesystem>
#include "adf_typed_list.hh"

class cADFFindData {
public:
    cADFFindData(AdfTypedList<AdfEntry>&& directory) : mDirectoryList(std::move(directory)), mCurrentEntry(mDirectoryList.begin()) {}

    AdfTypedList<AdfEntry> mDirectoryList;
    AdfTypedIterator<AdfEntry> mCurrentEntry;
    std::regex mFindMask;
};

class cADFPluginData {
    HINSTANCE				mAdfDll;
    size_t                  mLastError;
    std::filesystem::path   mPath;

    std::unique_ptr<AdfDevice, void(*)(AdfDevice*)> mAdfDevice;
    std::unique_ptr<AdfVolume, void(*)(AdfVolume*)> mAdfVolume;

protected:

    LPVFSFILEDATAHEADER GetVFSforEntry(const AdfEntry *pEntry, HANDLE pHeap);
    void GetWfdForEntry(const AdfEntry &entry, LPWIN32_FIND_DATA pData);

    FILETIME            GetFileTime(const AdfEntry &entry);
    void                SetEntryTime(AdfFile *pFile, FILETIME pFT);

    bool LoadFile(std::wstring_view pAfPath);
    AdfTypedList<AdfEntry> GetCurrentDirectoryList();

public:
    cADFPluginData();
    ~cADFPluginData();

    bool AdfChangeToPath(std::wstring_view pPath, bool pIgnoreLast = false);

    bool ReadDirectory(LPVFSREADDIRDATAW lpRDD);
    bool ReadFile(AdfFile* pFile, std::span<uint8_t> buffer, LPDWORD readSize);

    AdfFile* OpenFile(std::wstring pPath);
    void CloseFile(AdfFile* pFile);
 
    size_t GetAvailableSize(const std::wstring& pFile);
    size_t GetTotalSize(const std::wstring& pFile);

    int Delete(LPVFSBATCHDATAW lpBatchData, std::wstring_view path, const std::wstring& pFile, bool pAll = false);

    cADFFindData *FindFirstFile(LPWSTR lpszPath, LPWIN32_FIND_DATA lpwfdData, HANDLE hAbortEvent);
    bool FindNextFile(cADFFindData* lpRAF, LPWIN32_FIND_DATA lpwfdData);
    void FindClose(cADFFindData* lpRAF);

    int Import(LPVFSBATCHDATAW lpBatchData, std::wstring_view pFile, std::wstring_view pPath);
    int ImportFile(LPVFSBATCHDATAW lpBatchData, std::wstring_view pFile, std::wstring_view pPath);
    int ImportPath(LPVFSBATCHDATAW lpBatchData, std::wstring_view pFile, std::wstring_view pPath);

    int Extract(LPVFSBATCHDATAW lpBatchData, const std::wstring& pFile, const std::wstring& pDest);
    int ExtractFile(LPVFSBATCHDATAW lpBatchData, const AdfEntry* pEntry, const std::wstring& pDest);
    int ExtractPath(LPVFSBATCHDATAW lpBatchData, const std::wstring& pPath, const std::wstring& pDest);

    int ContextVerb(LPVFSCONTEXTVERBDATAW lpVerbData);
    uint32_t BatchOperation(std::wstring_view lpszPath, LPVFSBATCHDATAW lpBatchData);
    bool PropGet(vfsProperty propId, LPVOID lpPropData, LPVOID lpData1, LPVOID lpData2, LPVOID lpData3);
};
