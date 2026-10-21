# Samples — Authentication module

These files are an excerpt of the NewFogL launcher source, published so that the
Microsoft / Xbox Live / Minecraft Services usage can be reviewed.

| File | Purpose |
|---|---|
| `net/HttpClient.h` / `.cpp` | Minimal synchronous HTTP client (WinHTTP), GET and POST form/JSON |
| `auth/MicrosoftAuth.h` / `.cpp` | OAuth 2.0 device code flow, Xbox Live, XSTS, Minecraft login, profile |
| `auth/TokenCrypto.h` / `.cpp` | Token encryption with Windows DPAPI (`CryptProtectData`) |
| `auth/AuthService.h` / `.cpp` | Local + online validation of a stored token and game entitlement |

## Notes

- The client ID used in `MicrosoftAuth.h` is a public client identifier, not a secret.
  Public clients do not use a client secret.
- Tokens are encrypted with DPAPI before being stored; the config file never contains
  plaintext tokens.
- These files are not a standalone buildable project — they depend on the launcher's
  `core/Config`, `core/PathResolver` and logging modules, which are not part of this
  excerpt.
