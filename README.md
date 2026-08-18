# ARMBIO4

Android wrapper for **Biohazard 4 / Resident Evil 4 Mobile Edition**.

ARMBIO4 loads the original game's ARM native libraries and provides the Android code needed to run them.

## Features

- OpenGL ES 2.0
- Touch input
- Gamepad and physical button input
- Accelerometer
- FMV playback
- Landscape mode
- Fullscreen mode
- Runtime patches

## How it works

```text
Android App
    │
    ├── Bio4PreActivity
    ├── OpenGL ES
    ├── JNI Bridge
    │
    └── Native Bridge
          │
          ├── libbio4af.so
          └── libmc_eruption_for_android_jni.so

The native bridge loads the game's .so files with dlopen() and uses dlsym() to access the required functions.

Since ARMBIO4 runs on Android, it uses the real Android JNI, OpenGL ES, and media APIs instead of recreating them like the Vita port does.

## Requirements

- Android 5.0+ (API 21)
- ARM 32-bit device (armeabi-v7a)
- Original Biohazard 4 Android game files


## Game files

Put the game files in:
```
/sdcard/data/Bio4/
├── libbio4af.so
├── libmc_eruption_for_android_jni.so
└── assets/
```
The libraries can also be bundled with the APK:
```
app/src/main/jniLibs/armeabi-v7a/
```
## Building

### Install the Android SDK and NDK, then run:
```bash
./gradlew assembleDebug
```
For a release build:
```bash
./gradlew assembleRelease
```
### Install the APK:
```
adb install app/build/outputs/apk/debug/app-debug.apk
```

### Patches

ARMBIO4 currently includes fixes for:
- JNI string reference handling
- Repeated GetMilliSeconds JNI lookups

### Disclaimer

Biohazard 4 / Resident Evil 4 is owned by Capcom Co., Ltd.

ARMBIO4 is not affiliated with or endorsed by Capcom.

The project does not include the original game files. You must provide your own legally obtained copy.

### License

Apache License 2.0.

See [LICENSE](LICENSE) and [NOTICE](NOTICE].

## Credits

Based on work from [bio4-master](https://gitee.com/Moqi01/bio4), a PS Vita homebrew port of Biohazard 4.