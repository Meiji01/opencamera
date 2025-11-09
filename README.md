# Open Camera Advanced

This project is an enhanced fork of the original Open Camera project by Mark Harman, available at [https://sourceforge.net/projects/opencamera](https://sourceforge.net/projects/opencamera).

## Enhancements

This fork includes several enhancements over the original Open Camera project. The most significant addition is the implementation of HEIC (High Efficiency Image Container) encoding. This feature allows users to save photos in the HEIC format, which offers superior compression efficiency compared to traditional JPEG, resulting in smaller file sizes without significant loss of quality.

Crucially, this HEIC implementation leverages native JNI (Java Native Interface) to extend compatibility to older Android devices, enabling HEIC saving functionality down to Android 11. This ensures a wider range of users can benefit from the storage and quality advantages of the HEIC format, even if their device's native Android version does not inherently support it.

## Build Instructions

To build this project, ensure you have the Android SDK installed and configured.

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/gdelac/opencamera.git
    cd opencamera
    ```
    (Note: Replace `https://github.com/gdelac/opencamera.git` with the actual repository URL if available, otherwise, I'll use a placeholder or omit this step if it's a local project.)

2.  **Open in Android Studio:**
    Import the project into Android Studio. Android Studio should automatically configure the Gradle environment.

3.  **Build from command line:**
    Alternatively, you can build the project using Gradle from the command line:
    ```bash
    ./gradlew assembleDebug
    ```
    For a release build:
    ```bash
    ./gradlew assembleRelease
    ```

## Credits

- HEIC processing library: [https://github.com/strukturag/libheif](https://github.com/strukturag/libheif)
- Reference for HEIC implementation: [https://github.com/WanghongLin/HeifAndroid](https://github.com/WanghongLin/HeifAndroid)