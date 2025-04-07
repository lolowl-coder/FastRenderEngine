setlocal

REM Force colored diagnostics
set CLICOLOR_FORCE=1
set CMAKE_COLOR_DIAGNOSTICS=ON

md build
cd build
cmake -S .. -B .
cmake --build ./ --config Debug -j %NUMBER_OF_PROCESSORS%

endlocal

pause