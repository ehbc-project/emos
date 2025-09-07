cmake_minimum_required(VERSION 3.13)

if ("${TARGET_FIRMWARE}" STREQUAL "uefi")
    ExternalProject_Add(gnu-efi
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/gnu-efi"
        INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/gnu-efi"
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ${CMAKE_COMMAND} -E env CROSS_COMPILE=${TOOLCHAIN_PREFIX} make -C <SOURCE_DIR>
        INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory "<SOURCE_DIR>/ia32" "<INSTALL_DIR>/ia32"
    )
endif()
