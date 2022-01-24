rmdir /s/y %2
windeployqt.exe "%1\Caesium Image Compressor.exe" --dir %2 --release
copy /y "%1\Caesium Image Compressor.exe" "%2\Caesium Image Compressor.exe"
copy /y "C:\libcaesium\lib\libcaesium.dll" "%2\caesium.dll"
copy /y "C:\exiv2\bin\exiv2.dll" "%2\exiv2.dll"
copy /y "C:\winsparkle\x64\Release\WinSparkle.dll" "%2\WinSparkle.dll"
