# HolyCC Documentation

This folder contains the architecture and developer guide for the HolyCC solution, along with API references for implemented shared components.

## Start here

- [Architecture](Architecture.md) — solution layout, module responsibilities, and project dependencies.
- [Developer guide](DeveloperGuide.md) — prerequisites, Visual Studio build workflow, and project conventions.
- [Testing](Testing.md) — test harness and current test workflow.
- [Project status](ProjectStatus.md) — implementation status and entry points for all solution projects.

## API reference

- [HolyCore overview](HolyCore/README.md)
- [Arena](HolyCore/Arena.md)
- [HashMap](HolyCore/HashMap.md)
- [Platform](HolyCore/Platform.md)
- [SourceManager](HolyCore/SourceManager.md)
- [StringInterner](HolyCore/StringInterner.md)
- [Vector](HolyCore/Vector.md)

The reference describes the implementation currently present in the repository. Compiler-language syntax, command-line behavior, and APIs for modules without public headers are not specified here because they are not implemented or defined in the current source tree.