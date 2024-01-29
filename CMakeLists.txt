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

# Set the correct triplet for vcpkg
set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "VCPKG Target Triplet to use")

# Set and link transSriptRT modules
set(TRANSSCRIPTRT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(${PROJECT_NAME} PRIVATE ${TRANSSCRIPTRT_INCLUDE_DIR})

# Set and Link sdplog
set(SPDLOG_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows-static/include")
set(SPDLOG_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows-static/lib")
set(SPDLOG_LIBRARY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows-static/debug/lib")
target_include_directories(${PROJECT_NAME} PRIVATE ${SPDLOG_INCLUDE_DIR})
if(CMAKE_BUILD_TYPE MATCHES Debug)
  target_link_libraries(${PROJECT_NAME} PRIVATE ${SPDLOG_LIBRARY_DEBUG}/spdlogd.lib)
else()
  target_link_libraries(${PROJECT_NAME} PRIVATE ${SPDLOG_LIBRARY}/spdlog.lib)
endif()

# Set and Link PortAudio
set(PORTAUDIO_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows-static/include")
set(PORTAUDIO_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows-static/lib")
set(PORTAUDIO_LIBRARY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows-static/debug/lib")
target_include_directories(${PROJECT_NAME} PRIVATE ${PORTAUDIO_INCLUDE_DIR})
if(CMAKE_BUILD_TYPE MATCHES Debug)
  target_link_libraries(${PROJECT_NAME} PRIVATE ${PORTAUDIO_LIBRARY_DEBUG}/portaudio.lib)
else()
  target_link_libraries(${PROJECT_NAME} PRIVATE ${PORTAUDIO_LIBRARY}/portaudio.lib)
endif()

# Link against system libraries
target_link_libraries(${PROJECT_NAME} PRIVATE bcrypt ws2_32 Secur32)

# Set and link Windows Media Foundation
set(MF_LIB_DIR "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.22621.0/um/x64")
set(MF_INCLUDE_DIR "C:/Program Files (x86)/Windows Kits/10/Include/10.0.22621.0/um")
target_include_directories(${PROJECT_NAME} PRIVATE ${MF_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE 
  ${MF_LIB_DIR}/Mfplat.lib
  ${MF_LIB_DIR}/Mfuuid.lib
  ${MF_LIB_DIR}/Mfreadwrite.lib
)

# Set and Link FMTs
set(FMT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows-static/include")
set(FMT_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows-static/lib")
set(FMT_LIBRARY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows-static/debug/lib")
target_include_directories(${PROJECT_NAME} PRIVATE ${FMT_INCLUDE_DIR})
if(CMAKE_BUILD_TYPE MATCHES Debug)
  target_link_libraries(${PROJECT_NAME} PRIVATE ${FMT_LIBRARY_DEBUG}/fmtd.lib)
else()
  target_link_libraries(${PROJECT_NAME} PRIVATE ${FMT_LIBRARY}/fmt.lib)
endif()

# Set and Link FFmpeg
set(FFMPEG_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows-static/include")
set(FFMPEG_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows-static/lib")
set(FFMPEG_LIBRARY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows-static/debug/lib")
target_include_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_INCLUDE_DIR})
if (CMAKE_BUILD_TYPE MATCHES Debug)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    ${FFMPEG_LIBRARY_DEBUG}/avcodec.lib
    ${FFMPEG_LIBRARY_DEBUG}/avfilter.lib
    ${FFMPEG_LIBRARY_DEBUG}/avformat.lib
    ${FFMPEG_LIBRARY_DEBUG}/avutil.lib
    ${FFMPEG_LIBRARY_DEBUG}/swresample.lib
    ${FFMPEG_LIBRARY_DEBUG}/swscale.lib
  )
else()
  target_link_libraries(${PROJECT_NAME} PRIVATE
    ${FFMPEG_LIBRARY}/avcodec.lib
    ${FFMPEG_LIBRARY}/avfilter.lib
    ${FFMPEG_LIBRARY}/avformat.lib
    ${FFMPEG_LIBRARY}/avutil.lib
    ${FFMPEG_LIBRARY}/swresample.lib
    ${FFMPEG_LIBRARY}/swscale.lib
)
endif()

# Set and Link TBB
set(TBB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows-static/include")
set(TBB_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows-static/lib")
set(TBB_LIBRARY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows-static/debug/lib")
if(CMAKE_BUILD_TYPE MATCHES Debug)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    ${TBB_LIBRARY_DEBUG}/tbb12_debug.lib
    ${TBB_LIBRARY_DEBUG}/tbbmalloc_debug.lib
  )
else()
  target_link_libraries(${PROJECT_NAME} PRIVATE
    ${TBB_LIBRARY}/tbb12.lib
    ${TBB_LIBRARY}/tbbmalloc.lib
)
endif()

# Set and Link CUDA
set(CUDA_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/libraries/Cuda/include")
set(CUDA_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/libraries/Cuda/lib/x64")
target_include_directories(${PROJECT_NAME} PRIVATE ${CUDA_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${CUDA_LIBRARIES}/cudart_static.lib
    ${CUDA_LIBRARIES}/cublas.lib
)

# Set and Link cuDNN
set(CUDNN_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/libraries/cuDNN/include")
set(CUDNN_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/libraries/cuDNN/lib/x64")
target_include_directories(${PROJECT_NAME} PRIVATE ${CUDNN_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${CUDNN_LIBRARIES}/cudnn.lib
)

# Set and Link TensorRT
set(TENSORRT_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/libraries/TensorRT/include")
set(TENSORRT_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/libraries/TensorRT/lib")
target_include_directories(${PROJECT_NAME} PRIVATE ${TENSORRT_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${TENSORRT_LIBRARIES}/nvinfer.lib
    ${TENSORRT_LIBRARIES}/nvinfer_plugin.lib
    # Add other TensorRT libraries as needed for loading, preprocessing, and inference
)