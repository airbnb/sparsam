#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements. See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership. The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the License for the
# specific language governing permissions and limitations
# under the License.

# CMake issue #15943 for clang users targeting macOS
# If you are using CMake and clang to target macOS there is a bug
# that can cause the CMAKE_CXX_STANDARD feature to simply not work
# (not add any compiler flags).
# https://stackoverflow.com/a/31010221

if (POLICY CMP0025)
    cmake_policy(SET CMP0025 NEW)
endif ()

#
# Windows has a different header
if(MSVC)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/windows/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)
else()
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)
endif()

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

# Create flex and bison files and build the lib parse static library
BISON_TARGET(thrifty ${CMAKE_CURRENT_SOURCE_DIR}/src/thrifty.yy ${CMAKE_CURRENT_BINARY_DIR}/thrifty.cc)
FLEX_TARGET(thriftl ${CMAKE_CURRENT_SOURCE_DIR}/src/thriftl.ll ${CMAKE_CURRENT_BINARY_DIR}/thriftl.cc)
ADD_FLEX_BISON_DEPENDENCY(thriftl thrifty)

# HACK: Work around the fact that bison crates a .hh file but we need a .h file
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/thrifty.h
                   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/thrifty.hh ${CMAKE_CURRENT_BINARY_DIR}/thrifty.h
                   DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/thrifty.hh
                   )

set(libparse_SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/thrifty.cc
    ${CMAKE_CURRENT_BINARY_DIR}/thriftl.cc
    ${CMAKE_CURRENT_BINARY_DIR}/thrifty.h
)

add_library(libparse STATIC ${libparse_SOURCES})

# Create the thrift compiler
set( thrift_SOURCES
    src/main.cc
    src/md5.c
    src/generate/t_generator.cc
    src/generate/t_generator_registry.h
    src/globals.h
    src/main.h
    src/platform.h
    src/md5.h
    src/audit/t_audit.cpp
    src/parse/t_doc.h
    src/parse/t_type.h
    src/parse/t_base_type.h
    src/parse/t_enum.h
    src/parse/t_enum_value.h
    src/parse/t_typedef.h
    src/parse/t_typedef.cc
    src/parse/t_container.h
    src/parse/t_list.h
    src/parse/t_set.h
    src/parse/t_map.h
    src/parse/t_struct.h
    src/parse/t_field.h
    src/parse/t_service.h
    src/parse/t_function.h
    src/parse/t_program.h
    src/parse/t_scope.h
    src/parse/t_const.h
    src/parse/t_const_value.h
    src/parse/parse.cc
    src/generate/t_generator.h
    src/generate/t_oop_generator.h
    src/windows/config.h
    version.h
)

# This macro adds an option THRIFT_COMPILER_${NAME}
# that allows enabling or disabling certain languages
macro(THRIFT_ADD_COMPILER name description initial)
    string(TOUPPER "THRIFT_COMPILER_${name}" enabler)
    set(src "src/generate/t_${name}_generator.cc")
    option(${enabler} ${description} ${initial})
    if(${enabler})
        list(APPEND thrift_SOURCES ${src})
    endif()
endmacro()

# The following compiler can be enabled or disabled
THRIFT_ADD_COMPILER(ruby    "Enable compiler for Sparsam (Ruby)" ON)

# Thrift is looking for include files in the src directory
# we also add the current binary directory for generated files
include_directories(${CMAKE_CURRENT_BINARY_DIR} src)

add_executable(thrift-compiler ${thrift_SOURCES})
set_target_properties(thrift-compiler PROPERTIES OUTPUT_NAME sparsam-gen)

target_link_libraries(thrift-compiler libparse)

install(TARGETS thrift-compiler DESTINATION bin)
