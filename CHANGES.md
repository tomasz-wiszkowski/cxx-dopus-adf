# Changelog

## What's New (since initial fork) - v0.5

### Build System
- Dropped Visual Studio project files; migrated to **CMake** (Ninja generator, x64 debug/release configurations)
  - MSVC-generated code `Release` code would fail for unclear reasons, where
    `Debug` worked reasonably well. Migrated to `clang` for reliability.
- Bumped C++ standard to C++20 to benefit from filesystem, ranges, etc.
- `adflib` upgraded to **0.10.5**
- Directory Opus SDK is now downloaded automatically from gpSoftware at build time
- GitHub Actions workflow added to **auto-publish releases** on tag push
- VS Code tasks added for deploy-and-restart workflow
- `.clang-format` / clang-tidy enforced on save

### Code Modernisation
- Adopted `std::filesystem::path` throughout (replacing raw `WCHAR*` / `std::wstring` path handling)
- Replaced C-style casts and raw pointers with RAII wrappers (`adf::AdfDeviceDeleter`, `adf::AdfVolumeDeleter`, `Guard<>`)
- Introduced `dopus::wstring_view_span` helper type for iterating DOpus null-terminated string arrays
- Isolated Latin-1 ↔ UTF-16 conversion into `text_utils.hh`; fixed Windows Latin-1 character encoding
- Removed magic numbers; adopted `std::span`, `std::ranges` algorithms, and C++ wrappers for ADF objects
- Strengthened compiler warnings; cleaned up unused types, functions, and C-style returns

### Supported Operations

| Operation | Status |
|-----------|--------|
| Browse ADF/HDF in Opus lister (all view modes) | Working |
| Read directory listing (files & subdirectories) | Working |
| Extract files / directories to host filesystem | Working |
| Copy / Paste files **into** ADF/HDF | Working |
| Create directories inside ADF/HDF | Working |
| Delete files and directories | Working |
| Move / Rename files within the same ADF/HDF | Working (`MOVEBYRENAME` capability declared) |
| `GetFileInformation` (single-file metadata) | Working |
| `GetLastError` propagation | Working |
| `GetFreeDiskSpace` (free / total bytes) | Working |
| File timestamps read (last-write time shown in lister) | Working |
| Drag-and-drop (copy + move effects) | Working |
| Case-sensitive filenames | Declared via `VFSCAPABILITY_CASESENSITIVE` |
| HDF (hard-disk image, partition 0) | Working |
| `VFS_SetFileAttrW` — Amiga file protection bits | Stub returns `false`; adflib supports `adfSetEntryAccess` |
| `VFS_SetFileCommentW` — Amiga file comments | Stub returns `false`; adflib supports `adfSetEntryComment` |
| `VFS_SetFileTimeW` — modify timestamps | Stub returns `false`; internal `SetEntryTime` helper exists but is not wired up |
| File timestamp written on import | `ImportFile` writes data but does not call `SetEntryTime` |
| Streaming / chunked copy | Files are read fully into memory before writing (`ImportFile` TODO noted in source) |
| Read error handling during import | `ImportFile` does not check read errors (TODO noted) |
| Multi-partition HDF | Only partition 0 is mounted |
| Amiga soft-links | `ADF_ST_LNKFILE` / `ADF_ST_LNKDIR` entries are silently skipped |
| Plugin configuration / About dialog | `VFSF_CANCONFIGURE \| VFSF_SHOWABOUT` intentionally disabled |
| Tests | completely missing :( |
