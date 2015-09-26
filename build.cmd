@echo off
cls

set ZIP="C:\Program Files\7-Zip\7z.exe"
set DIST=.\walkera.zip

if exist %DIST% del %DIST% 

%ZIP% a -tzip %DIST% build.cmd .vscode\ hardware\walkera\ hardware\tools\walkera\

%ZIP% t %DIST%
