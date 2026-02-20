#pragma once

#include <filesystem>
#include <optional>

#include "adf_typed_list.hh"
#include "dopus_wstring_view_span.hh"


namespace adf {
struct AdfDeviceDeleter {
  void operator()(AdfDevice* device) const;
};

struct AdfVolumeDeleter {
  void operator()(AdfVolume* volume) const;
};
}  // namespace adf

/// @brief Guard object to set and restore fields.
template <typename T>
class Guard {
 private:
  T& old_;
  T& new_;

 public:
  Guard(T& hold, T& hnew) : old_(hold), new_(hnew) { std::swap(old_, new_); }

  ~Guard() { std::swap(old_, new_); }

  Guard(const Guard&) = delete;
  Guard& operator=(const Guard&) = delete;
  Guard(Guard&&) = delete;
  Guard& operator=(Guard&&) = delete;
};

class cADFFindData {
 public:
  cADFFindData(AdfTypedList<AdfEntry>&& directory)
      : mDirectoryList(std::move(directory)), mCurrentEntry(mDirectoryList.begin()) {}

  AdfTypedList<AdfEntry> mDirectoryList;
  AdfTypedIterator<AdfEntry> mCurrentEntry;
  std::optional<std::regex> mPattern;
};

class cADFPluginData {
  HANDLE mAbortEvent{};
  std::filesystem::path mPath;
  int mLastError{};

  std::unique_ptr<AdfDevice, adf::AdfDeviceDeleter> mAdfDevice;
  std::unique_ptr<AdfVolume, adf::AdfVolumeDeleter> mAdfVolume;

 private:
 protected:
  LPVFSFILEDATAHEADER GetVFSforEntry(const AdfEntry* pEntry, HANDLE pHeap);
  void GetWfdForEntry(const AdfEntry& entry, LPWIN32_FIND_DATA pData);

  Guard<HANDLE> SetAbortHandle(HANDLE& hAbortEvent);
  bool ShouldAbort() const;

  FILETIME GetFileTime(const AdfEntry& entry);
  void SetEntryTime(AdfFile* pFile, FILETIME pFT);

  std::optional<std::filesystem::path> LoadFile(const std::filesystem::path& pAfPath);
  AdfTypedList<AdfEntry> GetCurrentDirectoryList();

 public:
  bool AdfChangeToPath(std::filesystem::path path, bool pIgnoreLast = false);

  bool ReadDirectory(LPVFSREADDIRDATAW lpRDD);
  bool ReadFile(AdfFile* pFile, std::span<uint8_t> buffer, LPDWORD readSize);

  AdfFile* OpenFile(std::filesystem::path path);
  void CloseFile(AdfFile* pFile);

  size_t GetAvailableSize();
  size_t GetTotalSize();

  int Delete(LPVOID func_data, std::filesystem::path path, std::filesystem::path pFile, bool pAll = false);

  cADFFindData* FindFirstFile(std::filesystem::path path, LPWIN32_FIND_DATA lpwfdData, HANDLE hAbortEvent);
  bool FindNextFile(cADFFindData* lpRAF, LPWIN32_FIND_DATA lpwfdData);
  void FindClose(cADFFindData* lpRAF);

  LPVFSFILEDATAHEADER GetfileInformation(std::filesystem::path path, HANDLE heap);

  int Import(LPVOID func_data, std::filesystem::path file, std::filesystem::path path);
  int ImportFile(LPVOID func_data, std::filesystem::path file, std::filesystem::path path);
  int ImportPath(LPVOID func_data, std::filesystem::path file, std::filesystem::path path);

  bool Extract(LPVOID func_data, std::filesystem::path source_path, std::filesystem::path target_path);
  bool ExtractFile(LPVOID func_data, const AdfEntry& pEntry, std::filesystem::path target_path);
  bool ExtractPath(LPVOID func_data, std::filesystem::path source_path, std::filesystem::path target_path);
  bool ExtractEntries(LPVOID func_data, dopus::wstring_view_span entry_names, std::filesystem::path target_path);

  int ContextVerb(LPVFSCONTEXTVERBDATAW lpVerbData);
  uint32_t BatchOperation(std::filesystem::path lpszPath, LPVFSBATCHDATAW lpBatchData);
  bool PropGet(vfsProperty propId, LPVOID lpPropData, LPVOID lpData1, LPVOID lpData2, LPVOID lpData3);
  int GetLastError() const { return mLastError; }
};
