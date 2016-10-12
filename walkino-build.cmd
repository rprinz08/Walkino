@echo off
cls

set ZIP="C:\Program Files\7-Zip\7z.exe"
set DIST=.\walkino.zip

if exist %DIST% del /q %DIST%

%ZIP% a -tzip %DIST% ^
    .\readme.md ^
    .\misc\ ^
    .\walkino-remove.cmd .\walkino-remove.sh ^
    .\examples\Walkino\ ^
    .\hardware\walkera\ ^
    .\hardware\tools\walkera\

%ZIP% t %DIST%
