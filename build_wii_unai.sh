mkdir build_wii_unai
export CFLAGS="-O3 -fipa-pta -flto -DNDEBUG"
export CXXFLAGS="-O3 -fipa-pta -flto -DNDEBUG"
cmake -B build_wii_unai -DCMAKE_BUILD_TYPE=None -DWIISX_TARGET=NintendoWii -DGPU_PLUGIN=Unai
cmake --build build_wii_unai