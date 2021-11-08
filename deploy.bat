windeployqt.exe "%1\Caesium Image Compressor.exe" --dir package --release
copy /y "%1\Caesium Image Compressor.exe" "package\Caesium Image Compressor.exe"
copy /y "C:\libcaesium\lib\libcaesium.dll" "package\caesium.dll"
copy /y "C:\exiv2\bin\exiv2.dll" "package\exiv2.dll"
copy /y "C:\winsparkle\x64\Release\WinSparkle.dll" "package\WinSparkle.dll"
