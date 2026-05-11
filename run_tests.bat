@echo off
set QT_PATH=C:\Qt\6.10.1\mingw_64
set MINGW_PATH=C:\Qt\Tools\mingw1310_64
set NINJA_PATH=C:\tmp\mingw64\bin
set CMAKE_PATH=C:\Qt\Tools\CMake_64\bin

set PATH=%QT_PATH%\bin;%MINGW_PATH%\bin;%NINJA_PATH%;%CMAKE_PATH%;%PATH%

if not exist build_test mkdir build_test
cd build_test

echo [1/3] Configuring...
cmake -G "Ninja" -DCMAKE_PREFIX_PATH="%QT_PATH%" -DCMAKE_MAKE_PROGRAM="%NINJA_PATH%\ninja.exe" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON ..
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

echo [2/3] Building...
ninja unit_tests integration_tests
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

echo [3/3] Running tests...
rem GoogleTest (XML output)
unit_tests.exe --gtest_output=xml:junit_unit.xml

rem Qt Test (XML output)
integration_tests.exe -xml -o junit_integration.xml

echo.
echo All tests executed. XML reports are in build_test/
