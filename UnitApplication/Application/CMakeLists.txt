message("########## Application/Standard CMakeLists.txt - start ##########")

# Define the main source file with a default value (original application main.c)
# This is used to give a possibility to the user to change the main.c source file to a one from Examples folder by calling a special build command
set(MAIN_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/main.c" CACHE FILEPATH "Path to the main.c file to use")
message(STATUS "Using main source: ${MAIN_SOURCE}")

# Add subdirectories of the Software Components
add_subdirectory(OS)
add_subdirectory(Misc)
add_subdirectory(WiFi)
add_subdirectory(Monitor)
add_subdirectory(HAL)
add_subdirectory(Drivers)
add_subdirectory(PIO)

# Helper function to create a moduri target for a given bank
function(create_moduri_target target_name linker_script)
    add_executable(${target_name}
        ${MAIN_SOURCE} 
        ${OS_SOURCE_FILES}
        ${MISC_SOURCE_FILES}
        ${WIFI_SOURCE_FILES}
        ${MONITOR_SOURCE_FILES}
        ${HAL_SOURCE_FILES}
        ${DRIVERS_SOURCE_FILES}
    )

    # Link the flash utils library to the target
    target_link_libraries(${target_name} PRIVATE flash_utils)

    # Set compile definitions if needed
    target_compile_definitions(${target_name} PRIVATE)

    # Set include directories
    target_include_directories(${target_name} PRIVATE
        ${OS_INCLUDE_DIR} 
        ${MISC_INCLUDE_DIR}
        ${WIFI_INCLUDE_DIR}
        ${MONITOR_INCLUDE_DIR}
        ${HAL_INCLUDE_DIR}
        ${DRIVERS_INCLUDE_DIR}
    )

    # Link libraries and dependencies
    target_link_libraries(${target_name} PRIVATE
        # Core system libraries
        pico_stdlib
        pico_cyw43_arch_lwip_sys_freertos
        
        # Network stack
        pico_lwip_iperf
        
        # Security libraries
        pico_mbedtls

        # Hardware peripherals
        hardware_i2c
        hardware_adc
        hardware_spi
        hardware_pwm
        hardware_pio
        hardware_dma
        
        # RTOS
        FreeRTOS-Kernel-Heap4
    )

    # Generate the PIO header files
    foreach(PIO_FILE ${PIO_SOURCE_FILES})
        pico_generate_pio_header(${target_name} ${PIO_FILE})
    endforeach()

    # Use the provided custom linker script for this target
    pico_set_linker_script(${target_name} ${CMAKE_CURRENT_SOURCE_DIR}/${linker_script})

    # Add extra outputs specific to the Pico SDK
    pico_add_extra_outputs(${target_name})
endfunction()

# Create two targets: one for Bank A and one for Bank B
create_moduri_target(moduri_bank_A "application_bank_A.ld")
create_moduri_target(moduri_bank_B "application_bank_B.ld")

message("########## Application/Standard CMakeLists.txt - end ##########")
