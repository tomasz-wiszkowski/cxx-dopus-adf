// opusADF.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include <strsafe.h>

#include "text_utils.hh"

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

cADFPluginData::cADFPluginData() {
    mAdfDevice = 0;
    mAdfVolume = 0;
}

cADFPluginData::~cADFPluginData() {

}

FILETIME cADFPluginData::GetFileTime(const AdfEntry *pEntry) {
    SYSTEMTIME AmigaTime, TZAdjusted;
    AmigaTime.wDayOfWeek = 0;
    AmigaTime.wMilliseconds = 0;
    AmigaTime.wYear = pEntry->year;
    AmigaTime.wMonth = pEntry->month;
    AmigaTime.wDay = pEntry->days;
    AmigaTime.wHour = pEntry->hour;
    AmigaTime.wMinute = pEntry->mins;
    AmigaTime.wSecond = pEntry->secs;
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

    if (lpFDH = (LPVFSFILEDATAHEADER)HeapAlloc(pHeap, 0, sizeof(VFSFILEDATAHEADER) + sizeof(VFSFILEDATA))) {
        LPVFSFILEDATA lpFD = (LPVFSFILEDATA)(lpFDH + 1);

        lpFDH->cbSize = sizeof(VFSFILEDATAHEADER);
        lpFDH->lpNext = 0;
        lpFDH->iNumItems = 1;
        lpFDH->cbFileDataSize = sizeof(VFSFILEDATA);

        lpFD->dwFlags = 0;
        lpFD->lpszComment = 0;
        lpFD->iNumColumns = 0;
        lpFD->lpvfsColumnData = 0;

        GetWfdForEntry(pEntry, &lpFD->wfdData);
    }

    return lpFDH;
}

void cADFPluginData::GetWfdForEntry(const AdfEntry *pEntry, LPWIN32_FIND_DATA pData) {
    auto filename = latin1_to_wstring(pEntry->name);
    StringCchCopyW(pData->cFileName, MAX_PATH, filename.c_str());

    pData->nFileSizeHigh = 0;
    pData->nFileSizeLow = pEntry->size;

    if (pEntry->type == ADF_ST_DIR)
        pData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    else
        pData->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;

    pData->dwReserved0 = 0;
    pData->dwReserved1 = 0;

    pData->ftCreationTime = {};
    pData->ftLastAccessTime = {};
    pData->ftLastWriteTime = GetFileTime(pEntry);
}

bool cADFPluginData::AdfChangeToPath(std::wstring_view pPath, bool pIgnoreLast) {
    bool result = false;

    if (!LoadFile(pPath))
        return false;

    auto components = split_path_to_components(pPath, mPath);

    if (pIgnoreLast) {
        if (components.size() > 0)
            components.pop_back();
    }

    if (mAdfVolume) {

        adfToRootDir(mAdfVolume.get());

        auto head = GetCurrentDirectoryList();
        AdfList *cell = head.get();
        while (cell) {
            struct AdfEntry* entry = (AdfEntry*)cell->content;

            if (!(entry->type == ADF_ST_LFILE || entry->type == ADF_ST_LDIR || entry->type == ADF_ST_LSOFT)) {

                if (components.size()) {
                    auto Filename = latin1_to_wstring(entry->name);

                    // Found our sub directory?
                    if (components[0] == Filename) {

                        // Free current cell, and load the next
                        if (entry->type == ADF_ST_DIR || entry->type == ADF_ST_LDIR || components.size() > 1) {
                            adfChangeDir(mAdfVolume.get(), entry->name);

                            head = GetCurrentDirectoryList();
                            cell = head.get();
                        }

                        components.erase(components.begin());

                        // Empty folder?
                        if (!cell || !components.size())
                            return true;

                        continue;
                    }
                }
                else {
                    result = true;
                    break;
                }
            }

            cell = cell->next;
        }
    }

    return result;
}

static void maybeCallAdfDevUnMount(AdfDevice* device) {
    if (!device) return;
    adfDevUnMount(device);
    adfDevClose(device);
}

static void maybeCallAdfVolUnMount(AdfVolume* volume) {
    if (volume) adfVolUnMount(volume);
}

bool cADFPluginData::LoadFile(std::wstring_view pAdfPath) {
    std::wstring AdfPath(pAdfPath);

    std::transform(AdfPath.begin(), AdfPath.end(), AdfPath.begin(), ::tolower);
    size_t EndPos = AdfPath.find(L".adf");
    if (EndPos == std::wstring::npos) {
        EndPos = AdfPath.find(L".hdf");
    }
    if (EndPos == std::wstring::npos)
        return false;
    AdfPath = pAdfPath.substr(0, EndPos + 4);

    if (!mAdfDevice || !mAdfVolume) {
        // Find an open device for this disk image
        auto Filepath = wstring_to_latin1(AdfPath);

        mAdfDevice = std::move(std::shared_ptr<AdfDevice>(adfDevOpen(const_cast<CHAR*>(Filepath.c_str()), ADF_ACCESS_MODE_READONLY), maybeCallAdfDevUnMount));
        adfDevMount(mAdfDevice.get());
        if(mAdfDevice)
            mAdfVolume = std::shared_ptr<AdfVolume>(adfVolMount(mAdfDevice.get(), 0, ADF_ACCESS_MODE_READONLY), maybeCallAdfVolUnMount);
                
        if (mAdfVolume) {
            mPath = AdfPath;
            return true;
        }

        return false;
    }

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

    auto head = GetCurrentDirectoryList();
    AdfList *cell = head.get();

    LPVFSFILEDATAHEADER lpLastHeader = 0;
    while (cell) {
        struct AdfEntry* entry = (AdfEntry*)cell->content;

        if (!(entry->type == ADF_ST_LFILE || entry->type == ADF_ST_LDIR || entry->type == ADF_ST_LSOFT)) {
            LPVFSFILEDATAHEADER lpFDH = GetVFSforEntry(entry, lpRDD->hMemHeap);

            // Add the entries for this folder
            if (lpFDH) {
                if (lpLastHeader)
                    lpLastHeader->lpNext = lpFDH;
                else
                    lpRDD->lpFileData = lpFDH;

                lpLastHeader = lpFDH;
            }
        }

        cell = cell->next;
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

spList cADFPluginData::GetCurrentDirectoryList() {
    return std::shared_ptr<AdfList>(adfGetDirEnt(mAdfVolume.get(), mAdfVolume->curDirPtr), adfFreeDirList);
}

int cADFPluginData::ContextVerb(LPVFSCONTEXTVERBDATAW lpVerbData) {
    if (AdfChangeToPath(lpVerbData->lpszPath, true)) {
        auto want_file_name = file_from_path(lpVerbData->lpszPath);

        auto head = GetCurrentDirectoryList();
        AdfList *cell = head.get();

        while (cell) {
            struct AdfEntry* entry = (AdfEntry*)cell->content;
            auto Filepath = latin1_to_wstring(entry->name);

            if (Filepath == want_file_name) {

                if (entry->type == ADF_ST_FILE)
                    return VFSCVRES_EXTRACT;

                if (entry->type == ADF_ST_DIR)
                    return VFSCVRES_DEFAULT;
            }

            cell = cell->next;
        }
    }

    return VFSCVRES_FAIL;
}

int cADFPluginData::Delete(LPVFSBATCHDATAW lpBatchData, std::wstring_view path, const std::wstring& pFile, bool pAll) {
    int result = 0;
    DOpus.UpdateFunctionProgressBar(lpBatchData->lpFuncData, PROGRESSACTION_STATUSTEXT, (DWORD_PTR)"Deleting");

    auto components = split_path_to_components(pFile);

    if (!AdfChangeToPath(path)) return 0;

    auto head = GetCurrentDirectoryList();
    AdfList *cell = head.get();

    while (cell)
    {
        if (lpBatchData->hAbortEvent && WaitForSingleObject(lpBatchData->hAbortEvent, 0) == WAIT_OBJECT_0)
        {
            return 1;
        }

        struct AdfEntry *entry = (AdfEntry *)cell->content;
        auto Filename = latin1_to_wstring(entry->name);

        // Entry match?
        if (pAll || (components[components.size() - 1] == Filename))
        {
            DOpus.UpdateFunctionProgressBar(lpBatchData->lpFuncData, PROGRESSACTION_SETFILENAME, (DWORD_PTR)entry->name);
            DOpus.UpdateFunctionProgressBar(lpBatchData->lpFuncData, PROGRESSACTION_SETFILESIZE, (DWORD_PTR)entry->size);

            // TODO: perhaps can avoid creating strings here.
            std::wstring Filename(path);

            if (Filename[Filename.size() - 1] != '\\')
                Filename += L"\\";
            auto Filepath = latin1_to_wstring(entry->name);
            Filename += Filepath;

            if (entry->type == ADF_ST_FILE)
            {
                result |= adfRemoveEntry(mAdfVolume.get(), entry->parent, entry->name);
                if (!result)
                    DOpus.AddFunctionFileChange(lpBatchData->lpFuncData, true, OPUSFILECHANGE_REMDIR, Filename.c_str());
            }

            if (entry->type == ADF_ST_DIR)
            {

                Delete(lpBatchData, Filename, Filepath, true);
                result |= adfRemoveEntry(mAdfVolume.get(), entry->parent, entry->name);
                AdfChangeToPath(path);
                if (!result)
                    DOpus.AddFunctionFileChange(lpBatchData->lpFuncData, true, OPUSFILECHANGE_REMDIR, Filename.c_str());
            }
        }

        cell = cell->next;
    }

    return 0;
}

cADFFindData* cADFPluginData::FindFirstFile(LPWSTR lpszPath, LPWIN32_FIND_DATA lpwfdData, HANDLE hAbortEvent) {
    if (!AdfChangeToPath(lpszPath, true)) return nullptr;

    cADFFindData* findData = new cADFFindData();
    auto Filepath = wstring_to_latin1(file_from_path(lpszPath));

    findData->mFindMask = std::regex("(." + Filepath + ")");

    findData->mHead = GetCurrentDirectoryList();
    findData->mCell = findData->mHead.get();

    while (findData->mCell) {
        struct AdfEntry* entry = (AdfEntry*)findData->mCell->content;

        if (std::regex_match(entry->name, findData->mFindMask)) {

            if (!(entry->type == ADF_ST_LFILE || entry->type == ADF_ST_LDIR || entry->type == ADF_ST_LSOFT)) {

                GetWfdForEntry(entry, lpwfdData);
                findData->mCell = findData->mCell->next;
                break;
            }
        }

        findData->mCell = findData->mCell->next;
    }

    return findData;
}

bool cADFPluginData::FindNextFile(cADFFindData* pFindData, LPWIN32_FIND_DATA lpwfdData) {

    while (pFindData->mCell) {
        struct AdfEntry* entry = (AdfEntry*)pFindData->mCell->content;

        if (std::regex_match(entry->name, pFindData->mFindMask)) {

            if (!(entry->type == ADF_ST_LFILE || entry->type == ADF_ST_LDIR || entry->type == ADF_ST_LSOFT)) {

                GetWfdForEntry(entry, lpwfdData);

                pFindData->mCell = pFindData->mCell->next;
                return true;
            }
        }

        pFindData->mCell = pFindData->mCell->next;
    }

    return false;
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

    auto want_file_name = file_from_path(pPath);
    std::wstring Final(FinalPath);
    Final += want_file_name;

    FILETIME ft;
    {
        HANDLE file = CreateFile(pPath.data(), FILE_READ_ATTRIBUTES, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        ::GetFileTime(file, 0, 0, &ft);
        CloseHandle(file);
    }
    DateTime dt = ToDateTime(ft);

    int fd = adfCreateDir(mAdfVolume.get(), mAdfVolume->curDirPtr, wstring_to_latin1(want_file_name).data() /*, dt */);
    DOpus.AddFunctionFileChange(lpBatchData->lpFuncData, true, OPUSFILECHANGE_MAKEDIR, Final.c_str());

    auto head = GetCurrentDirectoryList();
    AdfList *cell = head.get();

    while (cell) {
        struct AdfEntry* entry = (AdfEntry*)cell->content;
        auto Filename = latin1_to_wstring(entry->name);

        if (Filename == want_file_name) {

            auto contents = directoryList(FinalPath + L"*.*");

            for (auto& file : contents) {
                if (file == L"." || file == L"..")
                    continue;

                std::wstring name = std::wstring(pFile) + L"\\" + std::wstring(want_file_name) + L"\\" + file;
                Import(lpBatchData, name, FinalPath + file);
            }

        }

        cell = cell->next;
    }


    return 0;
}

int cADFPluginData::Import(LPVFSBATCHDATAW lpBatchData, std::wstring_view pFile, std::wstring_view pPath) {

    if (AdfChangeToPath(pFile, false)) {

        if (lpBatchData->hAbortEvent && WaitForSingleObject(lpBatchData->hAbortEvent, 0) == WAIT_OBJECT_0) {
            return 1;
        }

        // is pPath a directory?
        auto Attribs = GetFileAttributes(pPath.data());

        if (Attribs & FILE_ATTRIBUTE_DIRECTORY)
            ImportPath(lpBatchData, pFile, pPath);
        else {
            ImportFile(lpBatchData, pFile, pPath);
        }
    }

    return 0;
}

int cADFPluginData::Extract(LPVFSBATCHDATAW lpBatchData, const std::wstring& pFile, const std::wstring& pDest) {
    DOpus.UpdateFunctionProgressBar(lpBatchData->lpFuncData, PROGRESSACTION_STATUSTEXT, (DWORD_PTR)"Copying");

    // TODO: what does `1` signify? Should we return different error codes for different failure cases?
    int result = 1;
    if (!AdfChangeToPath(pFile, true)) return result;

    auto want_file_name = file_from_path(pFile);

    auto head = GetCurrentDirectoryList();
    AdfList *cell = head.get();

    while (cell) {

        if (lpBatchData->hAbortEvent && WaitForSingleObject(lpBatchData->hAbortEvent, 0) == WAIT_OBJECT_0) {
            return 1;
        }

        struct AdfEntry* entry = (AdfEntry*)cell->content;

        std::wstring FinalName = pDest;
        if (FinalName[FinalName.size() - 1] != '\\') {
            FinalName += '\\';
        }
        auto Filename = latin1_to_wstring(entry->name);
        FinalName += Filename;

        // Entry match?
        if (Filename == want_file_name) {

            DOpus.UpdateFunctionProgressBar(lpBatchData->lpFuncData, PROGRESSACTION_SETFILENAME, (DWORD_PTR)entry->name);
            DOpus.UpdateFunctionProgressBar(lpBatchData->lpFuncData, PROGRESSACTION_SETFILESIZE, (DWORD_PTR)entry->size);

            // Create a directory, or extract a file?
            if (entry->type == ADF_ST_DIR) {
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
                result = ExtractFile(lpBatchData, entry, FinalName);
            }

            DOpus.UpdateFunctionProgressBar(lpBatchData->lpFuncData, PROGRESSACTION_NEXTFILE, 0);
            break;
        }

        cell = cell->next;
    }

    return result;
}

int cADFPluginData::ExtractPath(LPVFSBATCHDATAW lpBatchData, const std::wstring& pPath, const std::wstring& pDest) {
    int result = 0;

    if (!AdfChangeToPath(pPath)) return result;

    auto head = GetCurrentDirectoryList();
    AdfList *cell = head.get();

    while (cell)
    {
        struct AdfEntry *entry = (AdfEntry *)cell->content;

        std::wstring FinalPath = pPath;
        if (FinalPath[FinalPath.size() - 1] != '\\')
        {
            FinalPath += '\\';
        }
        auto Filepath = latin1_to_wstring(entry->name);
        FinalPath += Filepath;

        result |= Extract(lpBatchData, FinalPath, pDest);

        cell = cell->next;
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
	auto entryTime = GetFileTime(pEntry);
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

    auto result = VFSBATCHRES_COMPLETE;

    auto file = lpBatchData->pszFiles;

    for (int i = 0; i < lpBatchData->iNumFiles; ++i) {

        if (lpBatchData->uiOperation == VFSBATCHOP_EXTRACT) {
            lpBatchData->piResults[i] = Extract(lpBatchData, file, lpBatchData->pszDestPath);
        }

        if (lpBatchData->uiOperation == VFSBATCHOP_ADD) {
            lpBatchData->piResults[i] = Import(lpBatchData, path, file);
        }

        if (lpBatchData->uiOperation == VFSBATCHOP_DELETE) {
            lpBatchData->piResults[i] = Delete(lpBatchData, path, file);
        }

        if (lpBatchData->piResults[i]) {
            result = VFSBATCHRES_ABORT;
            break;
        }

        file += wcslen(file) + 1;
    }

    return result;
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