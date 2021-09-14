#!/bin/bash
macdeployqt "$1/Caesium Image Compressor.app" -always-overwrite
cp -R /Library/Frameworks/Sparkle.framework "$1/Caesium Image Compressor.app/Contents/Frameworks/Sparkle.framework"