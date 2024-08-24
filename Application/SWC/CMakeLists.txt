message("########## Application/Standard CMakeLists.txt - start ##########")

# Add subdirectory for Blinky software component
add_subdirectory(Blinky)

# Add subdirectory for ThirdParty files
add_subdirectory(ThirdParty)

# Define the executable target for the application
add_executable(blinky_main
        main.c
        ${BLINKY_SOURCE_FILES}  # Use the variable defined in the Blinky CMakeLists.txt
        )

# Set compile definitions if needed
target_compile_definitions(blinky_main PRIVATE)

# Set include directories
target_include_directories(blinky_main PRIVATE
        ${BLINKY_INCLUDE_DIR} 
        ${THIRDPARTY_INCLUDE_DIR}
        )

# Link libraries and dependencies
target_link_libraries(blinky_main 
                      pico_stdlib  
                      pico_cyw43_arch_none
                      FreeRTOS-Kernel 
                      FreeRTOS-Kernel-Heap1)

# Add extra outputs specific to the Pico SDK
pico_add_extra_outputs(blinky_main)

message("########## Application/Standard CMakeLists.txt - end ##########")
