#!/bin/sh
# Kompilacja krzyżowa (Linux -> Windows) przez MinGW-w64
set -e
cd "$(dirname "$0")"
mkdir -p build
x86_64-w64-mingw32-windres src/app.rc -O coff -o build/app.res
x86_64-w64-mingw32-g++ -std=c++17 -O2 -s -municode -mwindows -static \
  src/main.cpp build/app.res -o build/ModDate.exe \
  -lcomctl32 -lcomdlg32 -lshell32 -lgdiplus -lole32
echo "OK: build/ModDate.exe"
