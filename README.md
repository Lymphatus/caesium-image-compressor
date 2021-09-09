## Caesium - Image Compressor (Cs-133)
##### v2.0.0-beta.1 - Copyright &copy; Matteo Paonessa, 2021. All Rights Reserved.
##### ⚠️ WARNING This is a very early stage version! Don't use on important files without a backup and expect some major changes. ⚠️ 

![caesium_screenshot](https://user-images.githubusercontent.com/12133996/126871376-0816f6a8-eaa8-4ea8-a051-a7ddaf6c2e36.png)

----------

> Caesium is an image compression software aimed at helping photographers, bloggers, webmasters, businesses or casual users at storing, sending and sharing digital pictures.

----------

##### REQUIREMENTS
* [libcaesium](https://github.com/Lymphatus/libcaesium)
----------

##### TESTED PLATFORMS
* Windows 10 64bit
* MacOS X Big Sur (v11.5)

----------

##### INSTALLATION
You can find binaries for Windows and OS X in the [releases page](https://github.com/Lymphatus/caesium-image-compressor/releases)

For linux you must compile the source by yourself.

----------

##### BUILD FROM SOURCE
1. You need the [Qt5 SDK](https://www.qt.io/download/) installed on your platform.
2. Compile [libcaesium](https://github.com/Lymphatus/libcaesium-rust). Please refer to its own documentation for detailed instructions for your platform. IMPORTANT: since alpha.5 this project uses a unreleased version of libcaesium, made in Rust. You can still find the binaries in the release files.
3. Clone the repository using ``` git clone https://github.com/Lymphatus/caesium-image-compressor.git ```. Please also pull submodules. If it doesn't work somehow, you can download the latest snapshot [here](https://github.com/Lymphatus/caesium-image-compressor/archive/master.zip).
4. Open *CMakeLists.txt* on your favorite editor and compile it. 

----------

##### RESOURCES
* Caesium website - [http://saerasoft.com/caesium/](http://saerasoft.com/caesium/)
* Caesium Git Repository - [https://github.com/Lymphatus/caesium-image-compressor](https://github.com/Lymphatus/caesium-image-compressor)
* Author website - SaeraSoft - [http://saerasoft.com](http://saerasoft.com)
