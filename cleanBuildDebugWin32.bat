rd /s /q build
md build
cd build
cmake -S .. -B .
cmake --build ./ --config Release
pause