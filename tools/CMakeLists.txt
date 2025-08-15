cmake_minimum_required(VERSION 3.13)

ExternalProject_Add(mkfs.afs-host
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/mkfs.afs"
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/mkfs.afs"
    INSTALL_COMMAND "")
