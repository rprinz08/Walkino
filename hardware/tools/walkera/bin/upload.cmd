@echo off

echo Walkera Uploader

rem for debugging list all command line arguments
rem for %%I IN (%*) DO ECHO %%I

set PRG=%~1
set KEY=%2
set PORT=%3
set BAUD=%4
set BIN=%~dp0

rem echo PRG=%PRG%, KEY=%KEY%, PORT=%PORT%, BAUD=%BAUD%
rem echo Current path=%cd%
rem echo Script path=%BIN%

rem "%BIN%\avr-objdump" -D -l --source -t -m avr "%PRG%.elf" > "%PRG%.dump.txt"
"%BIN%\hex2bin" -e fw "%PRG%.hex" >nul 2>&1
rem "%BIN%\xor" "%PRG%.fw" "%PRG%.bin" %KEY%
"%BIN%\up42.exe" -p %PORT% -b %BAUD% -N -8 -1 -k %KEY% -i "%PRG%.fw" -o "%PRG%.bin"

