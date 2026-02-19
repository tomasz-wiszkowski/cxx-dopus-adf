#pragma once

#include <filesystem>
#include "adf_typed_list.hh"
#include "dopus_wstring_view_span.hh"

namespace adf {
struct AdfDeviceDeleter {
    void operator()(AdfDevice *device) const;
};

struct AdfVolumeDeleter {
    void operator()(AdfVolume *volume) const;
};
}  // namespace internal

/// @brief Guard object to set and restore fields.
template <typename T>
class Guard {
private:
    T &old_;
    T &new_;

public:
    Guard(T &hold, T &hnew) : old_(hold), new_(hnew) {
        std::swap(old_, new_);
    }

    ~Guard() {
        std::swap(old_, new_);
    }

    Guard(const Guard &) = delete;
    Guard &operator=(const Guard &) = delete;
    Guard(Guard &&) = delete;
    Guard &operator=(Guard &&) = delete;
};

class cADFFindData {
public:
    cADFFindData(AdfTypedList<AdfEntry>&& directory) : mDirectoryList(std::move(directory)), mCurrentEntry(mDirectoryList.begin()) {}

    AdfTypedList<AdfEntry> mDirectoryList;
    AdfTypedIterator<AdfEntry> mCurrentEntry;
    std::regex mFindMask;
};

class cADFPluginData {
    HANDLE                  mAbortEvent{};
    std::filesystem::path   mPath;
    int                     mLastError{};

    std::unique_ptr<AdfDevice, adf::AdfDeviceDeleter> mAdfDevice;
    std::unique_ptr<AdfVolume, adf::AdfVolumeDeleter> mAdfVolume;

private:
protected:
    LPVFSFILEDATAHEADER GetVFSforEntry(const AdfEntry *pEntry, HANDLE pHeap);
    void GetWfdForEntry(const AdfEntry &entry, LPWIN32_FIND_DATA pData);

    Guard<HANDLE> SetAbortHandle(HANDLE& hAbortEvent);
    bool ShouldAbort() const;

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

    int Delete(LPVOID func_data, std::wstring_view path, std::wstring_view pFile, bool pAll = false);

    cADFFindData *FindFirstFile(LPWSTR lpszPath, LPWIN32_FIND_DATA lpwfdData, HANDLE hAbortEvent);
    bool FindNextFile(cADFFindData* lpRAF, LPWIN32_FIND_DATA lpwfdData);
    void FindClose(cADFFindData* lpRAF);

    LPVFSFILEDATAHEADER GetfileInformation(std::wstring_view path, HANDLE heap);

    int Import(LPVOID func_data, std::wstring_view pFile, std::wstring_view pPath);
    int ImportFile(LPVOID func_data, std::wstring_view pFile, std::wstring_view pPath);
    int ImportPath(LPVOID func_data, std::wstring_view pFile, std::wstring_view pPath);

    bool Extract(LPVOID func_data, std::filesystem::path source_path, std::filesystem::path target_path);
    bool ExtractFile(LPVOID func_data, const AdfEntry& pEntry, std::filesystem::path target_path);
    bool ExtractPath(LPVOID func_data, std::filesystem::path source_path, std::filesystem::path target_path);
    bool ExtractEntries(LPVOID func_data, dopus::wstring_view_span entry_names, std::wstring_view target_path);

    int ContextVerb(LPVFSCONTEXTVERBDATAW lpVerbData);
    uint32_t BatchOperation(std::wstring_view lpszPath, LPVFSBATCHDATAW lpBatchData);
    bool PropGet(vfsProperty propId, LPVOID lpPropData, LPVOID lpData1, LPVOID lpData2, LPVOID lpData3);
    int GetLastError() const { return mLastError; }
};
