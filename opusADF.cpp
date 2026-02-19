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
    explicit AdfFindEntry(std::wstring_view name) : name(wstring_to_latin1(name)) {}

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

LPVFSFILEDATAHEADER cADFPluginData::GetfileInformation(std::wstring_view path, HANDLE heap) {
    mLastError = ERROR_FILE_NOT_FOUND;
    if (!AdfChangeToPath(path, true)) return nullptr;
    auto filename = file_from_path(path);
    auto directory = GetCurrentDirectoryList();
    auto iter = std::ranges::find_if(directory, AdfFindEntry(filename));
    if (iter == directory.end()) return nullptr;
    mLastError = 0;
    return GetVFSforEntry(&*iter, heap);
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
        if (!adfChangeDir(mAdfVolume.get(), entry->name)) return false;
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
    if (!AdfChangeToPath(pPath, true)) return nullptr;

    auto filename = file_from_path(pPath);
    return adfFileOpen(mAdfVolume.get(), wstring_to_latin1(filename).data(), ADF_FILE_MODE_READ);
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

    auto want_file_name = file_from_path(lpVerbData->lpszPath);
    auto directory = GetCurrentDirectoryList();

    auto iter = std::ranges::find_if(directory, AdfFindEntry(want_file_name));
    if (iter == directory.end()) return VFSCVRES_FAIL;
    if (iter->type == ADF_ST_FILE) return VFSCVRES_EXTRACT;
    return VFSCVRES_DEFAULT;
}

int cADFPluginData::Delete(LPVOID func_data, std::wstring_view path, std::wstring_view pFile, bool pAll) {
    int result = 0;
    DOpus.UpdateFunctionProgressBar(func_data, PROGRESSACTION_STATUSTEXT, (DWORD_PTR)"Deleting");
    auto adf_file_name = wstring_to_latin1(file_from_path(pFile));

    // TODO: magic numbers
    if (!AdfChangeToPath(path, true)) return 0;

    auto directory = GetCurrentDirectoryList();
    for (const AdfEntry& entry : directory) {
        // TODO: magic numbers
        if (ShouldAbort()) return 1;

        // Entry match?
        if (pAll || adf_file_name == entry.name) {
            {
                // Convert filename to show in progress bar
                auto filename = latin1_to_wstring(entry.name);
                DOpus.UpdateFunctionProgressBar(func_data, PROGRESSACTION_SETFILENAME, (DWORD_PTR)filename.data());
                DOpus.UpdateFunctionProgressBar(func_data, PROGRESSACTION_SETFILESIZE, (DWORD_PTR)entry.size);
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
                    DOpus.AddFunctionFileChange(func_data, true, OPUSFILECHANGE_REMDIR, Filename.c_str());
            }

            if (entry.type == ADF_ST_DIR)
            {

                Delete(func_data, Filename, Filepath, true);
                result |= adfRemoveEntry(mAdfVolume.get(), entry.parent, entry.name);
                AdfChangeToPath(path);
                if (!result)
                    DOpus.AddFunctionFileChange(func_data, true, OPUSFILECHANGE_REMDIR, Filename.c_str());
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

    if (iter == directory.end()) {
        mLastError = ERROR_NO_MORE_FILES;
        return false;
    }
    GetWfdForEntry(*iter, lpwfdData);
    ++iter;
    return true;
}

void cADFPluginData::FindClose(cADFFindData* pFindData) {
    delete pFindData;
}

int cADFPluginData::ImportFile(LPVOID func_data, std::wstring_view pFile, std::wstring_view pPath) {
    // TODO: Handle file read errors
    // TODO: Don't read the entire file into memory at once
    std::ifstream t(pPath.data(), std::ios::binary);
    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

    auto filename = file_from_path(pPath);

    DOpus.UpdateFunctionProgressBar(func_data, PROGRESSACTION_SETFILENAME, (DWORD_PTR)filename.data());
    DOpus.UpdateFunctionProgressBar(func_data, PROGRESSACTION_SETFILESIZE, (DWORD_PTR)buffer.size());


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

    DOpus.AddFunctionFileChange(func_data, true, OPUSFILECHANGE_CREATE, Final.c_str());

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

int cADFPluginData::ImportPath(LPVOID func_data, std::wstring_view pFile, std::wstring_view pPath) {
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
    DOpus.AddFunctionFileChange(func_data, true, OPUSFILECHANGE_MAKEDIR, Final.c_str());

    auto directory = GetCurrentDirectoryList();
    for (const AdfEntry& entry : directory) {
        if (adf_file_name != entry.name) continue;

        auto contents = directoryList(FinalPath + L"*.*");

        for (auto& file : contents) {
            if (file == L"." || file == L"..")
                continue;

            // TODO: perhaps can avoid creating strings here. Investigate use.
            std::wstring name = Final + L"\\" + file;
            Import(func_data, name, FinalPath + file);
        }
    }

    return 0;
}

int cADFPluginData::Import(LPVOID func_data, std::wstring_view pFile, std::wstring_view pPath) {
    // TODO: magic numbers
    if (!AdfChangeToPath(pFile, false)) return 0;

    if (ShouldAbort()) return 1;

    // is pPath a directory?
    auto Attribs = GetFileAttributes(pPath.data());

    if (Attribs & FILE_ATTRIBUTE_DIRECTORY)
        ImportPath(func_data, pFile, pPath);
    else
    {
        ImportFile(func_data, pFile, pPath);
    }

    // TODO: why 0? is that a success or failure? if success, then the AdfChangeToPath failure above should probably
    // return a different code, and if failure, then we should return a different code here too. Investigate.
    return 0;
}

bool cADFPluginData::Extract(LPVOID func_data, std::filesystem::path source_path, std::filesystem::path target_path) {
    DOpus.UpdateFunctionProgressBar(func_data, PROGRESSACTION_STATUSTEXT, (DWORD_PTR)"Copying");

    if (!AdfChangeToPath(source_path.wstring(), true)) return false;
    if (ShouldAbort()) return false;

    auto directory = GetCurrentDirectoryList();
    // If filename is empty, it means the path ended with a slash, so get the parent directory name instead.
    if (!source_path.has_filename()) source_path = source_path.parent_path();
    auto entry_name = source_path.filename().wstring();
    auto iter = std::ranges::find_if(directory, AdfFindEntry(entry_name));
    if (iter == directory.end()) return false;

    target_path.append(entry_name);

    DOpus.UpdateFunctionProgressBar(func_data, PROGRESSACTION_SETFILENAME, (DWORD_PTR)entry_name.data());
    DOpus.UpdateFunctionProgressBar(func_data, PROGRESSACTION_SETFILESIZE, (DWORD_PTR)iter->size);
    DOpus.AddFunctionFileChange(func_data, /* fIsDest= */true, OPUSFILECHANGE_CREATE, target_path.c_str());

    bool success;
    if (iter->type == ADF_ST_DIR) {
        std::filesystem::create_directories(target_path);
        success = ExtractPath(func_data, source_path, target_path);
    } else {
        success = ExtractFile(func_data, *iter, target_path);
    }

    DOpus.UpdateFunctionProgressBar(func_data, PROGRESSACTION_NEXTFILE, 0);
    return success;
}

bool cADFPluginData::ExtractPath(LPVOID func_data, std::filesystem::path source_path, std::filesystem::path target_path) {
    if (!AdfChangeToPath(source_path.wstring())) return false;

    auto directory = GetCurrentDirectoryList();
    bool success = true;
    for (const AdfEntry& entry : directory) {
        auto entry_name = latin1_to_wstring(entry.name);
        source_path.append(entry_name);
        success &= Extract(func_data, source_path.wstring(), target_path.wstring());
        source_path = source_path.parent_path();
    }

    return success;
}

bool cADFPluginData::ExtractFile(LPVOID func_data, const AdfEntry& pEntry, std::filesystem::path target_path) {
    std::vector<uint8_t> buffer(pEntry.size, 0);

    auto file = adfFileOpen(mAdfVolume.get(), pEntry.name, ADF_FILE_MODE_READ);
    if (!file) return false;

    auto n = adfFileRead(file, pEntry.size, buffer.data());
    if (!n) return false;

    std::ofstream ofile(target_path, std::ios::binary);
    ofile.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
    ofile.close();

    DOpus.UpdateFunctionProgressBar(func_data, PROGRESSACTION_STEPBYTES, (DWORD_PTR)pEntry.size);

    adfFileClose(file);
    return true;
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

Guard<HANDLE> cADFPluginData::SetAbortHandle(HANDLE& hAbortEvent) {
    return Guard<HANDLE>(mAbortEvent, hAbortEvent);
}

bool cADFPluginData::ShouldAbort() const {
    return mAbortEvent && WaitForSingleObject(mAbortEvent, 0) == WAIT_OBJECT_0;
}

uint32_t cADFPluginData::BatchOperation(std::wstring_view path, LPVFSBATCHDATAW lpBatchData) {
    auto abort_guard = SetAbortHandle(lpBatchData->hAbortEvent);
    memset(lpBatchData->piResults, 0, sizeof(int) * lpBatchData->iNumFiles);

    if (lpBatchData->uiOperation == VFSBATCHOP_EXTRACT) {
        // TODO: maybe pass piResults, too.
        return ExtractEntries(lpBatchData->lpFuncData, dopus::wstring_view_span(lpBatchData->pszFiles), lpBatchData->pszDestPath) ? VFSBATCHRES_COMPLETE : VFSBATCHRES_ABORT;
    }

    DOpus.UpdateFunctionProgressBar(lpBatchData->lpFuncData, PROGRESSACTION_SETPERCENT, (DWORD_PTR)0);

    dopus::wstring_view_span entry_names(lpBatchData->pszFiles);
    int index = 0;

    for (const auto& entry_path : entry_names) {
        // TODO: batch operation seems to rely on buildingblocks, like the ExtractEntries function.
        // Investigate the VFS API and see if we should adopt this here, too.
        if (entry_path.empty()) break;
        if (index >= lpBatchData->iNumFiles) break;

        if (lpBatchData->uiOperation == VFSBATCHOP_ADD) {
            lpBatchData->piResults[index] = Import(lpBatchData->lpFuncData, path, entry_path);
        }

        if (lpBatchData->uiOperation == VFSBATCHOP_DELETE) {
            lpBatchData->piResults[index] = Delete(lpBatchData->lpFuncData, path, entry_path);
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
    switch (propId) {
        case VFSPROP_ALLOWTOOLTIPGETSIZES:
        case VFSPROP_CANSHOWSUBFOLDERS:
        case VFSPROP_SHOWTHUMBNAILS:
        case VFSPROP_SUPPORTFILEHASH:
        case VFSPROP_SUPPORTPATHCOMPLETION:
        case VFSPROP_ISEXTRACTABLE:
            *((LPBOOL)lpPropData) = true;
            break;

        case VFSPROP_CANDELETESECURE:
        case VFSPROP_CANDELETETOTRASH:
        case VFSPROP_SHOWFILEINFO:
        case VFSPROP_USEFULLRENAME:
            *((LPBOOL)lpPropData) = false;
            break;

        case VFSPROP_SHOWPICTURESDIRECTLY:
        case VFSPROP_SHOWFULLPROGRESSBAR: // No progress bar even when copying.
            *((LPDWORD)lpPropData) = false;
            break;

        case VFSPROP_DRAGEFFECTS:
            *((LPDWORD)lpPropData) = DROPEFFECT_COPY;
            break;

        case VFSPROP_BATCHOPERATION:
            *((LPDWORD)lpPropData) = VFSBATCHRES_HANDLED;
            break;

        case VFSPROP_GETVALIDACTIONS:
            *((LPDWORD)lpPropData) = /* SFGAO_*/ 0;
            break;

        case VFSPROP_COPYBUFFERSIZE:
            *((LPDWORD)lpPropData) = 64 << 10;
            break;

        case VFSPROP_FUNCAVAILABILITY:
            *((LPDWORD)lpPropData) &= ~(VFSFUNCAVAIL_MOVE | VFSFUNCAVAIL_DELETE
                                        // | VFSFUNCAVAIL_GETSIZES
                                        | VFSFUNCAVAIL_MAKEDIR | VFSFUNCAVAIL_PRINT | VFSFUNCAVAIL_PROPERTIES | VFSFUNCAVAIL_RENAME | VFSFUNCAVAIL_SETATTR | VFSFUNCAVAIL_SHORTCUT
                                        //| VFSFUNCAVAIL_SELECTALL
                                        //| VFSFUNCAVAIL_SELECTNONE
                                        //| VFSFUNCAVAIL_SELECTINVERT
                                        | VFSFUNCAVAIL_VIEWLARGEICONS | VFSFUNCAVAIL_VIEWSMALLICONS
                                        //| VFSFUNCAVAIL_VIEWLIST
                                        | VFSFUNCAVAIL_VIEWDETAILS
                                        | VFSFUNCAVAIL_VIEWTHUMBNAIL | VFSFUNCAVAIL_CLIPCOPY | VFSFUNCAVAIL_CLIPCUT | VFSFUNCAVAIL_CLIPPASTE | VFSFUNCAVAIL_CLIPPASTESHORTCUT | VFSFUNCAVAIL_UNDO
                                        //| VFSFUNCAVAIL_SHOW
                                        | VFSFUNCAVAIL_DUPLICATE | VFSFUNCAVAIL_SPLITJOIN
                                        //| VFSFUNCAVAIL_SELECTRESELECT
                                        //| VFSFUNCAVAIL_SELECTALLFILES
                                        //| VFSFUNCAVAIL_SELECTALLDIRS
                                        //| VFSFUNCAVAIL_PLAY
                                        | VFSFUNCAVAIL_SETTIME | VFSFUNCAVAIL_VIEWTILE | VFSFUNCAVAIL_SETCOMMENT);
            break;

            // VFSPROP_GETFOLDERICON -> return icon file?
        default:
            return false;
    }

    return true;
}

bool cADFPluginData::ExtractEntries(LPVOID func_data, dopus::wstring_view_span entry_names, std::wstring_view target_path) {
    for (const auto& entry_path : entry_names) {
        if (entry_path.empty()) break;
        if (!Extract(func_data, entry_path, target_path)) return false;
    }

    return true;
}