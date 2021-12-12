#!/bin/bash
cd "$1" || exit
macdeployqt "Caesium Image Compressor.app" -always-overwrite -no-strip "$2"
rsync -a --delete /Library/Frameworks/Sparkle.framework "$1/Caesium Image Compressor.app/Contents/Frameworks/"