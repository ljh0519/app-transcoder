cmake_minimum_required(VERSION 2.8)
project(app-transcoder)

# option(LIBEXECINFO_STANDALONE "if libexecinfo is standalone" OFF)
# message("Option LIBEXECINFO_STANDALONE=${LIBEXECINFO_STANDALONE}")
# set(ENV{THIRD_LIB_DIR} "/root/rtc-bridge/out/third_stage/lib/")

message("Option THIRD_LIB_DIR=$ENV{THIRD_LIB_DIR}")
message("Option PKG_CONFIG_PATH=$ENV{PKG_CONFIG_PATH}")
message("Option CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
message("Option CMAKE_SYSTEM_NAME = ${CMAKE_SYSTEM_NAME}")
message("Option APP_VER=${APP_VER}")

if (CMAKE_VERSION VERSION_LESS "3.1")
    add_definitions(-std=c++14)
else ()
    set(CMAKE_CXX_STANDARD 14)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif ()

set(CMAKE_VERBOSE_MAKEFILE yes)

if (APPLE) 
     set(CMAKE_MACOSX_RPATH ON) 
     set(CMAKE_SKIP_BUILD_RPATH TRUE) 
     set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE) 
     set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE) 
endif () 


add_definitions(-DFMT_HEADER_ONLY=1)
add_definitions(-DFMT_STRING_ALIAS=1)

include_directories(src)

include_directories(third)

set(TRANSFMT_SOURCES
    src/NLogger.cpp
    src/NLogger.hpp
    src/NMediaBasic.hpp
    src/NMediaFrame.hpp
    src/NPool.hpp
    src/NStr.hpp
    src/NVideoTranscoder.hpp
    src/NVideoTranscoder.cpp
    src/NRegion.hpp
    src/YUVMixer.hpp
    src/YUVMixer.cpp
    src/NTErrorDefined.hpp
    src/SDLDisplay.cpp
    src/SDLDisplay.hpp
)

add_library(core_static STATIC ${TRANSFMT_SOURCES})
link_libraries(core_static)

FIND_PACKAGE(PkgConfig REQUIRED)

pkg_check_modules(LIBAVCODEC REQUIRED libavcodec)
pkg_check_modules(LIBAVDEVICE REQUIRED libavdevice)
pkg_check_modules(LIBAVFILTER REQUIRED libavfilter)
pkg_check_modules(LIBAVFORMAT REQUIRED libavformat)
pkg_check_modules(LIBAVUTIL REQUIRED libavutil)
pkg_check_modules(LIBSWRESAMPLE REQUIRED libswresample)
pkg_check_modules(LIBSWSCALE REQUIRED libswscale)
pkg_check_modules(SDL2 REQUIRED sdl2)


include_directories(
        ${LIBAVCODEC_INCLUDE_DIRS}
        ${LIBAVDEVICE_INCLUDE_DIRS}
        ${LIBAVFILTER_INCLUDE_DIRS}
        ${LIBAVFORMAT_INCLUDE_DIRS}
        ${LIBAVUTIL_INCLUDE_DIRS}
        ${LIBSWRESAMPLE_INCLUDE_DIRS}
        ${LIBSWSCALE_INCLUDE_DIRS}
        ${LIBAVRESAMPLE_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIRS}
)

link_directories(
        ${LIBAVCODEC_LIBRARY_DIRS}
        ${LIBAVDEVICE_LIBRARY_DIRS}
        ${LIBAVFILTER_LIBRARY_DIRS}
        ${LIBAVFORMAT_LIBRARY_DIRS}
        ${LIBAVUTIL_LIBRARY_DIRS}
        ${LIBSWRESAMPLE_LIBRARY_DIRS}
        ${LIBSWSCALE_LIBRARY_DIRS}
        ${LIBAVRESAMPLE_LIBRARY_DIRS}
        ${SDL2_LIBRARY_DIRS}
)

set(THIZ_LIBRARIES
        ${THIZ_LIBRARIES}
        ${LIBAVCODEC_LIBRARIES}
        ${LIBAVDEVICE_LIBRARIES}
        ${LIBAVFILTER_LIBRARIES}
        ${LIBAVFORMAT_LIBRARIES}
        ${LIBAVUTIL_LIBRARIES}
        ${LIBSWRESAMPLE_LIBRARIES}
        ${LIBSWSCALE_LIBRARIES}
        ${LIBAVRESAMPLE_LIBRARIES}
        ${SDL2_LIBRARIES}
)
if (APPLE) 
    message("APPLE: add framework to libraries")
    set(THIZ_LIBRARIES
        ${THIZ_LIBRARIES}
        "-framework AVFoundation"
        "-framework CoreGraphics"
        "-framework Foundation"
        "-framework MediaPlayer"
        "-framework CoreMedia"
        "-framework CoreVideo"
        "-framework CoreAudio"
        "-framework CoreServices"
        )

endif () 

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_package(g3logger CONFIG REQUIRED)
endif () 

add_executable(transcoder
        app/transcoder/main.cpp
        app/transcoder/old_transcoder_main.cpp
        app/transcoder/yuv_mix_main.cpp
            )

target_link_libraries(transcoder 
        ${THIZ_LIBRARIES}
        )

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
target_link_libraries(transcoder 
        g3logger
        )
endif () 