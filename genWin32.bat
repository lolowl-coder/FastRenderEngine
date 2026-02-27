md build
cd build
cmake -S .. -B . -DCMAKE_CUDA_FLAGS="-allow-unsupported-compiler"
pause