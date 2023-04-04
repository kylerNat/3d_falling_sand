@echo off

mkdir "build"

rem del "3dsand.exe"

echo compiling...

set OPTIMIZATION_LEVEL=-O2
set DEFINES=-D DEBUG -D DEV_CHEATS
rem -D SHOW_GL_NOTICES

set FLAGS=-ftime-trace -Wno-deprecated-declarations -Wno-braced-scalar-init -Wno-c++11-narrowing -Wno-writable-strings -ferror-limit=0 -g -gcodeview -mavx -mavx2 -mmmx -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -Xlinker /subsystem:windows

set OPTIONS=%FLAGS% %OPTIMIZATION_LEVEL% %DEFINES%

set INCLUDES=-I "%HOME%/lib/" -I "%HOME%/lib/zlib/" -I "%HOME%/lib/lz4/include" -I "%HOME%/lib/imgui/" -I "%HOME%/lib/imgui/backends/" -I "%HOME%/lib/tracy/public/" -I "%HOME%/lib/sdl/include/"
set SOURCES="code/win32_gl_main.cpp"
rem imgui sources
rem "%HOME%/lib/imgui/imgui*.cpp" "%HOME%/lib/imgui/backends/imgui_impl_opengl3.cpp" "%HOME%/lib/imgui/backends/imgui_impl_win32.cpp"
set LIBS= -lUser32.lib -lGdi32.lib -lopengl32.lib -lOle32.lib -L"%HOME%/lib/zlib/" -lzlib.lib -DLZ4_DLL_IMPORT=1 "%HOME%/lib/lz4/dll/liblz4.dll.a"

clang %OPTIONS% -o "build/3dsand.exe" %INCLUDES% %SOURCES% %LIBS%

echo done compiling

if not errorlevel 1 (
    echo running...
    echo.
    "build/3dsand.exe"
) else (
    EXIT /B 1
)
