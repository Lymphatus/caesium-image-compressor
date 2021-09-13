windeployqt.exe "%1\Caesium Image Compressor.exe" --dir package --release
copy /y "%1\Caesium Image Compressor.exe" "package\Caesium Image Compressor.exe"
copy /y "%1\bin\libexiv2.dll" "package\libexiv2.dll"
copy /y "C:\libcaesium\lib\libcaesium.dll" "package\caesium.dll"