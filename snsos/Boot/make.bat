@echo off

path=e:\masm32\nasm

nasm mbr.asm -o mbr.bin
if errorlevel 1 goto error

nasm boot.asm -o boot.bin
if errorlevel 1 goto error

goto end

:error

pause
exit

:end