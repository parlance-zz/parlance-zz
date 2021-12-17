@echo off

set bootpath=c:\programming\snsos\boot
set kernelpath=c:\programming\snsos\boot
set toolspath=c:\programming\snsos\tools
set imagepath=c:\programming\snsos\image
set emupath=e:\program files\qemu

chdir /d %bootpath%
%bootpath%\make

chdir /d %kernelpath%
%kernelpath%\make

chdir /d %toolspath%
%toolspath%\make

chdir /d %emupath%
%emupath%\qemu-system-x86_64.exe -L . -m 256 -hda %imagepath\disk.bin -soundhw all -localtime -M pc -smp 4 -net nic -net nic