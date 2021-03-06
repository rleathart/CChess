@echo off
setlocal

if NOT EXIST build (
  mkdir build
)
pushd build

:: TODO(robin): Get rid of CRT

set PlatformLibs=kernel32.lib user32.lib gdi32.lib opengl32.lib

set CommonCompilerFlags=-FC -Zi -Od -nologo

cl %CommonCompilerFlags% ..\src\win64_main.c -link %PlatformLibs%

set CLError=%ERRORLEVEL%

popd build

EXIT /B %CLError%
