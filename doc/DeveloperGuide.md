# Developer Guide

## Prerequisites

The solution is a native C++ Visual Studio solution (`HolyCC.slnx`). Project configurations specify C++20, the MSVC `v145` toolset, and the Windows 10 SDK. Install Visual Studio's C++ desktop development workload and the required MSVC toolset and SDK. The solution exposes `x64` and `x86` platforms; individual project configuration details may differ.

The project files contain repository-specific absolute include and library paths. If building from another checkout location, inspect the `.vcxproj` files and update those paths as appropriate before building.

## Build in Visual Studio

1. Open `HolyCC.slnx` in Visual Studio.
2. Select a configuration such as `Debug` or `Release` and a platform such as `x64`.
3. Build the solution, or build an individual project from Solution Explorer.
4. Resolve any missing toolset, SDK, or path configuration before treating a failure as a source-code issue.

There is no repository-provided build script or package restore workflow in the current solution.

## Repository layout

- `HolyCore/Include/HolyCore/` contains public core headers.
- `HolyCore/Sources/` contains core implementations.
- Other projects keep their current translation units under `Sources/`; several modules do not yet have public headers.
- `HolyTest/Sources/Core/` contains focused tests for core functionality.
- `doc/` contains this guide and component references.

## Working with core components

Use one `Arena` as the owner for a group of arena-backed objects, and ensure it outlives every object referring to it. `Vector<T>` and `HashMap<K,V>` require trivially copyable and trivially destructible element types. Their default constructors do not provide an arena; pass an arena before an operation that grows storage.

`SourceManager` and `StringInterner` retain copies in their arena. Their references and string views are valid only while the owning arena remains alive and has not been reset. See the individual API pages for contracts and failure behavior.

## Adding or changing code

- Keep declarations in the component's existing public include directory and implementations in its `Sources/` directory, following current project conventions.
- Update the `.vcxproj` and `.vcxproj.filters` when adding source files if the project does not discover them automatically.
- Keep solution build dependencies and project references aligned when introducing a dependency between modules.
- Add tests to `HolyTest` for behavior changes. Prefer checking invalid inputs and boundary cases as well as normal behavior.
- Update the relevant page under `doc/` when a public contract or project responsibility changes.

## Known limitations

The compiler stages and application entry points are mostly placeholders. There is no documented language grammar, complete compiler workflow, cross-platform build implementation, or stable command-line interface at this stage. Do not infer these contracts from project names.