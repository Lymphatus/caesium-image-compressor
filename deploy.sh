#!/bin/bash
macdeployqt "$1/Caesium Image Compressor.app" -always-overwrite -no-strip
cp -RP /Library/Frameworks/Sparkle.framework "$1/Caesium Image Compressor.app/Contents/Frameworks/Sparkle.framework"