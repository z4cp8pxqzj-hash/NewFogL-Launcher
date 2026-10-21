# Privacy Policy

NewFogL is a local desktop application. It has no backend server and no user accounts
of its own.

## Data the launcher handles

- **Microsoft access token / refresh token**, obtained through the OAuth 2.0 device code
  flow. They are stored only on the user's computer, encrypted with Windows DPAPI
  (`CryptProtectData`, current-user scope). They are used solely to sign in to Minecraft.
- **Minecraft profile name and UUID**, returned by Mojang, stored locally.

## Data the launcher does not collect

- No telemetry, analytics, crash reporting, or advertising identifiers.
- No upload of tokens, files, or usage data to any server operated by this project.

## Third-party endpoints

The launcher communicates only with:

- `login.microsoftonline.com` — OAuth 2.0 device code flow
- `user.auth.xboxlive.com`, `xsts.auth.xboxlive.com` — Xbox Live / XSTS
- `api.minecraftservices.com` — Minecraft login and profile
- Mojang official download endpoints — game files

## Logging

Tokens are never written to logs. Log files contain only launcher events and error
messages, and stay on the user's computer.

## Contact

2859638513@qq.com
