REM fmt lib
COPY ..\..\Binaries\fmt\vs2017\Debug\*.lib vendor\lib\vs2017\Win32\Debug
COPY ..\..\Binaries\fmt\vs2017\Release\*.lib vendor\lib\vs2017\Win32\Release

COPY ..\..\Binaries\fmt\vs2022\Debug\*.lib vendor\lib\vs2022\x64\Debug
COPY ..\..\Binaries\fmt\vs2022\Release\*.lib vendor\lib\vs2022\x64\Release

REM FreeImage 2017
COPY ..\..\Binaries\FreeImage\vs2017\bin\x86\Debug\*.dll vendor\bin\vs2017\Win32\Debug
COPY ..\..\Binaries\FreeImage\vs2017\bin\x86\Release\*.dll vendor\bin\vs2017\Win32\Release

COPY ..\..\Binaries\FreeImage\vs2017\lib\x86\Debug\*.lib vendor\lib\vs2017\Win32\Debug
COPY ..\..\Binaries\FreeImage\vs2017\lib\x86\Release\*.lib vendor\lib\vs2017\Win32\Release

REM FreeImage 2022
COPY ..\..\Binaries\FreeImage\vs2022\bin\x64\Debug\*.dll vendor\bin\vs2022\x64\Debug
COPY ..\..\Binaries\FreeImage\vs2022\bin\x64\Release\*.dll vendor\bin\vs2022\x64\Release

COPY ..\..\Binaries\FreeImage\vs2022\lib\x64\Debug\*.lib vendor\lib\vs2022\x64\Debug
COPY ..\..\Binaries\FreeImage\vs2022\lib\x64\Release\*.lib vendor\lib\vs2022\x64\Release

