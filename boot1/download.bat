@echo off

set tool_dir=.\tool

@%tool_dir%\xfel.exe version

@%tool_dir%\xfel.exe spinand erase 0x100000 0x100000

@%tool_dir%\xfel.exe spinand write 0x100000 .\build\t113.bin
