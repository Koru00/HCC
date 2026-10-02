# Architecture

## Overview

HolyCC is a C++20 Visual Studio solution organized as a set of libraries and executable projects. Its project names and declared dependencies outline a compiler-oriented pipeline, supported by shared core utilities, runtime and debug-information libraries, and application entry points.

The solution currently contains foundational implementations in `HolyCore` and a small custom test harness in `HolyTest`. Most compiler-stage translation units and application entry points are stubs. The dependency graph below describes project references, not a claim that the full pipeline is operational.

## Projects and dependency flow

| Project | Intended responsibility | Declared project dependencies | Current source outline |
| --- | --- | --- | --- |
| `HolyCore` | Platform and shared data structures | — | Arena, vectors, hash map, source management, string interning, Win32 services |
| `HolyLex` | Lexical analysis | `HolyCore` (project file reference) | `Lexer.cpp` placeholder |
| `HolyAST` | Syntax-tree representation and printing | `HolyLex` | `AstPrinter.cpp` placeholder |
| `HolySema` | Semantic analysis | `HolyAST` | `Sema_Decl.cpp` placeholder |
| `HolyIR` | Intermediate representation and lowering | `HolySema` | `Lower_Decl.cpp` placeholder |
| `HolyDbgInfo` | Debug-information writing | `HolyCore` | `DebugWriter.cpp` placeholder |
| `HolyGen` | Code generation and x64 instruction selection | `HolyDbgInfo`, `HolyIR` | `ISel_X64.cpp` placeholder |
| `HolyLink` | Linking and global-symbol management | `HolyGen` | `GlobalSymbolTable.cpp` placeholder |
| `HolyRT` | Runtime support | `HolyCore` | `Mem.cpp` placeholder |
| `HolyCC` | Compiler executable | `HolyLink`, `HolyRT` | `main` currently returns immediately |
| `HolyShell` | Shell executable | `HolyLink`, `HolyRT` | `main` currently returns immediately |
| `HolyCDB` | Debugger/database executable | `HolyDbgInfo` | `main` currently returns immediately |
| `HolyEdit` | Editor executable | `HolyLink`, `HolyRT` | `wWinMain` currently returns immediately |
| `HolyTest` | Test executable | `HolyLink`, `HolyRT` | Custom test harness and core tests |

The project files also define include paths and linker inputs that are broader than the explicit solution build-dependency list. Consult the `.vcxproj` files when changing project references or build configuration.

## Design notes

### Shared storage and data structures

`HolyCore::Arena` uses reserved virtual memory and bump allocation. `Vector`, `HashMap`, `SourceManager`, and `StringInterner` use arena-owned storage. Arena allocations are reclaimed together by resetting or destroying the arena; individual container growth does not reclaim its previous arena allocation.

### Source coordinates

`SourceManager` owns copies of registered file names and contents. `SourceLoc` is a nonzero global byte offset, `FileId` is a one-based registration index, and resolved line and column values are one-based. Columns count bytes rather than Unicode code points.

### Platform boundary

Platform and file APIs are declared in `HolyCore/Platform.h`. The implementation currently included in the project is Win32-specific (`Platform_Win32.cpp`); no non-Windows implementation is present in the project sources.

## Current scope

The repository does not yet define an implemented language grammar, parser, semantic rules, IR format, compiler command-line contract, or end-to-end compilation flow. Project names and filenames communicate intended roles only; consult [Project status](ProjectStatus.md) for what is actually implemented.