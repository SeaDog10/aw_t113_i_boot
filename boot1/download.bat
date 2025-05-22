@echo off

set tool_dir=.\tool

@%tool_dir%\xfel.exe version

@%tool_dir%\xfel.exe spinand erase 0 0x100000

@%tool_dir%\xfel.exe spinand write 0x0 .\build\t113.bin
