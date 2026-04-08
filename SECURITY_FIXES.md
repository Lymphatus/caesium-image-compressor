# Security Fixes

A security review of the Caesium Image Compressor codebase identified two high-severity vulnerabilities: an API endpoint hijacking risk that could leak bearer tokens to attacker-controlled servers, and OS command injection via `system()` calls for shutdown/sleep operations. Both have been fixed with minimal, targeted changes.

| File Path | Vulnerability Type | CWE | Severity | What Was Changed |
|---|---|---|---|---|
| `src/network/NetworkOperations.cpp` | Improper Input Validation / Credential Theft via Endpoint Hijacking | CWE-346 (Origin Validation Error) | High | Custom API endpoint is now trimmed and validated: must be a well-formed HTTPS URL. Rejects non-HTTPS or malformed URLs and falls back to the hardcoded default. Prevents a local attacker from redirecting API traffic (including auth tokens) to an arbitrary server. |
| `src/utils/PostCompressionActions.cpp` | OS Command Injection via `system()` | CWE-78 (OS Command Injection) | High | Replaced all `system()` calls with `QProcess::startDetached()` using explicit argument lists. `QProcess` does not invoke a shell, eliminating shell interpretation, PATH manipulation, and environment variable injection risks. |

Additional lower-severity findings were identified during this review and can be provided in follow-up PRs if desired.
