@echo off
setlocal

if not exist build mkdir build

where cl >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo Building with MSVC...
    cl /nologo /EHsc /std:c++17 /O2 /Iinclude tests\test_orderbook.cpp src\orderbook.cpp /Fe:build\test_orderbook.exe
    cl /nologo /EHsc /std:c++17 /O2 /Iinclude src\bench.cpp src\orderbook.cpp /Fe:build\bench.exe
    goto done
)

where g++ >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo Building with g++...
    g++ -std=c++17 -O3 -Iinclude tests/test_orderbook.cpp src/orderbook.cpp -o build/test_orderbook.exe
    g++ -std=c++17 -O3 -Iinclude src/bench.cpp src/orderbook.cpp -o build/bench.exe
    goto done
)

echo [ERROR] Neither MSVC (cl) nor g++ was found in PATH.
exit /b 1

:done
echo [SUCCESS] Binaries built in build\ directory.
endlocal
