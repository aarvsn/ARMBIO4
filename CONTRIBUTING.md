# Contributing to ARMBIO4

Thank you for your interest in contributing! Here's how to get started.

## Development Setup

1. **Fork** the repository to your GitHub account
2. **Clone** your fork locally:
   ```bash
   git clone https://github.com/YOUR_USERNAME/armbio4.git
   cd armbio4
   ```
3. **Set up** Android SDK + NDK (API 34, NDK r25+)
4. **Build** to verify everything works:
   ```bash
   ./gradlew assembleDebug
   ```

## Making Changes

1. Create a **feature branch** from `develop`:
   ```bash
   git checkout -b feature/my-feature develop
   ```
2. Make your changes and **commit** with clear messages
3. **Test** your changes:
   ```bash
   ./gradlew assembleDebug
   ./gradlew lint
   ```
4. **Push** to your fork and open a Pull Request against `develop`

## Code Style

- **Java**: Follow [Google Java Style Guide](https://google.github.io/styleguide/javaguide.html)
- **C**: Follow the existing `.clang-format` style in `app/src/main/cpp/`
- **Commits**: Use [Conventional Commits](https://www.conventionalcommits.org/) format:
  ```
  feat: add gamepad rumble support
  fix: resolve touch event race condition
  docs: update README with build instructions
  ```

## Pull Request Guidelines

- Keep PRs focused — one feature/fix per PR
- Include a clear description of what changes and why
- Verify CI passes before requesting review
- Update documentation if you change public APIs

## Reporting Issues

- Use [GitHub Issues](https://github.com/aarvsn/armbio4/issues)
- Include device info (model, Android version, architecture)
- Include logcat output for crashes: `adb logcat -s Bio4 Bio4Native Bio4GL`

## Important Note

**Do NOT submit any copyrighted game assets, code, or data from Capcom.** This project only accepts contributions to the wrapper/bridge code. Game data files must be provided by users from their own legal copies.

## License

By contributing, you agree that your contributions will be licensed under the Apache License 2.0.
