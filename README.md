## Caesium Image Compressor  [![](https://img.shields.io/static/v1?label=Sponsor&message=%E2%9D%A4&logo=GitHub&color=%23fe8e86)](https://github.com/sponsors/Lymphatus)

[![Build](https://github.com/Lymphatus/caesium-image-compressor/actions/workflows/build-qt.yml/badge.svg)](https://github.com/Lymphatus/caesium-image-compressor/actions/workflows/build-qt.yml)

###### v2.5.0

![caesium_screenshot_20220915](https://user-images.githubusercontent.com/12133996/190349051-6baebec3-9937-4047-8670-fb025c6c4869.PNG)

----------
### What is it for
Caesium is an image compression software that helps you store, send and share digital pictures, supporting JPG, PNG and WebP formats.  
You can quickly reduce the file size (and resolution, if you want) by preserving the overall quality of the image.

### Supported Platforms
- **Windows**: 10 (build 1809 or later) (use old version v1.x for Windows 7 or 8 - [link](https://www.fosshub.com/Caesium-Image-Compressor-old.html))
- **MacOS**: 10.15+
- **Linux**: tested on Ubuntu 22.04 and Manjaro

Note: only 64bit versions are supported

### Installation
Head to the [releases' page](https://github.com/Lymphatus/caesium-image-compressor/releases) for the available downloads.
- **Windows**: installer and portable versions are available
- **MacOS**: DMG package
- **Linux**: compile the source code yourself, or download binary from [third-party build](https://github.com/larygwil/caesium-image-compressor/releases)

Note that the main branch can contain unstable code. If you want to build on a stable version, use a tagged version.

### Troubleshooting and/or feature request
Please open an [issue](https://github.com/Lymphatus/caesium-image-compressor/issues).

### Build from source
##### Requirements
- [Rust](https://www.rust-lang.org/tools/install): required to compile [libcaesium](https://github.com/Lymphatus/libcaesium). Make sure you have `cargo` executable on you `$PATH`
- [Qt6 SDK](https://www.qt.io/download/): binaries are built on 6.5.1 (open source)
- [libssh](https://www.libssh.org/): macOS only
- [Sparkle](https://sparkle-project.org/): macOS only. Only version 1.27.1 is supported.

#### Build
##### Step 0 (macOS Only)
You need to set up Sparkle in order to compile the project
```
brew install --cask https://raw.githubusercontent.com/Homebrew/homebrew-cask/c6dfe6baf1639998ba1707f68668cf8fa97bac9d/Casks/sparkle.rb
sudo cp -R /usr/local/Caskroom/sparkle/1.27.1/Sparkle.framework /Library/Frameworks/Sparkle.framework
```
##### Step 1
You need to configure CMake first and the command is slightly different for all the platforms:
Change the path in variables with the correct directories of the requirements.
###### Windows
```
cmake -B build_dir -DCMAKE_PREFIX_PATH=/path/to/Qt/version -G "MinGW Makefiles"
```
###### MacOS
```
cmake -B build_dir -DCMAKE_PREFIX_PATH=/path/to/Qt/version/macos -DLIBSSH_INCLUDE_DIR=/libssh/dir/include -DSPARKLE_INCLUDE_DIR=/usr/local/Caskroom/sparkle/1.27.1/Sparkle.framework/Versions/Current/Headers
```
###### Linux
Make sure you have all the requirements installed with you own package manager
```
cmake -B build_dir -DCMAKE_PREFIX_PATH=/path/to/Qt/version/gcc_64
```
##### Step 2
Then you can build with
```
cmake --build build_dir --config Release --target caesium_image_compressor
```

### Languages and translations
The 1.0 version of Caesium had a lot of community made translations, but this 2.0 release has a whole new set of strings and no old translations can be used anymore.   
Caesium 2.0 is currently available in:
- <img src="https://flagcdn.com/h20/eg.png" srcset="https://flagcdn.com/h40/eg.png 2x" height="12" alt="EG"> ar_EG ![82%](https://progress-bar.dev/82)
- <img src="https://flagcdn.com/h20/de.png" srcset="https://flagcdn.com/h40/de.png 2x" height="12" alt="DE"> de_DE ![93%](https://progress-bar.dev/93)
- <img src="https://flagcdn.com/h20/gr.png" srcset="https://flagcdn.com/h40/gr.png 2x" height="12" alt="GR"> el_GR ![98%](https://progress-bar.dev/98)
- <img src="https://flagcdn.com/h20/us.png" srcset="https://flagcdn.com/h40/us.png 2x" height="12" alt="US"> en_US ![100%](https://progress-bar.dev/100)
- <img src="https://flagcdn.com/h20/es.png" srcset="https://flagcdn.com/h40/es.png 2x" height="12" alt="ES"> es_ES ![99%](https://progress-bar.dev/99)
- <img src="https://flagcdn.com/h20/fr.png" srcset="https://flagcdn.com/h40/fr.png 2x" height="12" alt="FR"> fr_FR ![83%](https://progress-bar.dev/83)
- <img src="https://flagcdn.com/h20/id.png" srcset="https://flagcdn.com/h40/id.png 2x" height="12" alt="ID"> id_ID ![98%](https://progress-bar.dev/98)
- <img src="https://flagcdn.com/h20/it.png" srcset="https://flagcdn.com/h40/it.png 2x" height="12" alt="IT"> it_IT ![100%](https://progress-bar.dev/100)
- <img src="https://flagcdn.com/h20/jp.png" srcset="https://flagcdn.com/h40/jp.png 2x" height="12" alt="JP"> ja_JP ![88%](https://progress-bar.dev/88)
- <img src="https://flagcdn.com/h20/pl.png" srcset="https://flagcdn.com/h40/pl.png 2x" height="12" alt="PL"> pl_PL ![96%](https://progress-bar.dev/96)
- <img src="https://flagcdn.com/h20/br.png" srcset="https://flagcdn.com/h40/br.png 2x" height="12" alt="BR"> pt_BR ![88%](https://progress-bar.dev/88)
- <img src="https://flagcdn.com/h20/ru.png" srcset="https://flagcdn.com/h40/ru.png 2x" height="12" alt="RU"> ru_RU ![98%](https://progress-bar.dev/98)
- <img src="https://flagcdn.com/h20/sk.png" srcset="https://flagcdn.com/h40/sk.png 2x" height="12" alt="SK"> sk_SK ![96%](https://progress-bar.dev/96)
- <img src="https://flagcdn.com/h20/tr.png" srcset="https://flagcdn.com/h40/tr.png 2x" height="12" alt="TR"> tr_TR ![97%](https://progress-bar.dev/97)
- <img src="https://flagcdn.com/h20/cn.png" srcset="https://flagcdn.com/h40/cn.png 2x" height="12" alt="CN"> zh_CN ![98%](https://progress-bar.dev/98)
- <img src="https://flagcdn.com/h20/tw.png" srcset="https://flagcdn.com/h40/tw.png 2x" height="12" alt="TW"> zh_TW ![64%](https://progress-bar.dev/64)

#### How to contribute with your translation
You can help translate by opening the `resources/i18n/caesium_en_US.ts` with QtLinguist or any XML editor. Rename it to your language (e.g. `caesium_it_IT.ts`) and translate all the sentences. Then you can start a pull request with your translated file.  
See [this pull request](https://github.com/Lymphatus/caesium-image-compressor/pull/106) as an example.
Thanks to all the people who contributed.
