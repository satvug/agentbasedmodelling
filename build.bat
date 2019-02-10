@echo off

set CompilerFlags= -O2 -nologo -MT -Oi -W4 -wd4100 -wd4505 -wd4005 -wd4201 -wd4305 -wd4127 -wd4996 -FC -Zi
set LinkerFlags=user32.lib gdi32.lib opengl32.lib -incremental:no
set CurrentPath=%~dp0

if not defined DevEnvDir (
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"
cd %CurrentPath%)

pushd build
cl %CompilerFlags% ..\source\platform_win32.cpp /link %LinkerFlags% /out:abm.exe
popd

tools\font_packer.exe c:\Windows\Fonts\consola.ttf build/abm.exe
