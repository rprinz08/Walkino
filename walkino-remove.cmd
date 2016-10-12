@echo off
cls

del /f /q /s .\.vscode
del /f /q /s .\misc
del /f /q /s .\examples\Walkino
del /f /q /s .\hardware\walkera
del /f /q /s .\hardware\tools\walkera

del /f /q .\readme.md ^
    .\walkino-build.cmd .\walkino-build.sh ^
    .\walkino-remove.cmd .\walkino-remove.sh
del /f /q .\walkino.zip
