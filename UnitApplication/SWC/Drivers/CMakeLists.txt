message("########## Drivers/CMakeLists.txt - start ##########")

# Define the path to the source files for Drivers
set(DRIVERS_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SSD1306_HAL.c
    )

# Define the path to the include directory for Drivers
set(DRIVERS_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
    )

# Export the variables to the parent scope
set(DRIVERS_SOURCE_FILES ${DRIVERS_SOURCE_FILES} PARENT_SCOPE)
set(DRIVERS_INCLUDE_DIR ${DRIVERS_INCLUDE_DIR} PARENT_SCOPE)

message("########## Drivers/CMakeLists.txt - end ##########")