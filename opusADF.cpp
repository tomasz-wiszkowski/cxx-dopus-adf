// opusADF.cpp : Defines the exported functions for the DLL application.
//

#include <filesystem>
#include <strsafe.h>
#include <cwctype>

#include "dopus_wstring_view_span.hh"
#include "stdafx.h"
#include "text_utils.hh"

namespace {
/// @brief Predicate to check if an AdfEntry is a supported type (file or directory).
/// @param type The AdfEntry type to check.
/// @return Whether the type is supported.
constexpr bool IsSupportedEntryType(int type) {
    return type == ADF_ST_FILE || type == ADF_ST_DIR;
}

/// @brief Predicate for finding an AdfEntry by name in a directory listing.
struct AdfFindEntry {
    const std::string name;
    AdfFindEntry(std::wstring_view name) : name(wstring_to_latin1(name)) {}

    bool operator()(const AdfEntry& entry) const {
        return IsSupportedEntryType(entry.type) && name == entry.name;
    }
};
}

struct DateTime {
	int year, mon, day, hour, min, sec;
};

DOpusPluginHelperFunction DOpus;

bool adfIsLeap(int y) {
    return !(y % 100) ? !(y % 400) : !(y % 4);
}

void adfTime2AmigaTime(struct DateTime dt, int32_t *day, int32_t *min, int32_t *ticks) {
    int jm[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };


    *min = dt.hour * 60 + dt.min;                /* mins */
    *ticks = dt.sec * 50;                        /* ticks */

                                                 /*--- days ---*/

    *day = dt.day;                         /* current month days */

                                               /* previous months days downto january */
    if (dt.mon > 1) {                      /* if previous month exists */
        dt.mon--;
        if (dt.mon > 2 && adfIsLeap(dt.year))    /* months after a leap february */
            jm[2 - 1] = 29;
        while (dt.mon > 0) {
            *day = *day + jm[dt.mon - 1];
            dt.mon--;
        }
    }

    /* years days before current year downto 1978 */
    if (dt.year > 78) {
        dt.year--;
        while (dt.year >= 78) {
            if (adfIsLeap(dt.year))
                *day = *day + 366;
            else
                *day = *day + 365;
            dt.year--;
        }
    }
}

FILETIME cADFPluginData::GetFileTime(const AdfEntry &entry) {
    SYSTEMTIME AmigaTime, TZAdjusted;
    AmigaTime.wDayOfWeek = 0;
    AmigaTime.wMilliseconds = 0;
    AmigaTime.wYear = static_cast<WORD>(entry.year);
    AmigaTime.wMonth = static_cast<WORD>(entry.month);
    AmigaTime.wDay = static_cast<WORD>(entry.days);
    AmigaTime.wHour = static_cast<WORD>(entry.hour);
    AmigaTime.wMinute = static_cast<WORD>(entry.mins);
    AmigaTime.wSecond = static_cast<WORD>(entry.secs);
    TzSpecificLocalTimeToSystemTime(nullptr, &AmigaTime, &TZAdjusted);
    FILETIME ft;
    SystemTimeToFileTime(&TZAdjusted, &ft);
    return ft;
}

DateTime ToDateTime(FILETIME pFT) {
    SYSTEMTIME AmigaTime, TZAdjusted;
    FileTimeToSystemTime(&pFT, &AmigaTime);
    SystemTimeToTzSpecificLocalTime(nullptr, &AmigaTime, &TZAdjusted);

    DateTime dt;
    dt.year = TZAdjusted.wYear - 1900;
    dt.mon = TZAdjusted.wMonth;
    dt.day = TZAdjusted.wDay;
    dt.hour = TZAdjusted.wHour;
    dt.min = TZAdjusted.wMinute;
    dt.sec = TZAdjusted.wSecond;
    return dt;
}

void cADFPluginData::SetEntryTime(AdfFile *pFile, FILETIME pFT) {
    DateTime dt = ToDateTime(pFT);
    adfTime2AmigaTime(dt, &(pFile->fileHdr->days), &(pFile->fileHdr->mins), &(pFile->fileHdr->ticks));
}

LPVFSFILEDATAHEADER cADFPluginData::GetVFSforEntry(const AdfEntry *pEntry, HANDLE pHeap) {
    LPVFSFILEDATAHEADER lpFDH;

    lpFDH = (LPVFSFILEDATAHEADER)HeapAlloc(pHeap, 0, sizeof(VFSFILEDATAHEADER) + sizeof(VFSFILEDATA));
    if (!lpFDH) return nullptr;

    LPVFSFILEDATA lpFD = (LPVFSFILEDATA)(lpFDH + 1);

    lpFDH->cbSize = sizeof(VFSFILEDATAHEADER);
    lpFDH->lpNext = 0;
    lpFDH->iNumItems = 1;
    lpFDH->cbFileDataSize = sizeof(VFSFILEDATA);

    lpFD->dwFlags = 0;
    lpFD->lpszComment = 0;
    lpFD->iNumColumns = 0;
    lpFD->lpvfsColumnData = 0;

    GetWfdForEntry(*pEntry, &lpFD->wfdData);

    return lpFDH;
}

void cADFPluginData::GetWfdForEntry(const AdfEntry &entry, LPWIN32_FIND_DATA pData) {
    auto filename = latin1_to_wstring(entry.name);
    StringCchCopyW(pData->cFileName, MAX_PATH, filename.c_str());

    pData->nFileSizeHigh = 0;
    pData->nFileSizeLow = entry.size;

    if (entry.type == ADF_ST_DIR)
        pData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    else
        pData->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;

    pData->dwReserved0 = 0;
    pData->dwReserved1 = 0;

    pData->ftCreationTime = {};
    pData->ftLastAccessTime = {};
    pData->ftLastWriteTime = GetFileTime(entry);
}

bool cADFPluginData::AdfChangeToPath(std::wstring_view pPath, bool pIgnoreLast) {
    if (!LoadFile(pPath)) return false;
    if (!mAdfVolume) return false;

    adfToRootDir(mAdfVolume.get());

    auto components = split_path_to_components(pPath, mPath.wstring());
    if (components.empty()) return !pIgnoreLast; // Root directory cannot ignore last component.
    if (pIgnoreLast) components.pop_back();

    for (auto& component : components) {
        auto directory = GetCurrentDirectoryList();
        auto entry = std::ranges::find_if(directory, AdfFindEntry(component));

        // No matching entry found for this component, path is invalid.
        if (entry == directory.end()) return false;
        adfChangeDir(mAdfVolume.get(), entry->name);
    }

    return true;
}

namespace adf {
void AdfDeviceDeleter::operator()(AdfDevice* device) const {
    if (!device) return;
    adfDevUnMount(device);
    adfDevClose(device);
}

void AdfVolumeDeleter::operator()(AdfVolume* volume) const {
    if (!volume) return;
    adfVolUnMount(volume);
}
}

// TODO: migrate all to std::filesystem::path.
bool cADFPluginData::LoadFile(std::wstring_view pAdfPath) {
    if (!mPath.empty() && pAdfPath.starts_with(mPath.wstring())) {
        // Path is already loaded, no need to check again.
        return true;
    }

    // Loading new file. Should we cache this?
    mPath.clear();
    mAdfVolume.reset();
    mAdfDevice.reset();

    // Walk the pAdfPath up until we find the valid file.
    std::filesystem::path real_file_path = pAdfPath;
    while (!real_file_path.empty()) {
        if (std::filesystem::exists(real_file_path)) break;
        real_file_path = real_file_path.parent_path();
    }
    if (real_file_path.empty()) return false;

    // Get extension and check if it's supported.
    auto extension = real_file_path.extension().wstring();
    std::ranges::transform(extension, extension.begin(), std::towlower);
    if (extension != L".adf" && extension != L".hdf") return false;

    auto utf_file_path = real_file_path.string();

    mAdfDevice.reset(adfDevOpen(utf_file_path.c_str(), ADF_ACCESS_MODE_READONLY));
    if (!mAdfDevice) return false;
    adfDevMount(mAdfDevice.get());

    mAdfVolume.reset(adfVolMount(mAdfDevice.get(), 0, ADF_ACCESS_MODE_READONLY));
    if (!mAdfVolume) {
        mAdfDevice.reset();
        return false;
    }

    // Supported extension, path is valid. Accept.
    mPath = real_file_path;

    return true;
}

bool cADFPluginData::ReadDirectory(LPVFSREADDIRDATAW lpRDD) {
    // Free directory if lister is closing (otherwise ignore free command)
    if (lpRDD->vfsReadOp == VFSREAD_FREEDIRCLOSE)
        return true;

    if (lpRDD->vfsReadOp == VFSREAD_FREEDIR)
        return true;

    // Do nothing if we have no path
    if (!lpRDD->lpszPath || !*lpRDD->lpszPath) {
        mLastError = ERROR_PATH_NOT_FOUND;
        return false;
    }

    if (!AdfChangeToPath(lpRDD->lpszPath))
        return false;

    if (lpRDD->vfsReadOp == VFSREAD_CHANGEDIR)
        return true;

    auto directory = GetCurrentDirectoryList();

    LPVFSFILEDATAHEADER lpLastHeader = 0;

    for (const AdfEntry& entry : directory) {
        if (!IsSupportedEntryType(entry.type)) continue; 
        LPVFSFILEDATAHEADER lpFDH = GetVFSforEntry(&entry, lpRDD->hMemHeap);

        // Add the entries for this folder
        if (lpFDH) {
            if (lpLastHeader)
                lpLastHeader->lpNext = lpFDH;
            else
                lpRDD->lpFileData = lpFDH;

            lpLastHeader = lpFDH;
        }
    }

    return true;
}

bool cADFPluginData::ReadFile(AdfFile* pFile, std::span<uint8_t> buffer, LPDWORD pReadSize) {
    if (!pFile) return false;
    *pReadSize = adfFileRead(pFile, (int32_t)buffer.size(), buffer.data());
    return *pReadSize > 0;
}

AdfFile* cADFPluginData::OpenFile(std::wstring pPath) {
    if (!AdfChangeToPath(pPath)) return nullptr;

    auto filename = wstring_to_latin1(file_from_path(pPath));
    return adfFileOpen(mAdfVolume.get(), filename.c_str(), ADF_FILE_MODE_READ);
}

void cADFPluginData::CloseFile(AdfFile* pFile) {
    if (!pFile) return;

    adfFileClose(pFile);
}

AdfTypedList<AdfEntry> cADFPluginData::GetCurrentDirectoryList() {
    return AdfTypedList<AdfEntry>(
            adfGetDirEnt(mAdfVolume.get(), mAdfVolume->curDirPtr),
            adfFreeDirList);
}

int cADFPluginData::ContextVerb(LPVFSCONTEXTVERBDATAW lpVerbData) {
    if (!AdfChangeToPath(lpVerbData->lpszPath, true)) return VFSCVRES_FAIL;

    auto want_file_name = wstring_to_latin1(file_from_path(lpVerbData->lpszPath));
    auto directory = GetCurrentDirectoryList();

    for (const AdfEntry& entry : directory) {
        if (want_file_name != entry.name) continue;

        if (entry.type == ADF_ST_FILE) return VFSCVRES_EXTRACT;
        if (entry.type == ADF_ST_DIR) return VFSCVRES_DEFAULT;
    }

    return VFSCVRES_FAIL;
}

int cADFPluginData::Delete(LPVFSBATCHDATAW lpBatchData, std::wstring_view path, std::wstring_view pFile, bool pAll) {
    int result = 0;
    DOpus.UpdateFunctionProgressBar(lpBatchData->lpFuncData, PROGRESSACTION_STATUSTEXT, (DWORD_PTR)"Deleting");
    auto adf_file_name = wstring_to_latin1(file_from_path(pFile));

    // TODO: magic numbers
    if (!AdfChangeToPath(path)) return 0;

    auto directory = GetCurrentDirectoryList();
    for (const AdfEntry& entry : directory) {
        // TODO: magic numbers
        if (lpBatchData->hAbortEvent && WaitForSingleObject(lpBatchData->hAbortEvent, 0) == WAIT_OBJECT_0) return 1;

        // Entry match?
        if (pAll || adf_file_name == entry.name) {
            {
                // Convert filename to show in progress bar
                auto filename = latin1_to_wstring(entry.name);
                DOpus.UpdateFunctionProgressBar(lpBatchData->lpFuncData, PROGRESSACTION_SETFILENAME, (DWORD_PTR)filename.data());
                DOpus.UpdateFunctionProgressBar(lpBatchData->lpFuncData, PROGRESSACTION_SETFILESIZE, (DWORD_PTR)entry.size);
            }

            // TODO: perhaps can avoid creating strings here. Not sure why that is needed.
            std::wstring Filename(path);

            if (Filename[Filename.size() - 1] != '\\')
                Filename += L"\\";
            auto Filepath = latin1_to_wstring(entry.name);
            Filename += Filepath;

            if (entry.type == ADF_ST_FILE)
            {
                result |= adfRemoveEntry(mAdfVolume.get(), entry.parent, entry.name);
                if (!result)
                    DOpus.AddFunctionFileChange(lpBatchData->lpFuncData, true, OPUSFILECHANGE_REMDIR, Filename.c_str());
            }

            if (entry.type == ADF_ST_DIR)
            {

                Delete(lpBatchData, Filename, Filepath, true);
                result |= adfRemoveEntry(mAdfVolume.get(), entry.parent, entry.name);
                AdfChangeToPath(path);
                if (!result)
                    DOpus.AddFunctionFileChange(lpBatchData->lpFuncData, true, OPUSFILECHANGE_REMDIR, Filename.c_str());
            }
        }
    }

    return 0;
}

cADFFindData* cADFPluginData::FindFirstFile(LPWSTR lpszPath, LPWIN32_FIND_DATA lpwfdData, HANDLE hAbortEvent) {
    if (!AdfChangeToPath(lpszPath, true)) return nullptr;

    // TODO: this is incorrect. This tries to operate on Latin1 characters, but should instead operate on the wide strings.
    auto Filepath = wstring_to_latin1(file_from_path(lpszPath));
    auto find_data = std::make_unique<cADFFindData>(GetCurrentDirectoryList());
    find_data->mFindMask = std::regex("(." + Filepath + ")");

    if (!FindNextFile(find_data.get(), lpwfdData)) find_data.reset();

    return find_data.release();
}

bool cADFPluginData::FindNextFile(cADFFindData* pFindData, LPWIN32_FIND_DATA lpwfdData) {
    const auto& directory = pFindData->mDirectoryList;
    auto& iter = pFindData->mCurrentEntry;
    const auto& pattern = pFindData->mFindMask;

    for (; iter != directory.end(); ++iter) {
        // TODO: this is incorrect. This tries to operate on Latin1 characters, but should instead operate on the wide strings.
        if (IsSupportedEntryType(iter->type) && std::regex_match(iter->name, pattern)) break;
    }

    if (iter == directory.end()) return false;
    GetWfdForEntry(*iter, lpwfdData);
    ++iter;
    return true;
}

void cADFPluginData::FindClose(cADFFindData* pFindData) {
    delete pFindData;
}

int cADFPluginData::ImportFile(LPVFSBATCHDATAW lpBatchData, std::wstring_view pFile, std::wstring_view pPath) {
    // TODO: Handle file read errors
    // TODO: Don't read the entire file into memory at once
    std::ifstream t(pPath.data(), std::ios::binary);
    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

    auto filename = file_from_path(pPath);

    DOpus.UpdateFunctionProgressBar(lpBatchData->lpFuncData, PROGRESSACTION_SETFILENAME, (DWORD_PTR)filename.data());
    DOpus.UpdateFunctionProgressBar(lpBatchData->lpFuncData, PROGRESSACTION_SETFILESIZE, (DWORD_PTR)buffer.size());


    FILETIME ft;
    {
        HANDLE file = CreateFile(pPath.data(), FILE_READ_ATTRIBUTES, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        ::GetFileTime(file, 0, 0, &ft);
        CloseHandle(file);
    }

    auto file = adfFileOpen(mAdfVolume.get(), wstring_to_latin1(filename).data(), ADF_FILE_MODE_WRITE);
    if (!file)
        return 1;

    SetEntryTime(file, ft);
    adfFileWrite(file, (int32_t) buffer.size(), buffer.data());
    adfFileClose(file);

    std::wstring Final(pFile);
    if (Final[Final.size() - 1] != L'\\')
        Final += L"\\";
    Final += filename;

    DOpus.AddFunctionFileChange(lpBatchData->lpFuncData, true, OPUSFILECHANGE_CREATE, Final.c_str());

    return 0;
}

std::vector<std::wstring> directoryList(const std::wstring pPath) {
    WIN32_FIND_DATA fdata;
    HANDLE dhandle;
    std::vector<std::wstring> results;

    if ((dhandle = ::FindFirstFile(pPath.c_str(), &fdata)) == INVALID_HANDLE_VALUE)
        return results;

    results.push_back(std::wstring(fdata.cFileName));

    while (1) {
        if (::FindNextFile(dhandle, &fdata)) {
            results.push_back(std::wstring(fdata.cFileName));
        }
        else {
            if (GetLastError() == ERROR_NO_MORE_FILES) {
                break;
            }
            else {
                FindClose(dhandle);
                return results;
            }
        }
    }

    FindClose(dhandle);
    return results;
}

int cADFPluginData::ImportPath(LPVFSBATCHDATAW lpBatchData, std::wstring_view pFile, std::wstring_view pPath) {
    std::wstring FinalPath(pPath);
    if (FinalPath[FinalPath.size() - 1] != L'\\')
        FinalPath += L"\\";

    auto sys_file_name = file_from_path(pPath);
    auto adf_file_name = wstring_to_latin1(sys_file_name);
    std::wstring Final(FinalPath);
    Final += sys_file_name;

    // TODO: Dead code below -> dt not consumed, investigate api?
    FILETIME ft;
    {
        HANDLE file = CreateFile(pPath.data(), FILE_READ_ATTRIBUTES, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        ::GetFileTime(file, 0, 0, &ft);
        CloseHandle(file);
    }
    DateTime dt = ToDateTime(ft);

    // TODO: check return codes?
    adfCreateDir(mAdfVolume.get(), mAdfVolume->curDirPtr, adf_file_name.data() /*, dt */);
    DOpus.AddFunctionFileChange(lpBatchData->lpFuncData, true, OPUSFILECHANGE_MAKEDIR, Final.c_str());

    auto directory = GetCurrentDirectoryList();
    for (const AdfEntry& entry : directory) {
        if (adf_file_name != entry.name) continue;

        auto contents = directoryList(FinalPath + L"*.*");

        for (auto& file : contents) {
            if (file == L"." || file == L"..")
                continue;

            // TODO: perhaps can avoid creating strings here. Investigate use.
            std::wstring name = Final + L"\\" + file;
            Import(lpBatchData, name, FinalPath + file);
        }
    }

    return 0;
}

int cADFPluginData::Import(LPVFSBATCHDATAW lpBatchData, std::wstring_view pFile, std::wstring_view pPath) {
    // TODO: magic numbers
    if (!AdfChangeToPath(pFile, false)) return 0;

    if (lpBatchData->hAbortEvent && WaitForSingleObject(lpBatchData->hAbortEvent, 0) == WAIT_OBJECT_0)
    {
        return 1;
    }

    // is pPath a directory?
    auto Attribs = GetFileAttributes(pPath.data());

    if (Attribs & FILE_ATTRIBUTE_DIRECTORY)
        ImportPath(lpBatchData, pFile, pPath);
    else
    {
        ImportFile(lpBatchData, pFile, pPath);
    }

    // TODO: why 0? is that a success or failure? if success, then the AdfChangeToPath failure above should probably
    // return a different code, and if failure, then we should return a different code here too. Investigate.
    return 0;
}

int cADFPluginData::Extract(LPVFSBATCHDATAW lpBatchData, std::wstring_view pFile, std::wstring_view pDest) {
    DOpus.UpdateFunctionProgressBar(lpBatchData->lpFuncData, PROGRESSACTION_STATUSTEXT, (DWORD_PTR)"Copying");

    // TODO: what does `1` signify? Should we return different error codes for different failure cases?
    int result = 1;
    if (!AdfChangeToPath(pFile, true)) return result;

    auto want_file_name = file_from_path(pFile);

    auto directory = GetCurrentDirectoryList();
    for (const AdfEntry& entry : directory) {
        if (lpBatchData->hAbortEvent && WaitForSingleObject(lpBatchData->hAbortEvent, 0) == WAIT_OBJECT_0) {
            return 1;
        }

        std::wstring FinalName(pDest);
        if (FinalName[FinalName.size() - 1] != '\\') {
            FinalName += '\\';
        }
        auto Filename = latin1_to_wstring(entry.name);
        FinalName += Filename;

        // Entry match?
        if (Filename == want_file_name) {

            DOpus.UpdateFunctionProgressBar(lpBatchData->lpFuncData, PROGRESSACTION_SETFILENAME, (DWORD_PTR)entry.name);
            DOpus.UpdateFunctionProgressBar(lpBatchData->lpFuncData, PROGRESSACTION_SETFILESIZE, (DWORD_PTR)entry.size);

            // Create a directory, or extract a file?
            if (entry.type == ADF_ST_DIR) {
                CreateDirectory(FinalName.c_str(), 0);
                result = ExtractPath(lpBatchData, pFile, FinalName);
                DOpus.AddFunctionFileChange(lpBatchData->lpFuncData, false, OPUSFILECHANGE_CREATE, lpBatchData->pszDestPath);

                if (!result) {
                    HANDLE filename = CreateFile(FinalName.c_str(), FILE_WRITE_ATTRIBUTES, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
                    auto entryTime = GetFileTime(entry);
                    SetFileTime(filename, 0, 0, &entryTime);
                    CloseHandle(filename);
                }
            }
            else {
                result = ExtractFile(lpBatchData, &entry, FinalName);
            }

            DOpus.UpdateFunctionProgressBar(lpBatchData->lpFuncData, PROGRESSACTION_NEXTFILE, 0);
            break;
        }
    }

    return result;
}

int cADFPluginData::ExtractPath(LPVFSBATCHDATAW lpBatchData, std::wstring_view pPath, std::wstring_view pDest) {
    int result = 0;

    if (!AdfChangeToPath(pPath)) return result;

    auto directory = GetCurrentDirectoryList();
    for (const AdfEntry& entry : directory) {
        std::wstring FinalPath(pPath);
        if (FinalPath[FinalPath.size() - 1] != '\\')
        {
            FinalPath += '\\';
        }
        auto Filepath = latin1_to_wstring(entry.name);
        FinalPath += Filepath;

        result |= Extract(lpBatchData, FinalPath, pDest);
    }

    return result;
}

int cADFPluginData::ExtractFile(LPVFSBATCHDATAW lpBatchData, const AdfEntry* pEntry, const std::wstring& pDest) {
    std::string buffer(pEntry->size, 0);

    // TODO: what do `1` and `2` signify? Should we return different error codes for different failure cases?
    auto file = adfFileOpen(mAdfVolume.get(), pEntry->name, ADF_FILE_MODE_READ);
    if (!file) return 1;

    auto n = adfFileRead(file, pEntry->size, reinterpret_cast<uint8_t*>(&buffer[0]));
    if (!n) return 2;

    std::ofstream ofile(pDest, std::ios::binary);
    ofile.write(buffer.c_str(), buffer.size());
    ofile.close();

    DOpus.UpdateFunctionProgressBar(lpBatchData->lpFuncData, PROGRESSACTION_STEPBYTES, (DWORD_PTR)pEntry->size);

    HANDLE filename = CreateFile(pDest.c_str(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	auto entryTime = GetFileTime(*pEntry);
    SetFileTime(filename, 0, 0, &entryTime);
    CloseHandle(filename);

    adfFileClose(file);
    DOpus.AddFunctionFileChange(lpBatchData->lpFuncData, false, OPUSFILECHANGE_CREATE, lpBatchData->pszDestPath);
    return 0;
}

size_t cADFPluginData::GetAvailableSize(const std::wstring& pFile) {
    // TODO: do we need to load the file here? seems like we should already have it loaded if we're doing an operation on it, but maybe not?
    if (!LoadFile(pFile)) return 0;

    size_t blocks = adfCountFreeBlocks(mAdfVolume.get());
    blocks *= mAdfVolume->datablockSize;

    return blocks;
}

size_t cADFPluginData::GetTotalSize(const std::wstring& pFile) {
    return mAdfDevice->sizeBlocks;
}

uint32_t cADFPluginData::BatchOperation(std::wstring_view path, LPVFSBATCHDATAW lpBatchData) {
    DOpus.UpdateFunctionProgressBar(lpBatchData->lpFuncData, PROGRESSACTION_SETPERCENT, (DWORD_PTR)0);

    dopus::wstring_view_span files(lpBatchData->pszFiles);
    int index = 0;

    for (const auto& file : files) {
        if (file.empty()) break;
        if (index >= lpBatchData->iNumFiles) break;

        if (lpBatchData->uiOperation == VFSBATCHOP_EXTRACT) {
            lpBatchData->piResults[index] = Extract(lpBatchData, file, lpBatchData->pszDestPath);
        }

        if (lpBatchData->uiOperation == VFSBATCHOP_ADD) {
            lpBatchData->piResults[index] = Import(lpBatchData, path, file);
        }

        if (lpBatchData->uiOperation == VFSBATCHOP_DELETE) {
            lpBatchData->piResults[index] = Delete(lpBatchData, path, file);
        }

        // Abort on failure.
        if (lpBatchData->piResults[index]) {
            return VFSBATCHRES_ABORT;
        }

        ++index;
    }

    return VFSBATCHRES_COMPLETE;
}

bool cADFPluginData::PropGet(vfsProperty propId, LPVOID lpPropData, LPVOID lpData1, LPVOID lpData2, LPVOID lpData3) {

    if (propId == VFSPROP_SHOWTHUMBNAILS) {

        *(bool*)lpPropData = true;

        return true;
    }

    if (propId == VFSPROP_FUNCAVAILABILITY) {
        unsigned __int64* Data = (unsigned __int64*)lpPropData;

        *Data &= ~(VFSFUNCAVAIL_DELETE | VFSFUNCAVAIL_MAKEDIR | VFSFUNCAVAIL_RENAME | VFSFUNCAVAIL_SETATTR | VFSFUNCAVAIL_CLIPCUT | VFSFUNCAVAIL_CLIPPASTE | VFSFUNCAVAIL_CLIPPASTESHORTCUT | VFSFUNCAVAIL_DUPLICATE);
        return true;
    }

    return false;
}