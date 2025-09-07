cmake_minimum_required(VERSION 3.13)

target_sources(kernel PRIVATE entry.S)
target_link_options(kernel PUBLIC -T "${CMAKE_CURRENT_SOURCE_DIR}/ldscript.lds")
set_target_properties(kernel PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ldscript.lds")
