cmake_minimum_required(VERSION 3.18)

set(CMAKE_PROJECT_INCLUDE_BEFORE /opt/devkitpro/cmake/devkitPPC.cmake)
project(wiisx VERSION 3.0 LANGUAGES C CXX ASM)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Type of build" FORCE)
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
		None Debug Release RelWithDebInfo MinSizeRel
	)
endif()

if (NOT WIISX_TARGET)
	set(WIISX_TARGET NintendoWii CACHE STRING "System name" FORCE)
	set_property(CACHE WIISX_TARGET PROPERTY
		STRINGS NintendoWii NintendoGameCube
	)
endif()

unset(CMAKE_SYSTEM_NAME)
set(CMAKE_SYSTEM_NAME ${WIISX_TARGET})

if (CMAKE_SYSTEM_NAME STREQUAL NintendoWii)
	set(OGC_SUBDIR wii)
	set(OGC_COMPILE_OPTIONS -mrvl)
else()
	set(OGC_SUBDIR cube)
	set(OGC_COMPILE_OPTIONS -mogc)
endif()

set(OGC_COMPILE_OPTIONS -mcpu=750 -meabi -mhard-float ${OGC_COMPILE_OPTIONS})
add_compile_options(${OGC_COMPILE_OPTIONS} -ffunction-sections)
add_link_options(${OGC_COMPILE_OPTIONS} -Wl,--gc-sections)

list(APPEND CMAKE_SYSTEM_PREFIX_PATH ${DEVKITPRO}/libogc2;${DEVKITPRO}/portlibs/ppc)
set(CMAKE_FIND_LIBRARY_CUSTOM_LIB_SUFFIX /${OGC_SUBDIR})

find_library(OGC_LIBRARIES ogc REQUIRED)
add_library(ogc STATIC IMPORTED)
set_target_properties(ogc PROPERTIES
	IMPORTED_LOCATION ${OGC_LIBRARIES}
	INTERFACE_COMPILE_DEFINITIONS $<$<PLATFORM_ID:NintendoWii>:WII>
)
target_include_directories(ogc INTERFACE
	${DEVKITPRO}/libogc2/include
	${DEVKITPRO}/libogc2/include/ogc
)

find_library(AESND_LIBRARIES aesnd REQUIRED)
find_library(FAT_LIBRARIES fat REQUIRED)
find_library(ISO9660_LIBRARIES iso9660 REQUIRED)
find_library(ZLIB_LIBRARIES z REQUIRED)
find_path(ZLIB_INCLUDE_DIR zlib.h REQUIRED)

if (CMAKE_SYSTEM_NAME STREQUAL NintendoWii)
	find_library(TINYSMB_LIBRARIES tinysmb REQUIRED)
	find_library(LIBDI_LIBRARIES di REQUIRED)
	find_library(LIBWIIUSE_LIBRARIES wiiuse REQUIRED)
	find_library(LIBBTE_LIBRARIES bte REQUIRED)
endif()

add_library(lightning OBJECT
	pcsx_rearmed/deps/lightning/lib/lightning.c
	pcsx_rearmed/deps/lightning/lib/jit_disasm.c
	pcsx_rearmed/deps/lightning/lib/jit_fallback.c
	pcsx_rearmed/deps/lightning/lib/jit_memory.c
	pcsx_rearmed/deps/lightning/lib/jit_names.c
	pcsx_rearmed/deps/lightning/lib/jit_note.c
	pcsx_rearmed/deps/lightning/lib/jit_print.c
	pcsx_rearmed/deps/lightning/lib/jit_rewind.c
	pcsx_rearmed/deps/lightning/lib/jit_size.c
)
target_include_directories(lightning PRIVATE
	pcsx_rearmed/include/lightning
)
target_include_directories(lightning PUBLIC
	pcsx_rearmed/deps/lightning/include
)
target_compile_definitions(lightning PRIVATE HAVE_MMAP=0)
set_target_properties(lightning PROPERTIES LINKER_LANGUAGE C)

add_library(lightrec OBJECT
	pcsx_rearmed/deps/lightrec/blockcache.c
	pcsx_rearmed/deps/lightrec/constprop.c
	pcsx_rearmed/deps/lightrec/disassembler.c
	pcsx_rearmed/deps/lightrec/emitter.c
	pcsx_rearmed/deps/lightrec/interpreter.c
	pcsx_rearmed/deps/lightrec/lightrec.c
	pcsx_rearmed/deps/lightrec/memmanager.c
	pcsx_rearmed/deps/lightrec/optimizer.c
	pcsx_rearmed/deps/lightrec/regcache.c
	pcsx_rearmed/deps/lightrec/tlsf/tlsf.c
)
target_include_directories(lightrec PRIVATE
	pcsx_rearmed/deps/lightrec
	pcsx_rearmed/include/lightrec
	pcsx_rearmed/include/lightning
)
target_compile_definitions(lightrec PRIVATE fprintf=lightrec_fprintf)
set_target_properties(lightrec PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(lightrec PUBLIC lightning)

option(WITH_LIGHTREC_THREADED_COMPILER "Enabled Lightrec's threaded compiler" OFF)
if (WITH_LIGHTREC_THREADED_COMPILER)
	target_sources(lightrec PRIVATE
		pcsx_rearmed/deps/lightrec/reaper.c
		pcsx_rearmed/deps/lightrec/recompiler.c
	)

	find_library(PTHREAD_LIBRARIES pthread REQUIRED)
	find_path(PTHREAD_INCLUDE_DIR pthread.h REQUIRED)

	target_include_directories(lightrec PRIVATE ${PTHREAD_INCLUDE_DIR})
	target_link_libraries(lightrec PUBLIC ${PTHREAD_LIBRARIES})

	target_compile_definitions(lightrec PRIVATE LIGHTREC_ENABLE_THREADED_COMPILER)
else()
	target_compile_definitions(lightrec PRIVATE LIGHTREC_ENABLE_THREADED_COMPILER=0)
endif()

add_library(lzma OBJECT
	pcsx_rearmed/deps/libchdr/deps/lzma-19.00/src/Alloc.c
	pcsx_rearmed/deps/libchdr/deps/lzma-19.00/src/Bra86.c
	pcsx_rearmed/deps/libchdr/deps/lzma-19.00/src/BraIA64.c
	pcsx_rearmed/deps/libchdr/deps/lzma-19.00/src/CpuArch.c
	pcsx_rearmed/deps/libchdr/deps/lzma-19.00/src/Delta.c
	pcsx_rearmed/deps/libchdr/deps/lzma-19.00/src/LzFind.c
	pcsx_rearmed/deps/libchdr/deps/lzma-19.00/src/Lzma86Dec.c
	pcsx_rearmed/deps/libchdr/deps/lzma-19.00/src/LzmaDec.c
	pcsx_rearmed/deps/libchdr/deps/lzma-19.00/src/LzmaEnc.c
	pcsx_rearmed/deps/libchdr/deps/lzma-19.00/src/Sort.c
)
set_target_properties(lzma PROPERTIES LINKER_LANGUAGE C)
target_compile_definitions(lzma PRIVATE _7ZIP_ST)
target_include_directories(lzma PUBLIC
	pcsx_rearmed/deps/libchdr/deps/lzma-19.00/include
)

add_library(libchdr OBJECT
	pcsx_rearmed/deps/libchdr/src/libchdr_bitstream.c
	pcsx_rearmed/deps/libchdr/src/libchdr_cdrom.c
	pcsx_rearmed/deps/libchdr/src/libchdr_chd.c
	pcsx_rearmed/deps/libchdr/src/libchdr_flac.c
	pcsx_rearmed/deps/libchdr/src/libchdr_huffman.c
)
set_target_properties(libchdr PROPERTIES LINKER_LANGUAGE C)
target_include_directories(libchdr PUBLIC
	pcsx_rearmed/deps/libchdr/include
	pcsx_rearmed/deps/libchdr/include/libchdr
)
target_include_directories(libchdr PRIVATE ${ZLIB_INCLUDE_DIR})
target_compile_definitions(libchdr INTERFACE HAVE_CHD)
target_link_libraries(libchdr PUBLIC lzma)

add_library(libpcsxcore OBJECT
	pcsx_rearmed/libpcsxcore/cdriso.c
	pcsx_rearmed/libpcsxcore/cdrom.c
	pcsx_rearmed/libpcsxcore/cheat.c
	pcsx_rearmed/libpcsxcore/database.c
	pcsx_rearmed/libpcsxcore/decode_xa.c
	pcsx_rearmed/libpcsxcore/disr3000a.c
	pcsx_rearmed/libpcsxcore/gte.c
	pcsx_rearmed/libpcsxcore/gte_divider.c
	pcsx_rearmed/libpcsxcore/mdec.c
	pcsx_rearmed/libpcsxcore/misc.c
	pcsx_rearmed/libpcsxcore/plugins.c
	pcsx_rearmed/libpcsxcore/ppf.c
	pcsx_rearmed/libpcsxcore/psxbios.c
	pcsx_rearmed/libpcsxcore/psxcommon.c
	pcsx_rearmed/libpcsxcore/psxcounters.c
	pcsx_rearmed/libpcsxcore/psxdma.c
	pcsx_rearmed/libpcsxcore/psxhle.c
	pcsx_rearmed/libpcsxcore/psxhw.c
	pcsx_rearmed/libpcsxcore/psxinterpreter.c
	pcsx_rearmed/libpcsxcore/psxmem.c
	pcsx_rearmed/libpcsxcore/r3000a.c
	pcsx_rearmed/libpcsxcore/sio.c
	pcsx_rearmed/libpcsxcore/socket.c
	pcsx_rearmed/libpcsxcore/spu.c
	pcsx_rearmed/libpcsxcore/new_dynarec/emu_if.c
	pcsx_rearmed/libpcsxcore/new_dynarec/events.c
	pcsx_rearmed/libpcsxcore/lightrec/plugin.c
)
target_include_directories(libpcsxcore PUBLIC
	pcsx_rearmed/libpcsxcore
	pcsx_rearmed/include
	pcsx_rearmed
)
target_include_directories(libpcsxcore PRIVATE pcsx_rearmed/deps/lightrec)
target_include_directories(libpcsxcore PUBLIC ${ZLIB_INCLUDE_DIR})
target_compile_definitions(libpcsxcore PUBLIC LIGHTREC LIGHTREC_CUSTOM_MAP NO_SOCKET
	$<IF:$<PLATFORM_ID:NintendoWii>,HW_RVL,HW_DOL>
)
set_target_properties(libpcsxcore PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(libpcsxcore PUBLIC ${ZLIB_LIBRARIES} libchdr)

get_filename_component(LIBGUI_RESOURCES_PATH Gamecube/libgui/resources ABSOLUTE)
configure_file(Gamecube/libgui/resources.s.cmakein libgui_resources.s @ONLY)
add_library(libgui OBJECT
	Gamecube/libgui/Button.cpp
	Gamecube/libgui/Component.cpp
	Gamecube/libgui/CursorManager.cpp
	Gamecube/libgui/FocusManager.cpp
	Gamecube/libgui/Frame.cpp
	Gamecube/libgui/GraphicsGX.cpp
	Gamecube/libgui/Gui.cpp
	Gamecube/libgui/GuiResources.cpp
	Gamecube/libgui/IPLFont.cpp
	Gamecube/libgui/Image.cpp
	Gamecube/libgui/InputManager.cpp
	Gamecube/libgui/InputStatusBar.cpp
	Gamecube/libgui/LoadingBar.cpp
	Gamecube/libgui/Logo.cpp
	Gamecube/libgui/MessageBox.cpp
	Gamecube/libgui/TextBox.cpp
	libgui_resources.s
)
target_include_directories(libgui PRIVATE Gamecube/libgui)
set_target_properties(libgui PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(libgui PUBLIC libpcsxcore ogc ${LIBDI_LIBRARIES})

get_filename_component(MENU_RESOURCES_PATH Gamecube/menu/resources ABSOLUTE)
configure_file(Gamecube/menu/MenuResources.s.cmakein menu_resources.s @ONLY)
add_library(menu OBJECT
	Gamecube/menu/ConfigureButtonsFrame.cpp
	Gamecube/menu/ConfigureInputFrame.cpp
	Gamecube/menu/CurrentRomFrame.cpp
	Gamecube/menu/FileBrowserFrame.cpp
	Gamecube/menu/LoadRomFrame.cpp
	Gamecube/menu/MainFrame.cpp
	Gamecube/menu/MenuContext.cpp
	Gamecube/menu/SettingsFrame.cpp
	menu_resources.s
)
set_target_properties(menu PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(menu PUBLIC libgui)

add_library(filebrowser OBJECT
	Gamecube/fileBrowser/fileBrowser-CARD.c
	Gamecube/fileBrowser/fileBrowser-SMB.c
	Gamecube/fileBrowser/fileBrowser-libfat.c
	Gamecube/fileBrowser/fileBrowser.c
)
set_target_properties(filebrowser PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(filebrowser PUBLIC libgui ogc)

add_library(dfsound OBJECT
	pcsx_rearmed/plugins/dfsound/adsr.c
	pcsx_rearmed/plugins/dfsound/dma.c
	pcsx_rearmed/plugins/dfsound/freeze.c
	pcsx_rearmed/plugins/dfsound/nullsnd.c
	pcsx_rearmed/plugins/dfsound/out.c
	pcsx_rearmed/plugins/dfsound/registers.c
	pcsx_rearmed/plugins/dfsound/reverb.c
	pcsx_rearmed/plugins/dfsound/spu.c
	pcsx_rearmed/plugins/dfsound/xa.c
)
set_target_properties(dfsound PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(dfsound PUBLIC ogc)
target_compile_definitions(dfsound PRIVATE HAVE_OSS)

option(GX_GPU "GX accelerated GPU plugin" OFF)

if (GX_GPU)
	add_library(gpu OBJECT
		PeopsGXGPU/about.c
		PeopsGXGPU/cfg.c
		PeopsGXGPU/draw.c
		PeopsGXGPU/fps.c
		PeopsGXGPU/gpu.c
		PeopsGXGPU/gpuPeopsOpenGL.c
		PeopsGXGPU/key.c
		PeopsGXGPU/menu.c
		PeopsGXGPU/prim.c
		PeopsGXGPU/soft.c
		PeopsGXGPU/ssave.c
		PeopsGXGPU/texture.c
		PeopsGXGPU/zn.c
		PeopsGXGPU/opengx/gc_gl.c
		PeopsGXGPU/opengx/image_DXT.c
	)
	target_compile_definitions(gpu PRIVATE GX_GPU)
else()
	add_library(gpu OBJECT
		PeopsSoftGPU/DrawString.c
		PeopsSoftGPU/cfg.c
		PeopsSoftGPU/drawGX.c
		PeopsSoftGPU/fps.c
		PeopsSoftGPU/gpu.c
		PeopsSoftGPU/gpupeopssoft.c
		PeopsSoftGPU/key.c
		PeopsSoftGPU/menu.c
		PeopsSoftGPU/prim.c
		PeopsSoftGPU/soft.c
	)
endif()
set_target_properties(gpu PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(gpu PUBLIC ogc libgui)
target_compile_definitions(gpu PRIVATE __GX__ _SDL)

add_executable(wiisx
	Gamecube/aesnd.c
	Gamecube/DEBUG.c
	Gamecube/PlugGPU.c
	Gamecube/PlugPAD.c
	Gamecube/TEXT.c
	Gamecube/gc_plugin.c
	Gamecube/profile.c
	Gamecube/sysconf.c
	Gamecube/xxhash.c
	Gamecube/GamecubeMain.cpp
	Gamecube/GamecubePlugins.c
	Gamecube/vm/vm.c
	Gamecube/vm/dsihandler.s
	Gamecube/gc_input/controller-Classic.c
	Gamecube/gc_input/controller-GC.c
	Gamecube/gc_input/controller-WiimoteNunchuk.c
)
set_source_files_properties(Gamecube/vm/dsihandler.s PROPERTIES
	COMPILE_DEFINITIONS _LANGUAGE_ASSEMBLY
	COMPILE_OPTIONS -xassembler-with-cpp
)
target_include_directories(wiisx PRIVATE
	Gamecube
	${CMAKE_SOURCE_DIR}
)
set_target_properties(wiisx PROPERTIES
	OUTPUT_NAME $<IF:$<PLATFORM_ID:NintendoWii>,WiiSX,CubeSX>$<$<BOOL:${GX_GPU}>:_GX>
)

target_link_libraries(wiisx PRIVATE
	menu libgui filebrowser dfsound libpcsxcore libchdr lzma lightrec lightning gpu
	${AESND_LIBRARIES} ${FAT_LIBRARIES} ${ISO9660_LIBRARIES}
	${TINYSMB_LIBRARIES} ${LIBDI_LIBRARIES} ${LIBWIIUSE_LIBRARIES} ${LIBBTE_LIBRARIES}
)
target_compile_definitions(wiisx PRIVATE GEKKO)
if (${CMAKE_BUILD_TYPE} STREQUAL Debug)
	target_compile_definitions(wiisx PRIVATE MAP_OFFSET=0x10000000)
endif()

find_program(ELF2DOL_EXE NAMES elf2dol HINTS "${DEVKITPRO}/tools/bin")
if (NOT ELF2DOL_EXE)
    message(FATAL_ERROR "Could not find elf2dol: try installing gamecube-tools")
endif()

add_custom_command(TARGET wiisx POST_BUILD
	COMMAND ${ELF2DOL_EXE}
	$<TARGET_FILE:wiisx>
	$<TARGET_FILE_BASE_NAME:wiisx>.dol
)
