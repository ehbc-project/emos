cmake_minimum_required(VERSION 3.13)

ExternalProject_Add(esh
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/esh"
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/esh"
    INSTALL_COMMAND ""
)

ExternalProject_Add(fdisk
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/fdisk"
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/fdisk"
    INSTALL_COMMAND ""
)

ExternalProject_Add(pkg
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/pkg"
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/pkg"
    INSTALL_COMMAND ""
)

ExternalProject_Add(sysutils
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sysutils"
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/sysutils"
    INSTALL_COMMAND ""
)
