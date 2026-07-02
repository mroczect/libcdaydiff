cmake_minimum_required(VERSION 3.10)
project(libcdaydiff VERSION 1.0.1 LANGUAGES C)

# C99 standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Warning flags (platform-agnostic)
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Define POSIX source on Unix-like systems
if(UNIX)
    add_definitions(-D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE)
endif()

# Collect source files
set(SOURCES
    src/core.c
    src/parser.c
    src/diff.c
    src/manip.c
    src/fmt.c
    src/tz.c
    src/epoch.c
    src/error.c
    src/info.c
    src/comparison.c
)

# Internal include directory
include_directories(src/include src/internal)

# Static library
add_library(cdaydiff_static STATIC ${SOURCES})
set_target_properties(cdaydiff_static PROPERTIES
    OUTPUT_NAME cdaydiff
    POSITION_INDEPENDENT_CODE ON
)

# Shared library
add_library(cdaydiff_shared SHARED ${SOURCES})
set_target_properties(cdaydiff_shared PROPERTIES
    OUTPUT_NAME cdaydiff
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# Export public header
target_include_directories(cdaydiff_shared PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/include>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(cdaydiff_static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/include>
    $<INSTALL_INTERFACE:include>
)

# Alias (convenience)
add_library(libcdaydiff::static ALIAS cdaydiff_static)
add_library(libcdaydiff::shared ALIAS cdaydiff_shared)

# Unit test executable
add_executable(test_date test.c)
target_link_libraries(test_date PRIVATE cdaydiff_static)
# If you want to use ctest, add:
enable_testing()
add_test(NAME TestAll COMMAND test_date)

# Installation rules
include(GNUInstallDirs)

install(TARGETS cdaydiff_static cdaydiff_shared
    EXPORT libcdaydiff-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # for DLL on Windows
)

install(FILES src/include/libcdaydiff.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT libcdaydiff-targets
    FILE libcdaydiffTargets.cmake
    NAMESPACE libcdaydiff::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libcdaydiff
)

# Create a Version file for find_package
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/libcdaydiffConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/libcdaydiffConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libcdaydiff
)