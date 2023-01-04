@echo off
cls

del /f /q /s .\.vscode
del /f /q /s .\misc
del /f /q /s .\examples\Walkino
del /f /q /s .\hardware\walkera
del /f /q /s .\hardware\tools\walkera

del /f /q .\readme.md ^
    .\walkino-build-legacy.cmd .\walkino-build-legacy.sh ^
    .\walkino-remove-legacy.cmd .\walkino-remove-legacy.sh
del /f /q .\walkino-legacy.zip

