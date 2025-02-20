message("########## Bootloader CMakeLists.txt - start ##########")

# Define the executable target for the bootloader and specify the source files
add_executable(bootloader
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    Source/flash_operations.c
)

# Specify the include directories
target_include_directories(bootloader PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
)

# Link libraries and dependencies
target_link_libraries(bootloader PRIVATE
                        pico_stdlib
                        hardware_flash
)

# Use custom linker script
pico_set_linker_script(bootloader ${CMAKE_CURRENT_SOURCE_DIR}/bootloader.ld)

# Generate extra outputs (map/bin/hex/uf2 files in addition to ELF)
pico_add_extra_outputs(bootloader)

message("########## Bootloader CMakeLists.txt - end ##########")