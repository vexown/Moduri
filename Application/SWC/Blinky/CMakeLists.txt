
message("########## Blinky/CMakeLists.txt - start ##########")

# Define the path to the source files for Blinky
set(BLINKY_SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/blinky_main.c
    )

# Define the path to the include directory for Blinky
set(BLINKY_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
    )

# Export the variables to the parent scope
set(BLINKY_SOURCE_FILES ${BLINKY_SOURCE_FILES} PARENT_SCOPE)
set(BLINKY_INCLUDE_DIR ${BLINKY_INCLUDE_DIR} PARENT_SCOPE)

message("########## Blinky/CMakeLists.txt - end ##########")
