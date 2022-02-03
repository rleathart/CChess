#!/bin/sh

mkdir build 2> /dev/null
pushd build > /dev/null

if [ $(uname) == "Linux" ]; then
  MainSource="../src/linux_x11_main.c"
  MainExe="linux_x11_main"
  LinuxCompilerFlags="
  -lX11
  -lGL
  "
else
  MainSource="../src/mac_main.m"
  MainExe="mac_main"
  MacCompilerFlags="
  -framework AppKit
  -framework OpenGL
  "
fi

CommonCompilerFlags="
-g
-fdiagnostics-absolute-paths
-Wno-deprecated-declarations
-Wno-microsoft-anon-tag
-Wno-switch
-fms-extensions
"

clang -g $LinuxCompilerFlags $MacCompilerFlags \
  $CommonCompilerFlags $MainSource -o $MainExe

ErrorCode=$?

popd > /dev/null

exit $ErrorCode
