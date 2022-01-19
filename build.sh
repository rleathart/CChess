#!/bin/sh

mkdir build 2> /dev/null
pushd build > /dev/null

CommonCompilerFlags="
-fdiagnostics-absolute-paths
-Wno-microsoft-anon-tag
-Wno-switch
-fms-extensions
-framework AppKit
-framework OpenGL
"

clang -g $CommonCompilerFlags ../src/mac_main.m -o mac_main

ErrorCode=$?

popd > /dev/null

exit $ErrorCode
