## Caesium (Cs-133)
##### v2.0.0-beta1 (build 20160630) - Copyright &copy; Matteo Paonessa, 2016. All Rights Reserved.
###### WARNING This version is not completed yet! Use at your own risk!

<img width="1111" alt="screen" src="https://cloud.githubusercontent.com/assets/12133996/15804910/c59544d4-2b19-11e6-97d3-ac36d6373889.png">

----------

> Caesium is an image compression software aimed at helping photographers, bloggers, webmasters, businesses or casual users at storing, sending and sharing digital pictures.

----------

##### REQUIREMENTS
* [mozjpeg](https://github.com/mozilla/mozjpeg)
* [zopfli](https://github.com/google/zopfli)
* [lodepng](https://github.com/lvandeve/lodepng)
* [exiv2](http://www.exiv2.org/)

----------

##### TESTED PLATFORMS
* Windows 10 64bit
* MacOSX El Capitan (v. 10.11.4)
* Arch Linux (Known issues)
* Ubuntu 16.04 (Known issues)

----------

##### INSTALLATION
Binaries will be available soon.

Linux source code must be compiled using Qt5.5.

----------

##### BUILD FROM SOURCE
1. You need the [Qt5 SDK](https://www.qt.io/download/) installed on you platform.
2. Compile [mozjpeg](https://github.com/mozilla/mozjpeg), [zopfli](https://github.com/google/zopfli) and [exiv2](http://www.exiv2.org/). Please refer to their own documentation for detailed instructions for your platform.
3. Clone the repository using ``` git clone https://github.com/Lymphatus/caesium-image-compressor.git ```. If it doesn't work somehow, you can download the latest snapshot [here](https://github.com/Lymphatus/caesium-image-compressor/archive/master.zip).
4. Open *Cs-133.pro* on Qt Creator and compile it. It should compile both Caesium and its Updater.
5. Should be done!

----------

##### KNOWN ISSUES
* Lodepng fails on linux with error 68 and causes crash.

----------

##### RESOURCES
* Caesium website - [http://saerasoft.com/caesium/](http://saerasoft.com/caesium/)
* Caesium Git Repository - [https://github.com/Lymphatus/caesium-image-compressor](https://github.com/Lymphatus/caesium-image-compressor)
* Author website - SaeraSoft - [http://saerasoft.com](http://saerasoft.com)
* Author Twitter [http://twitter.com/MatteoPaonessa](http://twitter.com/MatteoPaonessa)
