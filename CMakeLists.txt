# Copyright (C) [Year] [Name of Copyright Holder]
#
# This file is part of [Your Project Name].
#
# [Your Project Name] is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# [Your Project Name] is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <https://gnu.org>.


cmake_minimum_required(VERSION 3.21)
project(joltcr VERSION 0.1.0 LANGUAGES CXX)

# 1. Define the library target
add_library(joltcr src/joltcr.cpp)
add_library(joltcr::joltcr ALIAS joltcr)

# 2. Set C++ standard and include directories
target_compile_features(joltcr PUBLIC cxx_std_17)
target_include_directories(joltcr
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# 3. Install the library and headers
include(GNUInstallDirs)

install(TARGETS joltcr
    EXPORT joltcrTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# 4. Export the target so find_package(joltcr) works
install(EXPORT joltcrTargets
    FILE joltcrTargets.cmake
    NAMESPACE joltcr::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/joltcr
)

# 5. Generate the Config file for find_package
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/joltcrConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/joltcrConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/joltcr
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/joltcrConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/joltcrConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/joltcrConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/joltcr
)