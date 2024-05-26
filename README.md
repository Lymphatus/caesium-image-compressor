## Caesium Image Compressor  [![](https://img.shields.io/static/v1?label=Sponsor&message=%E2%9D%A4&logo=GitHub&color=%23fe8e86)](https://github.com/sponsors/Lymphatus)

[![Build](https://github.com/Lymphatus/caesium-image-compressor/actions/workflows/build-qt.yml/badge.svg)](https://github.com/Lymphatus/caesium-image-compressor/actions/workflows/build-qt.yml)

Try it directly on browser at [caesium.app](https://caesium.app)

###### v2.7.1

![caesium_screenshot_20240413](https://github.com/Lymphatus/caesium-image-compressor/assets/12133996/c09f06ab-abe2-4fa6-8c43-29b7073cb6df)

### Special Thanks
<a href="https://github.com/7eventech77"><img src="https://github.com/7eventech77.png" width="30px" alt="7eventech77" style="border-radius: 100%;" /></a>
<a href="https://github.com/ClearRabbit"><img src="https://github.com/ClearRabbit.png" width="30px" alt="ClearRabbit" style="border-radius: 100%;" /></a>

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
- [Qt6 SDK](https://www.qt.io/download/): binaries are built on 6.5.3 (open source)
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
- <img src="https://flagcdn.com/48x36/eg.png" width="24" height="18" alt="EG"> ![81%](https://progress-bar.dev/81/?title=ar_EG)
- <img src="https://flagcdn.com/48x36/de.png" width="24" height="18" alt="DE"> ![77%](https://progress-bar.dev/77/?title=de_DE)
- <img src="https://flagcdn.com/48x36/gr.png" width="24" height="18" alt="GR"> ![73%](https://progress-bar.dev/73/?title=el_GR)
- <img src="https://flagcdn.com/48x36/us.png" width="24" height="18" alt="US"> ![98%](https://progress-bar.dev/98/?title=en_US)
- <img src="https://flagcdn.com/48x36/es.png" width="24" height="18" alt="ES"> ![71%](https://progress-bar.dev/71/?title=es_ES)
- <img src="https://flagcdn.com/48x36/fi.png" width="24" height="18" alt="FI"> ![76%](https://progress-bar.dev/76/?title=fi_FI)
- <img src="https://flagcdn.com/48x36/fr.png" width="24" height="18" alt="FR"> ![72%](https://progress-bar.dev/72/?title=fr_FR)
- <img src="https://flagcdn.com/48x36/in.png" width="24" height="18" alt="IN"> ![75%](https://progress-bar.dev/75/?title=hi_IN)
- <img src="https://flagcdn.com/48x36/id.png" width="24" height="18" alt="ID"> ![73%](https://progress-bar.dev/73/?title=id_ID)
- <img src="https://flagcdn.com/48x36/it.png" width="24" height="18" alt="IT"> ![98%](https://progress-bar.dev/98/?title=it_IT)
- <img src="https://flagcdn.com/48x36/jp.png" width="24" height="18" alt="JP"> ![77%](https://progress-bar.dev/77/?title=ja_JP)
- <img src="https://flagcdn.com/48x36/pl.png" width="24" height="18" alt="PL"> ![73%](https://progress-bar.dev/73/?title=pl_PL)
- <img src="https://flagcdn.com/48x36/br.png" width="24" height="18" alt="BR"> ![98%](https://progress-bar.dev/98/?title=pt_BR)
- <img src="https://flagcdn.com/48x36/ru.png" width="24" height="18" alt="RU"> ![79%](https://progress-bar.dev/79/?title=ru_RU)
- <img src="https://flagcdn.com/48x36/sk.png" width="24" height="18" alt="SK"> ![72%](https://progress-bar.dev/72/?title=sk_SK)
- <img src="https://flagcdn.com/48x36/tr.png" width="24" height="18" alt="TR"> ![73%](https://progress-bar.dev/73/?title=tr_TR)
- <img src="https://flagcdn.com/48x36/cn.png" width="24" height="18" alt="CN"> ![100%](https://progress-bar.dev/100/?title=zh_CN)
- <img src="https://flagcdn.com/48x36/tw.png" width="24" height="18" alt="TW"> ![76%](https://progress-bar.dev/76/?title=zh_TW)

#### How to contribute with your translation
You can help translate by opening the `resources/i18n/caesium_en_US.ts` with QtLinguist or any XML editor. Rename it to your language (e.g. `caesium_it_IT.ts`) and translate all the sentences. Then you can start a pull request with your translated file.  
See [this pull request](https://github.com/Lymphatus/caesium-image-compressor/pull/106) as an example.
Thanks to all the people who contributed.
