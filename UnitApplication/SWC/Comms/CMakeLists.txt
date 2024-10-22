
message("########## Comms/CMakeLists.txt - start ##########")

# Define the path to the source files for Comms
set(COMMS_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/WiFi_Transmit.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/WiFi_Receive.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/WiFi_Init.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/WiFi_main.c 
    )

# Define the path to the include directory for Comms
set(COMMS_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
    )

# Export the variables to the parent scope
set(COMMS_SOURCE_FILES ${COMMS_SOURCE_FILES} PARENT_SCOPE)
set(COMMS_INCLUDE_DIR ${COMMS_INCLUDE_DIR} PARENT_SCOPE)

message("########## Comms/CMakeLists.txt - end ##########")
