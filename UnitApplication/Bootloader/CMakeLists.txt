message("########## Bootloader CMakeLists.txt - start ##########")

#################################################### Metadata ####################################################

# Create object file from metadata source
add_library(metadata_obj OBJECT
    Source/initial_metadata.c
)

target_include_directories(metadata_obj PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
)

# Set compile options to ensure minimal sections
target_compile_options(metadata_obj PRIVATE
    -fno-builtin
    -fno-function-sections
    -fdata-sections
    -ffreestanding
)

# Custom command to create binary from object file
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/metadata.bin
    COMMAND ${CMAKE_OBJCOPY} -O binary 
            $<TARGET_OBJECTS:metadata_obj> 
            ${CMAKE_CURRENT_BINARY_DIR}/metadata.bin
    DEPENDS metadata_obj
    COMMENT "Generating metadata binary"
)

# Add custom target to ensure binary gets built
add_custom_target(metadata ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/metadata.bin
)

#################################################### Bootloader ####################################################

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