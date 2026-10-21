# NewFogL Launcher

An open-source launcher for Minecraft: Java Edition 1.20.1, written in C++17 with Qt 6,
plus a Java-based mod loader written in Java 21.

> This repository is a **partial public snapshot** used to document the project and to
> apply for Minecraft Services access. The complete source is released together with
> each binary release under AGPL-3.0, as required by the license.

## Why Microsoft sign-in is required

NewFogL uses the Microsoft identity platform (OAuth 2.0 device code flow) only to:

1. verify that the user owns Minecraft: Java Edition;
2. obtain the user's Minecraft profile (name and UUID);
3. launch the game under that profile on the user's own computer.

The flow is exactly the standard one used by third-party launchers:

```
Microsoft OAuth (device code)
        -> Xbox Live authentication
        -> XSTS authorization (rp://api.minecraftservices.com/)
        -> Minecraft Services login
        -> /minecraft/profile
```

## Token security

- Access and refresh tokens are obtained directly from Microsoft; no intermediary
  server is involved.
- Tokens are encrypted with **Windows DPAPI** (`CryptProtectData`, current-user scope)
  before being written to disk. The configuration file only contains `dpapi:`-prefixed
  ciphertext.
- Tokens are **never** written to logs, error messages, or UI text.
- Tokens are **never** transmitted to any server operated by this project.
  NewFogL has no backend service.
- The application performs no telemetry, analytics, or crash upload.

See [PRIVACY.md](PRIVACY.md) for the full data-handling description.

## Development progress

The launcher is in early development. Current state:

| Area | Status |
|---|---|
| Main window, title bar, navigation | Implemented (UI) |
| Sidebar, instance selection, log panel | Implemented (UI) |
| Instance settings (memory, Java, shortcuts) | Implemented (UI + local persistence) |
| Global settings (launch options, memory, background) | Implemented (UI + JSON persistence) |
| Disclaimer / terms gating | Implemented (UI) |
| Authentication module (device code, Xbox, XSTS, profile) | Implemented, pending Mojang app approval |
| Instance creation / import gating | Implemented, temporarily relaxed during development |
| Launch pipeline (junction sync, loader process, state parsing) | Partially implemented |
| Java loader: scanning, pre-flight, dependency resolution, deployment | Implemented |
| Java loader: sandbox execution of the actual game | In progress |
| Pack export / import | Exporter and importer exist; UI integration is a placeholder |
| Three-loader coexistence (Forge + NeoForge + Fabric) | Design only, not implemented |

This repository contains the design documents (`docs/`) and the authentication module
(`samples/src/auth`, `samples/src/net`) so that the API usage can be reviewed.

## What this project does and does not do

- Does **not** bundle, mirror, or redistribute the Minecraft client, assets, or libraries.
- Downloads game files only from Mojang's official endpoints, on the user's machine.
- Does **not** bypass authentication or licensing in any way.
- Does **not** collect telemetry or analytics.
- Requires the user to own a legitimate copy of Minecraft: Java Edition.

## Build (full source, not included in this snapshot)

- Windows 10/11 x64, CMake 3.20+, MSVC 2022, Qt 6 (Core / Gui / Widgets)

```
cmake -S newfogl-launcher -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

## License

GNU Affero General Public License v3.0. See [LICENSE](LICENSE).

## Contact

xysxys1001@qq.com
