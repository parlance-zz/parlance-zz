@echo off

path=E:\Mingw64\bin

del kernel.o
del console.o
del memory.o
del kernel.exe
del kernel.bin

x86_64-w64-mingw32-g++.exe -funsigned-char -masm=intel -m64 -nostdinc -c console.c memory.c kernel.c
if errorlevel 1 goto error

x86_64-w64-mingw32-ld.exe -nostdlib -static -e __main -Ttext 0x100000 kernel.o console.o memory.o -o kernel.exe 
if errorlevel 1 goto error

x86_64-w64-mingw32-objcopy.exe --remove-section=.idata -O binary kernel.exe kernel.bin
if errorlevel 1 goto error

REM %toolspath%\disasm32-1.7.30.exe -b64 kernel.bin 100000 > kernel_disasm.asm

%toolspath%\padbin kernel.bin kernel.bin 131072

goto end

:error

pause
exit

:end

