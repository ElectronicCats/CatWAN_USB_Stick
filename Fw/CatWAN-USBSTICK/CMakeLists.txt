# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(catwan_usbstick LANGUAGES C)

set(CATSNIFFER_FW_VERSION "" CACHE STRING "Firmware version label (for releases)")
if(DEFINED ENV{CATSNIFFER_FW_VERSION} AND NOT "$ENV{CATSNIFFER_FW_VERSION}" STREQUAL "")
    set(CATSNIFFER_FW_VERSION "$ENV{CATSNIFFER_FW_VERSION}" CACHE STRING "Firmware version label (for releases)" FORCE)
endif()

find_package(Git QUIET)
set(CATSNIFFER_GIT_SHA "unknown")
set(CATSNIFFER_GIT_DIRTY "unknown")
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} -C ${CMAKE_SOURCE_DIR} rev-parse --short HEAD
        OUTPUT_VARIABLE CATSNIFFER_GIT_SHA
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} -C ${CMAKE_SOURCE_DIR} status --porcelain --untracked-files=no
        OUTPUT_VARIABLE CATSNIFFER_GIT_STATUS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(CATSNIFFER_GIT_STATUS STREQUAL "")
        set(CATSNIFFER_GIT_DIRTY "clean")
    else()
        set(CATSNIFFER_GIT_DIRTY "dirty")
    endif()
endif()

if(NOT CATSNIFFER_FW_VERSION)
    set(CATSNIFFER_FW_VERSION "dev-${CATSNIFFER_GIT_SHA}-${CATSNIFFER_GIT_DIRTY}")
endif()

string(TIMESTAMP CATSNIFFER_BUILD_TIME_UTC "%Y-%m-%dT%H:%M:%SZ" UTC)
set(CATSNIFFER_COMPILER_ID "${CMAKE_C_COMPILER_ID}")
set(CATSNIFFER_COMPILER_VERSION "${CMAKE_C_COMPILER_VERSION}")

configure_file(
    ${CMAKE_SOURCE_DIR}/include/fw_version.h.in
    ${CMAKE_BINARY_DIR}/generated/fw_version.h
    @ONLY
)

target_include_directories(app PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(app PRIVATE ${CMAKE_BINARY_DIR}/generated)

FILE(GLOB app_sources src/*.c)

# Add local usb init
target_sources_ifdef(CONFIG_USB_DEVICE_STACK_NEXT app PRIVATE
    src/USB/usbd_init.c
)

target_sources(app PRIVATE ${app_sources})
