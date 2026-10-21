# Design documents

These documents describe the architecture of NewFogL. The full texts are written in
Chinese; this index provides an English summary of each file.

| File | Content |
|---|---|
| `01-architecture.txt` | Directory layout, embedded resources, the 13-stage launcher flow and the 10-stage loader flow |
| `02-launcher-loader-interface.txt` | Protocol between the C++ launcher and the Java loader: CLI arguments, `[STATE]` lines, exit codes, `newfog-rules.json` |
| `03-mod-loader.txt` | Java loader design: scanning, metadata parsing, five pre-flight checks, dependency resolution, deployment, sandbox planning |
| `04-launcher.txt` | C++ launcher design: module layout, process management, IPC, crash detection, instance management, embedded resource extraction |
| `05-naming-conventions.txt` | Naming rules for packages, classes, methods and variables (C++ and Java) |
| `06-code-comments.txt` | Comment requirements: file headers, class headers, method headers |
| `07-error-handling.txt` | Error categories, exit codes, log format and user-facing messages |
| `08-config-reference.txt` | All JSON configuration files and their fields |
| `09-module-dependencies.txt` | Module dependency graph for both the launcher and the loader |
| `10-project-overview.txt` | Plain-language introduction to the project and its current progress |

## Status

The project is in early development. Some designed modules are not implemented yet;
the README in the repository root lists the current progress.
