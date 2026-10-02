# Project Status

This inventory distinguishes project intent (suggested by project and source names) from code currently present. Most pipeline modules have a translation-unit placeholder and no public API headers in the current repository.

| Project | Intended role | Current implementation |
| --- | --- | --- |
| `HolyCore` | Shared memory, containers, source and platform utilities | Implemented foundational utilities; see [API reference](HolyCore/README.md). Win32 platform implementation. |
| `HolyLex` | Lexer | `Lexer.cpp` is a placeholder; no lexer API or behavior is defined. |
| `HolyAST` | AST and printer | `AstPrinter.cpp` is a placeholder; no AST API is defined. |
| `HolySema` | Semantic analysis | `Sema_Decl.cpp` is a placeholder; semantic rules are not defined. |
| `HolyIR` | IR and lowering | `Lower_Decl.cpp` is a placeholder; IR is not defined. |
| `HolyDbgInfo` | Debug-information output | `DebugWriter.cpp` is a placeholder. |
| `HolyGen` | Code generation / x64 instruction selection | `ISel_X64.cpp` is a placeholder. |
| `HolyLink` | Linking and global symbols | `GlobalSymbolTable.cpp` is a placeholder. |
| `HolyRT` | Runtime support | `Mem.cpp` is a placeholder. |
| `HolyCC` | Compiler application | `main` returns immediately; no command-line interface or compilation pipeline. |
| `HolyShell` | Shell application | `main` returns immediately; no shell behavior. |
| `HolyCDB` | Debugger/database application | `main` returns immediately; no debugger behavior. |
| `HolyEdit` | Editor application | `wWinMain` returns immediately; no editor behavior. |
| `HolyTest` | Test executable | Registration-based harness and core tests are present. |

Project names and declared dependencies show intended organization, not completed functionality. Update this page as modules acquire public contracts and working implementations.