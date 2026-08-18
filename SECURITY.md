# Security Policy

## Reporting a Vulnerability

If you discover a security vulnerability in ARMBIO4, please report it responsibly:

1. **Do NOT** file a public GitHub issue for security vulnerabilities
2. Email the maintainer directly or use [GitHub Security Advisories](https://github.com/aarvsn/armbio4/security/advisories/new)
3. Include:
   - Description of the vulnerability
   - Steps to reproduce
   - Affected versions
   - Potential impact

## Response Timeline

- **Acknowledgment**: Within 48 hours
- **Initial assessment**: Within 7 days
- **Fix**: Depends on severity, typically within 30 days

## Security Considerations

ARMBIO4 loads native `.so` libraries from external storage at runtime. This is inherently a security-sensitive operation:

- The app only loads libraries from the designated game data directory
- Path traversal attacks are mitigated by validating the data directory
- Users should only load libraries from trusted sources (their own legal game copy)
- The app requests `MANAGE_EXTERNAL_STORAGE` only for accessing game data — no other file access is needed

## Supported Versions

| Version | Supported |
| ------- | --------- |
| 1.0.x   | ✅ Active |
| < 1.0   | ❌ Pre-release |
