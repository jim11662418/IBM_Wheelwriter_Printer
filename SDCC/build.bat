@echo off
echo compiling...
sdcc -c keyboard.c
sdcc -c uart12.c
sdcc -c watchdog.c
sdcc -c wheelwriter.c

echo linking...
sdcc main.c keyboard.rel uart12.rel watchdog.rel wheelwriter.rel

echo generating Intel HEX file...
packihx main.ihx > printer.hex

echo cleaning up...
REM optional cleanup...
del *.asm
del *.ihx
del *.lk
del *.lst
del *.map
del *.mem
del *.rel
del *.rst
del *.sym

pause