cmake_minimum_required(VERSION 3.18)
set(PROJECT_NAME VIRenderer)

set(COMPILER_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(COMPILER_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})

include(tools.cmake)

set(OSX_VERSION "10.15")

if (WIN32)
  project(${PROJECT_NAME} C CXX ASM)
  set(CMAKE_CXX_STANDARD 14)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
  add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
  add_definitions(-DUNICODE)
  add_definitions(-D_UNICODE)
elseif (APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "${OSX_VERSION}" CACHE STRING "Minimum OS X deployment version")
  enable_language(OBJCXX)
  project(${PROJECT_NAME} C CXX OBJC OBJCXX ASM)
  set(CMAKE_CXX_STANDARD 14)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_FLAGS "-std=c++14 -stdlib=libc++")
  set(CMAKE_MACOSX_RPATH ON)
  set(BUILD_RPATH_USE_ORIGIN TRUE)
  set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

file (
  GLOB_RECURSE vi_src
  LIST_DIRECTORIES false
  "${PROJECT_SOURCE_DIR}/Source/*.cpp"
  "${PROJECT_SOURCE_DIR}/Source/*.hpp"
  "${PROJECT_SOURCE_DIR}/Source/*.h"
  "${PROJECT_SOURCE_DIR}/Source/*.m"
)

set(LIBRARY_NAME "vi")

add_library(${LIBRARY_NAME} SHARED ${vi_src})

if (WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4819")
endif()

target_include_directories(${LIBRARY_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/Source")
target_include_directories(${LIBRARY_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/Source/camera")
target_include_directories(${LIBRARY_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/Source/video")
if(WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4819")
  target_compile_definitions(${LIBRARY_NAME} PRIVATE VI_DLL)
elseif (APPLE)
  target_compile_options(${LIBRARY_NAME} PUBLIC -fvisibility=hidden)
  target_compile_options(${LIBRARY_NAME} PUBLIC -fvisibility-inlines-hidden)
  target_compile_options(${LIBRARY_NAME} PUBLIC -x objective-c++)
endif()

git_clone("ThirdParty/fmt" "https://github.com/fmtlib/fmt" "9e8b86fd2d9806672cc73133d21780dd182bfd24") #8.0.0

if(WIN32)
  file(REMOVE_RECURSE ${PROJECT_SOURCE_DIR}/ThirdParty/ffmpeg)
  if ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    file(ARCHIVE_EXTRACT INPUT "${PROJECT_SOURCE_DIR}/vendor/ffmpeg-4.4.1/win32/ffmpeg-n4.4.1-2-gcc33e73618-win32-gpl-shared-4.4.zip" DESTINATION ${PROJECT_SOURCE_DIR}/ThirdParty VERBOSE)
    file(RENAME ${PROJECT_SOURCE_DIR}/ThirdParty/ffmpeg-n4.4.1-2-gcc33e73618-win32-gpl-shared-4.4 ${PROJECT_SOURCE_DIR}/ThirdParty/ffmpeg)
    target_link_options(${LIBRARY_NAME} PUBLIC /SAFESEH:NO)
  else()
    file(ARCHIVE_EXTRACT INPUT "${PROJECT_SOURCE_DIR}/vendor/ffmpeg-4.4.1/win32/ffmpeg-n4.4.1-2-gcc33e73618-win64-gpl-shared-4.4.zip" DESTINATION ${PROJECT_SOURCE_DIR}/ThirdParty VERBOSE)
    file(RENAME ${PROJECT_SOURCE_DIR}/ThirdParty/ffmpeg-n4.4.1-2-gcc33e73618-win64-gpl-shared-4.4 ${PROJECT_SOURCE_DIR}/ThirdParty/ffmpeg)
  endif()
  target_include_directories(${LIBRARY_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/ThirdParty/ffmpeg/include")
  target_link_directories(${LIBRARY_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/ThirdParty/ffmpeg/lib")
  target_link_libraries(${LIBRARY_NAME} PRIVATE avcodec.lib PRIVATE avdevice.lib PRIVATE avfilter.lib PRIVATE avformat.lib PRIVATE avutil.lib PRIVATE postproc.lib PRIVATE swresample.lib PRIVATE swscale.lib)
  install(FILES $<TARGET_PDB_FILE:${LIBRARY_NAME}> DESTINATION bin OPTIONAL)
  file (
    GLOB_RECURSE ffmpeg_dlls
    LIST_DIRECTORIES false
    "${PROJECT_SOURCE_DIR}/ThirdParty/ffmpeg/bin/*.dll"
  )
  foreach(_ffmpeg_dll IN ITEMS ${ffmpeg_dlls})
    add_custom_command(TARGET ${LIBRARY_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${_ffmpeg_dll} $<TARGET_FILE_DIR:${LIBRARY_NAME}>
    )
  endforeach()
elseif (APPLE)
  file(REMOVE_RECURSE ${PROJECT_SOURCE_DIR}/ThirdParty/ffmpeg)
  file(COPY "${PROJECT_SOURCE_DIR}/vendor/ffmpeg-4.4.1/darwin/${OSX_VERSION}" DESTINATION ${PROJECT_SOURCE_DIR}/ThirdParty VERBOSE)

  set(DYLIBS 
    libavcodec.58.dylib 
    libavdevice.58.dylib 
    libavfilter.7.dylib 
    libavformat.58.dylib
    libavutil.56.dylib
    libpostproc.55.dylib
    libswresample.3.dylib
    libswscale.5.dylib
  )

  file(RENAME ${PROJECT_SOURCE_DIR}/ThirdParty/10.15 ${PROJECT_SOURCE_DIR}/ThirdParty/ffmpeg)
  target_include_directories(${LIBRARY_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/ThirdParty/ffmpeg/include")
  target_link_directories(${LIBRARY_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/ThirdParty/ffmpeg/lib")

  foreach(DYLIB ${DYLIBS})
    target_link_libraries(${LIBRARY_NAME} PRIVATE ${DYLIB})
    add_custom_command(TARGET ${LIBRARY_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/ThirdParty/ffmpeg/lib/${DYLIB} $<TARGET_FILE_DIR:${LIBRARY_NAME}>
    )
  endforeach()

  find_library(ACCELETATE Accelerate)
  target_link_libraries(${LIBRARY_NAME} PRIVATE ${ACCELETATE})
  find_library(APPKIT AppKit)
  target_link_libraries(${LIBRARY_NAME} PRIVATE ${APPKIT})
  find_library(CORE_FOUNDATION CoreFoundation)
  target_link_libraries(${LIBRARY_NAME} PRIVATE ${CORE_FOUNDATION})
  find_library(AVFOUNDATION AVFoundation)
  target_link_libraries(${LIBRARY_NAME} PRIVATE ${AVFOUNDATION})
  find_library(COCOA Cocoa)
  target_link_libraries(${LIBRARY_NAME} PRIVATE ${COCOA})
  find_library(COREIMAGE CoreImage)
  target_link_libraries(${LIBRARY_NAME} PRIVATE ${COREIMAGE})
  find_library(COREMEDIA CoreMedia)
  target_link_libraries(${LIBRARY_NAME} PRIVATE ${COREMEDIA})
  find_library(COREVIDEO CoreVideo)
  target_link_libraries(${LIBRARY_NAME} PRIVATE ${COREVIDEO})
endif()

add_subdirectory(${PROJECT_SOURCE_DIR}/ThirdParty/fmt)
target_link_libraries(${LIBRARY_NAME} PRIVATE fmt)
target_include_directories(${LIBRARY_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/ThirdParty/fmt/include")

set(INSTALL_BIN_DIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE PATH "Installation directory for executables")
set(INSTALL_LIB_DIR "${CMAKE_INSTALL_PREFIX}/lib" CACHE PATH "Installation directory for libraries")
set(INSTALL_INC_DIR "${CMAKE_INSTALL_PREFIX}/include" CACHE PATH "Installation directory for headers")

install(TARGETS ${LIBRARY_NAME} ${LIBRARY_NAME}
        RUNTIME DESTINATION "${INSTALL_BIN_DIR}"
        ARCHIVE DESTINATION "${INSTALL_LIB_DIR}"
        LIBRARY DESTINATION "${INSTALL_LIB_DIR}")

set(LIBRARY_PUBLIC_HDRS 
  ${PROJECT_SOURCE_DIR}/Source/VI.h
)

install(FILES ${LIBRARY_PUBLIC_HDRS} DESTINATION "${INSTALL_INC_DIR}")
if(WIN32)
  install(FILES $<TARGET_PDB_FILE:${LIBRARY_NAME}> DESTINATION bin OPTIONAL)
  install(FILES ${ffmpeg_dlls} DESTINATION bin)
elseif (APPLE)
  foreach(DYLIB ${DYLIBS})
    install(FILES ${PROJECT_SOURCE_DIR}/ThirdParty/ffmpeg/lib/${DYLIB} DESTINATION bin)
  endforeach()
endif()

# ============ Test ==============
file (
  GLOB_RECURSE main_src
  LIST_DIRECTORIES false
  "${PROJECT_SOURCE_DIR}/Test/*.cpp"
  "${PROJECT_SOURCE_DIR}/Test/*.h"
)

add_executable(${PROJECT_NAME} ${main_src})
target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/Source")

install_glfw(${PROJECT_NAME})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:${LIBRARY_NAME}>" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBRARY_NAME})

install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}
RUNTIME DESTINATION "${INSTALL_BIN_DIR}"
ARCHIVE DESTINATION "${INSTALL_LIB_DIR}"
LIBRARY DESTINATION "${INSTALL_LIB_DIR}")

if (APPLE)
  set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-Wl,-rpath,./")
  set_target_properties(${PROJECT_NAME} PROPERTIES CMAKE_BUILD_RPATH "$ORIGIN")
  set_target_properties(${PROJECT_NAME} PROPERTIES CMAKE_INSTALL_RPATH "$ORIGIN")
  install(CODE "file(COPY ${CMAKE_INSTALL_PREFIX}/lib/libvi.dylib DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_targets_folder()

