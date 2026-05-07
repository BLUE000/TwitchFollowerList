@echo off
set QT_PATH=C:\Qt\6.10.1\mingw_64
set MINGW_PATH=C:\Qt\Tools\mingw1310_64
set NINJA_PATH=C:\tmp\mingw64\bin
set CMAKE_PATH=C:\Qt\Tools\CMake_64\bin

echo Setting up environment...
set PATH=%QT_PATH%\bin;%MINGW_PATH%\bin;%NINJA_PATH%;%CMAKE_PATH%;%PATH%

if not exist build_cmd mkdir build_cmd
cd build_cmd

echo Running CMake...
cmake -G "Ninja" -DCMAKE_PREFIX_PATH="%QT_PATH%" -DCMAKE_MAKE_PROGRAM="%NINJA_PATH%\ninja.exe" -DCMAKE_BUILD_TYPE=Debug ..
if %ERRORLEVEL% neq 0 (
    echo CMake failed.
    pause
    exit /b %ERRORLEVEL%
)

echo Building...
ninja
if %ERRORLEVEL% neq 0 (
    echo Build failed.
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo Build Successful!
echo Running windeployqt to prepare DLLs...
windeployqt ..\bin\Debug\FollowerList.exe

echo.
echo All Done. You can find the executable in bin/Debug/FollowerList.exe
pause
