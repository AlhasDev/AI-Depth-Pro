cmake_minimum_required(VERSION 3.16)
project(AI_Depth_Pro VERSION 0.2.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(BUILD_TESTS "Build regression tests" ON)
option(BUILD_INFERENCE_TESTS "Run actual ONNX model tests" OFF)
set(ONNX_RUNTIME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/onnxruntime" CACHE PATH "ONNX runtime libraries")
find_package(Threads REQUIRED)
set(CORE_SOURCES
    src/Utils/Logger.cpp src/Utils/FileUtils.cpp src/Cache/LRUCache.cpp src/Cache/PersistentDepthCache.cpp
    src/DepthEngine/DepthEngine.cpp src/DepthEngine/OnnxDepthEngine.cpp
    src/Processing/EdgeRefinement.cpp src/Processing/DepthNormalizer.cpp src/Processing/ColorMap.cpp
    src/Temporal/MotionEstimator.cpp src/Temporal/TemporalStabilizer.cpp
    src/Effects/DepthMask.cpp src/Effects/DepthOfField.cpp src/Effects/Parallax.cpp
    src/Effects/DepthZoom.cpp src/Effects/DepthFog.cpp src/Effects/Relighting.cpp src/UI/OverlayRenderer.cpp)
add_library(depth_core STATIC ${CORE_SOURCES})
set_target_properties(depth_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(depth_core PUBLIC include include/openfx src)
target_link_libraries(depth_core PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
if(WIN32)
    target_compile_definitions(depth_core PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
endif()
if(MSVC)
    target_compile_options(depth_core PRIVATE /W4 /EHsc /MP)
else()
    target_compile_options(depth_core PRIVATE -Wall -Wextra)
endif()
add_library(AI_Depth_Pro SHARED src/Plugin.cpp src/PluginEntry.cpp)
target_link_libraries(AI_Depth_Pro PRIVATE depth_core)
if(WIN32)
    set(OFX_PLATFORM Win64)
elseif(APPLE)
    set(OFX_PLATFORM MacOS)
else()
    set(OFX_PLATFORM Linux-x86-64)
endif()
set(OFX_BUNDLE "${CMAKE_BINARY_DIR}/AI-Depth-Pro.ofx.bundle")
set_target_properties(AI_Depth_Pro PROPERTIES PREFIX "" OUTPUT_NAME "AI-Depth-Pro" SUFFIX ".ofx"
    RUNTIME_OUTPUT_DIRECTORY "${OFX_BUNDLE}/Contents/${OFX_PLATFORM}"
    LIBRARY_OUTPUT_DIRECTORY "${OFX_BUNDLE}/Contents/${OFX_PLATFORM}")
foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set_target_properties(AI_Depth_Pro PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${config} "${OFX_BUNDLE}/Contents/${OFX_PLATFORM}"
        LIBRARY_OUTPUT_DIRECTORY_${config} "${OFX_BUNDLE}/Contents/${OFX_PLATFORM}")
endforeach()
function(copy_runtime target)
    file(GLOB runtime_libraries "${ONNX_RUNTIME_DIR}/*.dll" "${ONNX_RUNTIME_DIR}/*.so*" "${ONNX_RUNTIME_DIR}/*.dylib")
    foreach(lib IN LISTS runtime_libraries)
        add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${lib}" "$<TARGET_FILE_DIR:${target}>")
    endforeach()
endfunction()
copy_runtime(AI_Depth_Pro)
add_custom_command(TARGET AI_Depth_Pro POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/models" "${OFX_BUNDLE}/Contents/models")
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
