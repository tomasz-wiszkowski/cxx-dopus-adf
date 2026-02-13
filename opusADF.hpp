typedef std::shared_ptr<AdfList>   spList;
typedef std::shared_ptr<AdfDevice> spDevice;
typedef std::shared_ptr<AdfVolume> spVolume;

typedef std::weak_ptr<AdfDevice>   wpDevice;
typedef std::weak_ptr<AdfVolume>   wpVolume;

class cADFFindData {

public:

    spList mHead;
    AdfList*  mCell;
    std::regex mFindMask;

};

class cADFPluginData {

    HINSTANCE				mAdfDll;
    size_t                  mLastError;
    std::wstring             mPath;

    spDevice                 mAdfDevice;
    spVolume                 mAdfVolume;

protected:

    LPVFSFILEDATAHEADER GetVFSforEntry(const AdfEntry *pEntry, HANDLE pHeap);
    void GetWfdForEntry(const AdfEntry *pEntry, LPWIN32_FIND_DATA pData);

    FILETIME            GetFileTime(const AdfEntry *pEntry);
    void                SetEntryTime(AdfFile *pFile, FILETIME pFT);

    bool LoadFile(const std::wstring& pAfPath);
    spList GetCurrentDirectoryList();

public:
    cADFPluginData();
    ~cADFPluginData();

    std::vector<std::wstring> GetPaths(std::wstring pPath);

    bool AdfChangeToPath(const std::wstring& pPath, bool pIgnoreLast = false);

    bool ReadDirectory(LPVFSREADDIRDATAW lpRDD);
    bool ReadFile(AdfFile* pFile, size_t pBytes, std::uint8_t* pBuffer, LPDWORD pReadSize );

    AdfFile* OpenFile(std::wstring pPath);
    void CloseFile(AdfFile* pFile);
 
    size_t TotalFreeBlocks(const std::wstring& pFile);
    size_t TotalDiskBlocks(const std::wstring& pFile);

    int Delete(LPVFSBATCHDATAW lpBatchData, const std::wstring& pPath, const std::wstring& pFile, bool pAll = false);

    cADFFindData *FindFirstFile(LPTSTR lpszPath, LPWIN32_FIND_DATA lpwfdData, HANDLE hAbortEvent);
    bool FindNextFile(cADFFindData* lpRAF, LPWIN32_FIND_DATA lpwfdData);
    void FindClose(cADFFindData* lpRAF);

    int Import(LPVFSBATCHDATAW lpBatchData, const std::wstring& pFile, const std::wstring& pPath);
    int ImportFile(LPVFSBATCHDATAW lpBatchData, const std::wstring& pFile, const std::wstring& pPath);
    int ImportPath(LPVFSBATCHDATAW lpBatchData, const std::wstring& pFile, const std::wstring& pPath);

    int Extract(LPVFSBATCHDATAW lpBatchData, const std::wstring& pFile, const std::wstring& pDest);
    int ExtractFile(LPVFSBATCHDATAW lpBatchData, const AdfEntry* pEntry, const std::wstring& pDest);
    int ExtractPath(LPVFSBATCHDATAW lpBatchData, const std::wstring& pPath, const std::wstring& pDest);

    int ContextVerb(LPVFSCONTEXTVERBDATAW lpVerbData);
    UINT BatchOperation(LPTSTR lpszPath, LPVFSBATCHDATAW lpBatchData);
    bool PropGet(vfsProperty propId, LPVOID lpPropData, LPVOID lpData1, LPVOID lpData2, LPVOID lpData3);

};
