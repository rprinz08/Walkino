@echo off
cls

set ZIP="C:\Program Files\7-Zip\7z.exe"
set DIST=.\walkino-legacy.zip

if exist %DIST% del /q %DIST%

%ZIP% a -tzip %DIST% ^
    .\readme.md ^
    .\misc\ ^
    .\walkino-remove-legacy.cmd ^
    .\examples\Walkino\ ^
    .\hardware\walkera\ ^
    .\hardware\tools\walkera\

%ZIP% t %DIST%
