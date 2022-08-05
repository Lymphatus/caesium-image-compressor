## Caesium Image Compressor

###### v2.2.0

![x](https://user-images.githubusercontent.com/12133996/141089441-b7589314-3a5d-48ff-8854-c3d32bd8dd5e.PNG)

----------
### What is it for
Caesium is an image compression software that helps you store, send and share digital pictures, supporting JPG, PNG and WebP formats.  
You can quickly reduce the file size (and resolution, if you want) by preserving the overall quality of the image.

### Supported Platforms
- **Windows**: 7+
- **MacOS**: 10.15+
- **Linux**: tested on Ubuntu 22.04 and Manjaro

Note: only 64bit versions are supported

### Installation
Head to the [releases' page](https://github.com/Lymphatus/caesium-image-compressor/releases) for the available downloads.
- **Windows**: installer and portable versions are available
- **MacOS**: DMG package
- **Linux**: no binaries are available right now, you must compile the source code yourself

### Troubleshooting and/or feature request
Please open an [issue](https://github.com/Lymphatus/caesium-image-compressor/issues).

### Build from source
##### Requirements
- [Rust](https://www.rust-lang.org/tools/install): required to compile [libcaesium](https://github.com/Lymphatus/libcaesium). Make sure you have `cargo` executable on you `$PATH`
- [Qt6 SDK](https://www.qt.io/download/): binaries are built on 6.2.4 (open source)
- [libssh](https://www.libssh.org/): macOS only

#### Build
##### Step 1
You need to configure CMake first and the command is slightly different for all the platforms:
Change the path in variables with the correct directories of the requirements.
###### Windows
```
cmake -B build_dir -DCMAKE_PREFIX_PATH=/path/to/Qt/version -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles"
```
###### MacOS
```
cmake -B build_dir -DCMAKE_PREFIX_PATH=/path/to/Qt/version/macos -DLIBSSH_INCLUDE_DIR=/libssh/dir/include -DCMAKE_BUILD_TYPE=Release
```
###### Linux
Make sure you have all the requirements installed with you own package manager
```
cmake -B build_dir -DCMAKE_PREFIX_PATH=/path/to/Qt/version/gcc_64 -DCMAKE_BUILD_TYPE=Release
```
##### Step 2
Then you can build with
```
cmake --build build_dir --config Release --target caesium_image_compressor
```

### Languages and translations
The 1.0 version of Caesium had a lot of community made translations, but this 2.0 release has a whole new set of strings and no old translations can be used anymore.   
Caesium 2.0 is currently available in:
- 🇺🇸 English ![100%](https://progress-bar.dev/100)
- 🇮🇹 Italian ![100%](https://progress-bar.dev/100)
- 🇫🇷 French ![100%](https://progress-bar.dev/100)
- 🇯🇵 Japanese ![94%](https://progress-bar.dev/94)
- 🇨🇳 Simplified Chinese ![100%](https://progress-bar.dev/100)
- 🇪🇸 Spanish ![94%](https://progress-bar.dev/94)
- 🇪🇬 Arabic (Egypt) ![100%](https://progress-bar.dev/100)
- 🇩🇪 German ![90%](https://progress-bar.dev/90)

#### How to contribute with your translation
You can help translate by opening the `resources/i18n/caesium_en_US.ts` with QtLinguist or any XML editor. Rename it to your language (e.g. `caesium_it_IT.ts`) and translate all the sentences. Then you can start a pull request with your translated file.  
You can use [this pull request](https://github.com/Lymphatus/caesium-image-compressor/pull/106) as an example.
Thanks to all the people who contributed.
