#!/bin/sh

mkdir build 2> /dev/null
pushd build > /dev/null

CommonCompilerFlags="
-MJ compile_commands.json
-fdiagnostics-absolute-paths
-Wno-deprecated-declarations
-Wno-microsoft-anon-tag
-Wno-switch
-fms-extensions
-framework AppKit
-framework OpenGL
"

clang -g $CommonCompilerFlags ../src/mac_main.m -o mac_main

ErrorCode=$?

# clang output needs to be wrapped in []
echo '['$'\n'"$(cat compile_commands.json)"$'\n'']' > compile_commands.json

popd > /dev/null

exit $ErrorCode
