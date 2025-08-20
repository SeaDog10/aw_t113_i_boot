@echo off

set tool_dir=.

@%tool_dir%\xfel.exe version

@%tool_dir%\xfel.exe ddr t113-i

@%tool_dir%\xfel.exe write 0x40100000 .\rtthread_m.bin

@%tool_dir%\xfel.exe write 0x40200000 .\rtthread_s.bin

@%tool_dir%\xfel.exe exec 0x40100000
