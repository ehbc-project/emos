cmake_minimum_required(VERSION 3.13)

target_sources(kernel PRIVATE entry.S gdt.c init.c instruction.c isr.c isr.S panic.c pic.c thread.c tss.c)
target_link_options(kernel PUBLIC -T "${CMAKE_CURRENT_SOURCE_DIR}/ldscript.lds")
set_target_properties(kernel PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ldscript.lds")

target_include_directories(kernel PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_custom_target(kernel_execbin ALL
    COMMAND ${CMAKE_OBJCOPY} -O binary "$<TARGET_FILE:kernel>" kernel.bin
    DEPENDS kernel
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Creating kernel executable binary"
)
