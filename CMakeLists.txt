cmake_minimum_required(VERSION 3.14)
project(transScriptRT)

# Set the C++ standard for the project
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

if(MSVC)
  # Replace MDd with MTd for Debug builds to match the TBB debug library
  string(REPLACE "/MDd" "/MTd" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
  # Replace MD with MT for Release builds to use the static runtime library
  string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
endif()

# Add the executables
add_executable(${PROJECT_NAME} 
  src/main.cpp 
  src/audio_tsrt.cpp 
  src/logger_tsrt.cpp 
  src/script_engine_tsrt.cpp)

# Set and link transSriptRT modules
set(TRANSSCRIPTRT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(${PROJECT_NAME} PRIVATE ${TRANSSCRIPTRT_INCLUDE_DIR})

# pkg-config
find_package(PkgConfig REQUIRED)

# spdlog
find_package(spdlog CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)

# fmt
find_package(fmt CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)

# TBB
find_package(TBB CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE TBB::tbb TBB::tbbmalloc)

# PortAudio
pkg_check_modules(PORTAUDIO REQUIRED IMPORTED_TARGET portaudio-2.0)
target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::PORTAUDIO)

# FFmpeg
pkg_check_modules(AVCODEC REQUIRED IMPORTED_TARGET libavcodec)
pkg_check_modules(AVFORMAT REQUIRED IMPORTED_TARGET libavformat)
pkg_check_modules(AVUTIL REQUIRED IMPORTED_TARGET libavutil)
pkg_check_modules(AVFILTER REQUIRED IMPORTED_TARGET libavfilter)
target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::AVCODEC PkgConfig::AVFORMAT PkgConfig::AVUTIL PkgConfig::AVFILTER)