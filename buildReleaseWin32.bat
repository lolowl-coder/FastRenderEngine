md build
cd build
cmake -S .. -B .
cmake --build ./ --config Release -j %NUMBER_OF_PROCESSORS%
pause