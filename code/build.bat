@echo off

mkdir "../build"
pushd "../build"

del "3dsand.exe"
rem clang -ftime-trace -Wno-deprecated-declarations -Wno-braced-scalar-init -Wno-c++11-narrowing -Wno-writable-strings -g -gcodeview -ffast-math -O2 -mavx -mavx2 -mmmx -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -o "3dsand.exe" -D DEBUG -I "C:/Users/Kyler/lib/" "../code/win32_gl_main.cpp" -lUser32.lib -lGdi32.lib -lopengl32.lib -lOle32.lib
clang -ftime-trace -Wno-deprecated-declarations -Wno-braced-scalar-init -Wno-c++11-narrowing -Wno-writable-strings -g -gcodeview -O2 -mavx -mavx2 -mmmx -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -o "3dsand.exe" -D DEBUG -I "C:/Users/Kyler/lib/" "../code/win32_gl_main.cpp" -lUser32.lib -lGdi32.lib -lopengl32.lib -lOle32.lib

if not errorlevel 1 (
    echo running...
    echo.
    "3dsand.exe"
) else (
    EXIT /B 1
)
popd
