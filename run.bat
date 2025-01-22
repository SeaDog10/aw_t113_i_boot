@echo off

set tool_dir=.\tool

@%tool_dir%\xfel.exe version

@%tool_dir%\xfel.exe ddr t113-i

@%tool_dir%\xfel.exe write 0x40000000 .\build\t113.bin

@%tool_dir%\xfel.exe exec 0x40000000
