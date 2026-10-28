cmake_minimum_required(VERSION 3.22)
project(ReKHCoMAndroid C CXX ASM)
if(NOT ANDROID OR (NOT ANDROID_ABI STREQUAL "armeabi-v7a" AND NOT ANDROID_ABI STREQUAL "x86" AND NOT ANDROID_ABI STREQUAL "arm64-v8a"))
    message(FATAL_ERROR "Android ABIs: armeabi-v7a and x86 (the 32-bit core), arm64-v8a (the 64-bit host, ANDROID_64.md)")
endif()
get_filename_component(ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
if(NOT SDL_SOURCE_DIR)
    message(FATAL_ERROR "Set SDL_SOURCE_DIR to a verified SDL3 source checkout")
endif()
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory("${SDL_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/SDL")
if(NOT FREETYPE_SOURCE_DIR)
    message(FATAL_ERROR "Set FREETYPE_SOURCE_DIR to the pinned FreeType checkout")
endif()
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
add_subdirectory("${FREETYPE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/freetype")
# The launcher: the PC launcher's own code (port/decompui, LAUNCHER_ANDROID) as the app's first screen
# (LauncherActivity), on every ABI; the game runs in GameActivity's process
set(RC "${ROOT}/port/third_party/rcheevos/src")
file(GLOB RCHEEVOS_SOURCES "${RC}/rcheevos/*.c" "${RC}/rapi/*.c" "${RC}/rhash/*.c")
set(IMGUI "${ROOT}/port/decompui/third_party/imgui")
add_library(launcher SHARED "${ROOT}/port/decompui/decompui.cpp"
    "${IMGUI}/imgui.cpp" "${IMGUI}/imgui_draw.cpp" "${IMGUI}/imgui_tables.cpp" "${IMGUI}/imgui_widgets.cpp"
    "${IMGUI}/backends/imgui_impl_sdl3.cpp" "${IMGUI}/backends/imgui_impl_opengl3.cpp"
    "${ROOT}/port/achievements/ra_client.c" "${ROOT}/port/achievements/offline_achievements.c"
    "${ROOT}/port/decompui/miniz_unit.c" "${RC}/rc_client.c" "${RC}/rc_compat.c" "${RC}/rc_util.c" "${RC}/rc_version.c"
    ${RCHEEVOS_SOURCES})
target_compile_definitions(launcher PRIVATE LAUNCHER_ANDROID IMGUI_IMPL_OPENGL_ES3 RC_CLIENT_SUPPORTS_HASH)
target_include_directories(launcher PRIVATE "${IMGUI}" "${ROOT}/port/decompui/third_party"
    "${ROOT}/port/third_party/rcheevos/include" "${ROOT}/port/include")
target_compile_options(launcher PRIVATE -O2 -w $<$<COMPILE_LANGUAGE:CXX>:-std=c++17>)
target_link_libraries(launcher PRIVATE SDL3::SDL3 GLESv3 EGL log android m)

if(ANDROID_ABI STREQUAL "arm64-v8a")
    # 64-bit: the game is the AArch64 image with 32-bit pointers (port/android64/build_android64.py, in the APK's
    # assets); this library is only its host
    add_library(rekhcom64 SHARED "${ROOT}/port/android64/host/android64_host.c" "${ROOT}/port/portable/hdmovie_android.c")
    target_include_directories(rekhcom64 PRIVATE "${ROOT}/port/ps5/shared")
    target_compile_options(rekhcom64 PRIVATE -O2 -Wall)
    target_link_libraries(rekhcom64 PRIVATE SDL3::SDL3 freetype log android m)
    return()
endif()
set(REGIONS "eu" CACHE STRING "Cartridge regions to compile: eu;us;jp")
option(REKHCOM_UBSAN "Test build: log shifts by 32+, divisions by zero, NULL and misaligned accesses (ubsan_log.c)" OFF)
set(UBSAN_FLAGS -fsanitize=shift,integer-divide-by-zero,null,alignment -fsanitize-minimal-runtime -fsanitize-recover=all)
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS "${ROOT}/src/*.c")
if(ANDROID_ABI STREQUAL "armeabi-v7a")
    # Cartridge arithmetic belongs to game C units only. Do not wrap SDL,
    # launcher/importer, portable code, or the arm64 host's compiler.
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    set(CARTRIDGE_COMPILER "${ROOT}/port/tools/compile_cartridge_game.py")
    set(CARTRIDGE_LOWERER "${ROOT}/port/tools/cartridge_division_ir.py")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${CARTRIDGE_COMPILER}" "${CARTRIDGE_LOWERER}")
    execute_process(COMMAND "${Python3_EXECUTABLE}" "${CARTRIDGE_COMPILER}" --print-fingerprint
        RESULT_VARIABLE CARTRIDGE_RESULT OUTPUT_VARIABLE CARTRIDGE_FINGERPRINT
        ERROR_VARIABLE CARTRIDGE_ERROR OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT CARTRIDGE_RESULT EQUAL 0)
        message(FATAL_ERROR "Cartridge compiler fingerprint failed: ${CARTRIDGE_ERROR}")
    endif()
    # Both content in the compiler command and source dependencies invalidate
    # old unguarded objects, including a build resumed after updating tools.
    set_property(SOURCE ${GAME_SOURCES} APPEND PROPERTY OBJECT_DEPENDS
        "${CARTRIDGE_COMPILER}" "${CARTRIDGE_LOWERER}")
endif()
file(GLOB PORT_SOURCES CONFIGURE_DEPENDS "${ROOT}/port/src/*.c")
foreach(name port_memory port_sections port_state port_hdtext port_watchdog port_biosfault)
    list(REMOVE_ITEM PORT_SOURCES "${ROOT}/port/src/${name}.c")
endforeach()
file(GLOB INCLUDE_DIRS LIST_DIRECTORIES true "${ROOT}/include/*")
set(GAME_INCLUDES "${ROOT}/include")
foreach(dir ${INCLUDE_DIRS})
    if(IS_DIRECTORY "${dir}" AND NOT dir MATCHES "/gba$")
        list(APPEND GAME_INCLUDES "${dir}")
    endif()
endforeach()
foreach(region IN LISTS REGIONS)
    string(TOUPPER "${region}" upper)
    set(gen "${ROOT}/build/${region}/gen")
    set(glue "${ROOT}/build/${region}/pc")
    file(READ "${glue}/rom_symbols.s" symbols)
    string(REPLACE ".section .rdata,\"dr\"" ".section .rodata" symbols "${symbols}")
    string(REPLACE ".globl _" ".globl " symbols "${symbols}")
    string(REPLACE ".set _" ".set " symbols "${symbols}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/rom_symbols_${region}.S" "${symbols}")
    file(READ "${ROOT}/port/src/port_symbols.s" fixed)
    string(REPLACE ".globl _" ".globl " fixed "${fixed}")
    string(REPLACE ".set _" ".set " fixed "${fixed}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/fixed_symbols_${region}.S" "${fixed}")
    add_library(game_${region} OBJECT ${GAME_SOURCES})
    if(ANDROID_ABI STREQUAL "armeabi-v7a")
        set_property(TARGET game_${region} PROPERTY C_COMPILER_LAUNCHER
            "${Python3_EXECUTABLE};${CARTRIDGE_COMPILER};--fingerprint;${CARTRIDGE_FINGERPRINT};--")
    endif()
    set_target_properties(game_${region} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_compile_options(game_${region} PRIVATE -O2 -w -std=gnu89 -funsigned-char -fno-strict-aliasing -fwrapv -fno-common
        -include "${ROOT}/port/include/port/game_prefix.h")
    target_compile_definitions(game_${region} PRIVATE PLATFORM_PC VERSION_${upper})
    if(REKHCOM_UBSAN)
        target_compile_options(game_${region} PRIVATE ${UBSAN_FLAGS})
    endif()
    # Game malloc.h must never shadow bionic's <malloc.h> from <stdlib.h>.
    foreach(dir ${GAME_INCLUDES} "${gen}" "${ROOT}/port/include")
        target_compile_options(game_${region} PRIVATE "-iquote${dir}")
    endforeach()
    add_library(rekhcom_${region} SHARED $<TARGET_OBJECTS:game_${region}> ${PORT_SOURCES}
        "${glue}/rom_relocs.c" "${CMAKE_CURRENT_BINARY_DIR}/rom_symbols_${region}.S"
        "${CMAKE_CURRENT_BINARY_DIR}/fixed_symbols_${region}.S"
        native/port_memory_android.c native/platform_android.c native/touch_android.c
        $<$<BOOL:${REKHCOM_UBSAN}>:native/ubsan_log.c>
        "${ROOT}/port/portable/hdtext_freetype.c" "${ROOT}/port/portable/hdmovie_android.c")
    target_compile_definitions(rekhcom_${region} PRIVATE PLATFORM_PC VERSION_${upper})
    if(REKHCOM_UBSAN)
        target_compile_options(rekhcom_${region} PRIVATE ${UBSAN_FLAGS})
    endif()
    target_compile_options(rekhcom_${region} PRIVATE -O2 -funsigned-char -fno-strict-aliasing -fwrapv)
    foreach(dir ${GAME_INCLUDES} "${gen}")
        target_compile_options(rekhcom_${region} PRIVATE "-iquote${dir}")
    endforeach()
    target_include_directories(rekhcom_${region} PRIVATE "${ROOT}/port/include")
    target_link_libraries(rekhcom_${region} PRIVATE SDL3::SDL3 freetype log android m)
    # Cartridge symbols are SHN_ABS (hardware addresses and integer constants).
    # Bind locally: older Android loaders otherwise add the shared-object bias
    # to GLOB_DAT absolute symbols, e.g. the music-player count 26.
    set_property(TARGET rekhcom_${region} APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/native/exports.map")
    target_link_options(rekhcom_${region} PRIVATE -Wl,-z,nostart-stop-gc -Wl,-Bsymbolic
        "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/native/exports.map")
endforeach()
