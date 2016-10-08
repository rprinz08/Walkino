@echo off
cls

set ZIP="C:\Program Files\7-Zip\7z.exe"
set DIST=.\walkino.zip

if exist %DIST% del %DIST%

%ZIP% a -tzip %DIST% build.cmd .vscode\ readme.md misc\ walkino-build.cmd walkino-build.sh walkino-remove.cmd walkino-remove.sh examples\Walkino\ hardware\walkera\ hardware\tools\walkera\

%ZIP% t %DIST%
