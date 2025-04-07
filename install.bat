cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=release -S . -B build
cmake --build build --config Release
cmake --install build --config Release
pause