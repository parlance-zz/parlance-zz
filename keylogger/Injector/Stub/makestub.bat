path=e:\masm32\nasm

nasm stub.asm -o stub.bin
if errorlevel 1 goto error

goto end

:error

pause
exit

:end
