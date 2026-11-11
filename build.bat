@echo off
rem Kompilacja w "Developer Command Prompt for Visual Studio"
if not exist build mkdir build
rc /nologo /fo build\app.res src\app.rc || exit /b 1
cl /nologo /std:c++17 /O2 /EHsc /MT /DUNICODE /D_UNICODE src\main.cpp build\app.res /Fo:build\ /Fe:build\ModDate.exe /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib comctl32.lib comdlg32.lib shell32.lib gdiplus.lib ole32.lib || exit /b 1
echo OK: build\ModDate.exe
