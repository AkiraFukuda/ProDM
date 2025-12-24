cmake_minimum_required(VERSION 3.15)
project(ProDM VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(PRODM_THIRDPARTY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/external")

# zstd headers (bundled inside SZ3 repo/tools in your tree)
set(ZSTD_INCLUDE_DIR   "${PRODM_THIRDPARTY_ROOT}/SZ3/tools/zstd")

# SZ3 headers + libs (your external SZ3 install tree)
set(SZ3_PREFIX         "${PRODM_THIRDPARTY_ROOT}/SZ3/install")
set(SZ3_INCLUDE_DIR    "${SZ3_PREFIX}/include")
set(SZ3_LIB_DIR        "${SZ3_PREFIX}/lib")

# MGARDx headers (your external MGARDx install tree)
set(MGARDX_PREFIX      "${PRODM_THIRDPARTY_ROOT}/MGARDx/install")
set(MGARDX_INCLUDE_DIR "${MGARDX_PREFIX}/include")

# Libraries (build-time discovery)
find_library(ZSTD_LIB zstd
  HINTS "${PRODM_THIRDPARTY_ROOT}/SZ/install/${CMAKE_INSTALL_LIBDIR}"
        "${PRODM_THIRDPARTY_ROOT}/SZ/install/lib"
        "${PRODM_THIRDPARTY_ROOT}/SZ/install/lib64"
)

find_library(SZ3_LIB SZ3c
  HINTS "${SZ3_LIB_DIR}" "${SZ3_PREFIX}/lib64" "${SZ3_PREFIX}/lib"
)

if(NOT ZSTD_LIB)
  message(FATAL_ERROR "Could not find zstd library (ZSTD_LIB). Check external/SZ/install/*")
endif()

if(NOT SZ3_LIB)
  message(FATAL_ERROR "Could not find SZ3c library (SZ3_LIB). Check external/SZ3/install/lib*")
endif()

# ProDM target (header-only interface)
add_library(ProDM INTERFACE)
add_library(ProDM::ProDM ALIAS ProDM)

# Build-tree: use local headers (including external)
target_include_directories(ProDM
  INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${SZ3_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${ZSTD_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${MGARDX_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Link deps (still required even though ProDM is header-only)
target_link_libraries(ProDM
  INTERFACE
    ${ZSTD_LIB}
    ${SZ3_LIB}
)

# ProDM own headers
install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

# Bundle SZ3 headers into prefix/include
install(DIRECTORY "${SZ3_INCLUDE_DIR}/"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

# Bundle MGARDx headers into prefix/include
install(DIRECTORY "${MGARDX_INCLUDE_DIR}/"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

# Bundle zstd headers into prefix/include
# (If these are flat headers like zstd.h, this is fine. If it's a subdir layout, it preserves it.)
install(DIRECTORY "${ZSTD_INCLUDE_DIR}/"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(TARGETS ProDM
  EXPORT ProDMTargets
)

install(EXPORT ProDMTargets
  NAMESPACE ProDM::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/ProDM"
)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ProDMConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/ProDMConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/ProDM"
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/ProDMConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY SameMajorVersion
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/ProDMConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/ProDMConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/ProDM"
)

add_subdirectory(test)
