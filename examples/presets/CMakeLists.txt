cmake_minimum_required(VERSION 3.20)

project(squid_presets_example CXX)

find_package(squid REQUIRED)

# Create the executable
add_executable(squid_presets main.cpp)

# Link against squid
target_link_libraries(squid_presets PRIVATE squid::squid)

# C++ standard
set_property(TARGET squid_presets PROPERTY CXX_STANDARD 20)
