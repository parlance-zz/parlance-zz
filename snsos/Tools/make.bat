@echo off

path=E:\Mingw64\bin

del padbin.exe
del makedisk.exe
del virtualinstall.exe

x86_64-w64-mingw32-g++.exe padbin.cpp -o padbin.exe
if errorlevel 1 goto error

x86_64-w64-mingw32-g++.exe makedisk.cpp -o makedisk.exe
if errorlevel 1 goto error

x86_64-w64-mingw32-g++.exe virtualinstall.cpp -o virtualinstall.exe
if errorlevel 1 goto error

goto end

:error

pause
exit

:end

