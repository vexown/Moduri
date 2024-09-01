message("########## Application/Standard CMakeLists.txt - start ##########")

# Add subdirectories of the Software Components
add_subdirectory(OS)
add_subdirectory(ThirdParty)
add_subdirectory(Comms)

# Define the executable target for the application
add_executable(moduri
        main.c
        ${OS_SOURCE_FILES}
        ${COMMS_SOURCE_FILES}
        )

# Set compile definitions if needed
target_compile_definitions(moduri PRIVATE)

# Set include directories
target_include_directories(moduri PRIVATE
        ${OS_INCLUDE_DIR} 
        ${THIRDPARTY_INCLUDE_DIR}
        ${COMMS_INCLUDE_DIR}
        )

# Link libraries and dependencies
target_link_libraries(moduri 
                        pico_cyw43_arch_lwip_sys_freertos
                        pico_stdlib
                        pico_lwip_iperf
                        FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
                        )

# Add extra outputs specific to the Pico SDK
pico_add_extra_outputs(moduri)

message("########## Application/Standard CMakeLists.txt - end ##########")
