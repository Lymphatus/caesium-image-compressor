# Caesium Image Compressor

[![](https://img.shields.io/static/v1?label=Sponsor&message=%E2%9D%A4&logo=GitHub&color=%23fe8e86)](https://github.com/sponsors/Lymphatus)

v3.0.0-alpha.0

Try it online at [caesium.app](https://caesium.app)

> [!WARNING]
> This is a very early development version and very much subject to change. Many features are missing or straight up don't work. Please use with care and be
> sure to back up your pictures before using it.


<img width="1431" height="954" alt="Image" src="https://github.com/user-attachments/assets/1b4ac0a4-2fc4-41c7-ab51-6e04a4ba9eae" />

## About

**Caesium Image Compressor** is a free, open-source, modern tool for compressing and optimizing images. Caesium helps
you store, send, and share digital pictures by dramatically reducing their file size while preserving visual quality.
The application supports JPG, PNG, WebP and TIFF formats, as well as batch processing, resizing, and much more.

Caesium v3 is a complete rewrite using [Tauri](https://tauri.app/) and a React-based interface.

## Features

- Compress images (JPG, PNG, WebP, TIFF) with adjustable quality
- Batch processing and folder drag-and-drop
- Preview before/after compression
- Retain or remove EXIF metadata
- Easily resize images
- Adjustable multi-core performance
- Modern, easy-to-use interface
- Multilingual support
- Fully cross-platform: Windows, macOS, and Linux

> **Note:** v3 is a major upgrade; you can contribute your suggestions and join the conversation
> in [this discussion thread](https://github.com/Lymphatus/caesium-image-compressor/issues/334).

## Supported Platforms

- **Windows**: Windows 10 (20H2+), 11 (64 bit)
- **macOS**: 10.15+ (Apple Silicon & Intel)
- **Linux**: 64 bit, tested on Ubuntu 24.04

> _Older systems? Use [v1.x - Legacy Releases](https://www.fosshub.com/Caesium-Image-Compressor-old.html)_

## Installation

Go to the [releases page](https://github.com/Lymphatus/caesium-image-compressor/releases) for downloads.

- **Windows**: Installer and portable ZIP available
- **macOS**: Universal DMG
- **Linux**: AppImage (or build from source)

_The main branch may contain development or unstable features—prefer tagged releases for production._

## Build from Source

### Requirements

- [Tauri](https://tauri.app/start/prerequisites/)

### How to Build

```sh
# 1. Clone the repo
git clone [https://github.com/Lymphatus/caesium-image-compressor.git)
cd caesium-image-compressor
# 2. Install JS dependencies
npm install
# 3. Build & run the app (development mode)
npm run tauri dev
# 4. Build production binaries
npm run tauri build
``` 

See Tauri and Rust documentation if you run into system dependencies or configuration issues.

## Languages & Translations

WIP

## Need Help? Feature Requests? Bugs?

- Open an [issue](https://github.com/Lymphatus/caesium-image-compressor/issues) - be sure to specify it's version 3
- Or join the [discussion thread for v3](https://github.com/Lymphatus/caesium-image-compressor/issues/334)

## Special Thanks

<a href="https://github.com/7eventech77"><img src="https://github.com/7eventech77.png" width="30px" alt="7eventech77" style="border-radius: 100%;" /></a>
<a href="https://github.com/ClearRabbit"><img src="https://github.com/ClearRabbit.png" width="30px" alt="ClearRabbit" style="border-radius: 100%;" /></a>
<a href="https://github.com/LushawnDev"><img src="https://github.com/LushawnDev.png" width="30px" alt="LushawnDev" style="border-radius: 100%;" /></a>
<a href="https://github.com/yannkost"><img src="https://github.com/yannkost.png" width="30px" alt="yannkost" style="border-radius: 100%;" /></a>
<a href="https://github.com/stamminator"><img src="https://github.com/stamminator.png" width="30px" alt="stamminator" style="border-radius: 100%;" /></a>

## License

[MIT](LICENSE.md)


