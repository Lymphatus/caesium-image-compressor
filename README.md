## Caesium - Image Compressor (Cs-133)
> v2.0.0-beta.2 - Copyright &copy; Matteo Paonessa, 2021. All Rights Reserved.

###### ⚠️ WARNING This is a very early stage version! Don't use on important files without a backup and expect some major changes. ⚠️ 

![Screenshot](https://user-images.githubusercontent.com/12133996/136700618-e3b94328-e1c9-4bb0-9937-23e6e71e3ff1.PNG)

----------

> Caesium is an image compression software aimed at helping photographers, bloggers, webmasters, businesses or casual users at storing, sending and sharing digital pictures. Available for Windows, MacOS and Linux.

----------
#### Supported Platforms
- **Windows** 7/10/11
- **MacOS** >= 10.14
- **Linux** (tested on Ubuntu 20.04)

### Installation
Head to the  [releases page](https://github.com/Lymphatus/caesium-image-compressor/releases) for the available downloads.

No binaries for Linux are available.


### Build from source
##### Requirements
- [Qt6 SDK](https://www.qt.io/download/) - binaries are built on 6.2 (open source)
- [libcaesium](https://github.com/Lymphatus/libcaesium) - version >= 0.6.0
- [exiv2](https://github.com/exiv2/exiv2)
- [Sparkle](https://sparkle-project.org/) for MacOS or [WinSparkle](https://winsparkle.org/) for Windows
- [libssh](https://www.libssh.org/) - MacOS only

#### Build
##### Step 1
You need to configure CMake first and the command is slightly different for all the platforms:
###### Windows
Change the path in variables with the correct directories of the requirements.
```
cmake -B build_dir -DCMAKE_PREFIX_PATH=/path/to/Qt/version/msvc2019_64 -DLIBCAESIUM_LIB_DIR=/libcaesium/dir/lib -DEXIV2_LIB_DIR=/exiv2/dir/lib -DEXIV2_INCLUDE_DIR=/exiv2/dir/exiv2/include -DWINSPARKLE_INCLUDE_DIR=/winsparke/release/dir
```
###### MacOS
Change the path in variables with the correct directories of the requirements.
```
cmake -B build_dir -DCMAKE_PREFIX_PATH=/path/to/Qt/version/macos -DLIBCAESIUM_LIB_DIR=/libcaesium/dir/lib -DEXIV2_LIB_DIR=/exiv2/dir/lib -DEXIV2_INCLUDE_DIR=/exiv2/dir/exiv2/include -DSPARKLE_INCLUDE_DIR=/Library/Frameworks/Sparkle.framework/Headers -DLIBSSH_INCLUDE_DIR=/libssh/dir/include
```
###### Linux
Make sure you have all the requirements installed by you own package manager
```
cmake -B build_dir -DCMAKE_PREFIX_PATH=/path/to/Qt/version/gcc_64 -DLIBCAESIUM_LIB_DIR=/libcaesium/dir/lib
```
##### Step 2
Then you can build with
```
cmake --build build_dir --config Release --target caesium_image_compressor
```

### Translate
The first version of Caesium had a lot of community made translations, but this new release has a whole new set of strings and no old translations can be used anymore.   
You can help translate by opening the `resources/i18n/caesium_en_US.ts` with QtLinguist or any XML editor. Rename it to your language (e.g. `caesium_it_IT.ts`) and translate all the sentences. Then you can start a pull request with your translated file.  
You can use [this pull request](https://github.com/Lymphatus/caesium-image-compressor/pull/57) as an example.

### Resources
* Caesium website - [http://saerasoft.com/caesium/](https://saerasoft.com/caesium/)
* Caesium Git Repository - [https://github.com/Lymphatus/caesium-image-compressor](https://github.com/Lymphatus/caesium-image-compressor)
* Author website - SaeraSoft - [http://saerasoft.com](https://saerasoft.com)
