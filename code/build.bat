@echo off

mkdir "../build"
pushd "../build"

rem del "3dsand.exe"

rem del "preprocess_shaders.exe"
rem clang -ftime-trace -Wno-deprecated-declarations -Wno-braced-scalar-init -Wno-c++11-narrowing -Wno-writable-strings -g -gcodeview -O2 -mavx -mavx2 -mmmx -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -o "preprocess_shaders.exe" -D DEBUG -I "C:/Users/Kyler/lib/" "../code/preprocess_shaders.cpp"

if not errorlevel 1 (
    echo preprocessing shaders...
    "preprocess_shaders.exe"
    echo done preprocessing shaders
) else (
    EXIT /B 1
)

echo compiling...

clang -ftime-trace -Wno-deprecated-declarations -Wno-braced-scalar-init -Wno-c++11-narrowing -Wno-writable-strings -ferror-limit=-1 -g -gcodeview -O2 -mavx -mavx2 -mmmx -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -o "3dsand.exe" -D DEBUG -I "%HOME%/lib/" -I "%HOME%/lib/imgui/" -I "%HOME%/lib/imgui/backends/" -I "%HOME%/lib/tracy/public/" "../code/win32_gl_main.cpp" -lUser32.lib -lGdi32.lib -lopengl32.lib -lOle32.lib
rem imgui sources
rem "%HOME%/lib/imgui/imgui*.cpp" "%HOME%/lib/imgui/backends/imgui_impl_opengl3.cpp" "%HOME%/lib/imgui/backends/imgui_impl_win32.cpp"

echo done compiling

if not errorlevel 1 (
    echo running...
    echo.
    "3dsand.exe"
) else (
    EXIT /B 1
)
popd
