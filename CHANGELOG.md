# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.0.1] - 2025-11-07

### Added
- Initial release
- File browser with vim-style navigation (j/k, arrows)
- Automatic FNV-1a hash calculation during scan
- Duplicate file detection
- Safe file/directory deletion with confirmation
- System path protection (/, /home, /usr, etc.)
- Removable media warnings
- Color-coded file types (executables, directories, duplicates, zero-byte)
- Async directory scanning with progress indicator
- Human-readable file sizes
- Virtualized rendering (handles 100k+ files)

### Known Issues
- The loading indicator sometimes does not disappear after changing directories (being investigated) <- FIXED!
- No directory size calculation (files only)
- TUI doesn't accept command-line path argument (use CLI tool instead)
- No configuration file support

### Technical Details
- C++17 codebase
- FTXUI v6.1.9 for terminal UI
- CMake-based build system with FetchContent for dependencies
- Doxygen-documented API