message("########## Application/Standard CMakeLists.txt - start ##########")

# Add subdirectories of the Software Components
add_subdirectory(OS)
add_subdirectory(Misc)
add_subdirectory(WiFi)
add_subdirectory(Monitor)
add_subdirectory(HAL)
add_subdirectory(PIO)

# Define the executable target for the application
add_executable(moduri
        main.c
        ${OS_SOURCE_FILES}
        ${MISC_SOURCE_FILES}
        ${WIFI_SOURCE_FILES}
        ${MONITOR_SOURCE_FILES}
        ${HAL_SOURCE_FILES}
        )

# Set compile definitions if needed
target_compile_definitions(moduri PRIVATE)

# Set include directories
target_include_directories(moduri PRIVATE
        ${OS_INCLUDE_DIR} 
        ${MISC_INCLUDE_DIR}
        ${WIFI_INCLUDE_DIR}
        ${MONITOR_INCLUDE_DIR}
        ${HAL_INCLUDE_DIR}
        )

# Link libraries and dependencies
target_link_libraries(moduri 
                        pico_cyw43_arch_lwip_sys_freertos
                        pico_stdlib
                        pico_lwip_iperf
                        hardware_i2c
                        hardware_adc
                        hardware_spi
                        hardware_pwm
                        hardware_pio
                        hardware_dma
                        FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
                        )

# Generate the PIO header files
foreach(PIO_FILE ${PIO_SOURCE_FILES})
pico_generate_pio_header(moduri ${PIO_FILE})
endforeach()

# Add extra outputs specific to the Pico SDK
pico_add_extra_outputs(moduri)

message("########## Application/Standard CMakeLists.txt - end ##########")
