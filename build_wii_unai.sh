mkdir build_wii_unai
export CFLAGS="-O3 -fipa-pta -DNDEBUG -DSHOW_DEBUG -DPRINTGECKO"
export CXXFLAGS="-O3 -fipa-pta -DNDEBUG -DSHOW_DEBUG -DPRINTGECKO"
cmake -B build_wii_unai -DCMAKE_BUILD_TYPE=None -DWIISX_TARGET=NintendoWii -DGPU_PLUGIN=Unai
cmake --build build_wii_unai