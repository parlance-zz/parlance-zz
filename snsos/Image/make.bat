@echo off

%toolspath%\makedisk -bin 8388608 disk.bin
if errorlevel 1 goto error

%toolspath%\virtualinstall -bin disk.bin %bootpath%\mbr.bin %bootpath%\boot.bin %kernelpath%\kernel.bin 2097152
if errorlevel 1 goto error

goto end

:error

pause
exit

:end