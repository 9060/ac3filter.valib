@echo off
del *.aps 2> nul
del *.ncb 2> nul
del *.plg 2> nul
del *.opt 2> nul
del *.dep 2> nul
rmdir /s /q debug 2> nul
rmdir /s /q perf 2> nul
del release\*.bsc 2> nul
del release\*.exp 2> nul
del release\*.idb 2> nul
del release\*.obj 2> nul
del release\*.pch 2> nul
del release\*.res 2> nul
del release\*.sbr 2> nul
del release\*.ilk 2> nul
del release\*.asm 2> nul
del release\*.cod 2> nul
del release\*.map 2> nul

rmdir /s /q debug_libc 2> nul
del release_libc\*.bsc 2> nul
del release_libc\*.exp 2> nul
del release_libc\*.idb 2> nul
del release_libc\*.obj 2> nul
del release_libc\*.pch 2> nul
del release_libc\*.res 2> nul
del release_libc\*.sbr 2> nul
del release_libc\*.ilk 2> nul
del release_libc\*.asm 2> nul
del release_libc\*.cod 2> nul
del release_libc\*.map 2> nul
