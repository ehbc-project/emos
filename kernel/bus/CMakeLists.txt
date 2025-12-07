cmake_minimum_required(VERSION 3.13)

add_subdirectory(nonpnp)

target_sources(kernel PRIVATE bus.c)
