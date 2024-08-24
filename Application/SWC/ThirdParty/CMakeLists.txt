
message("########## ThirdParty/CMakeLists.txt - start ##########")

# Define the path to the source files for ThirdParty
set(THIRDPARTY_SOURCE_FILES
    #${CMAKE_CURRENT_SOURCE_DIR}/Source/source_file.c
    )

# Define the path to the include directory for ThirdParty
set(THIRDPARTY_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
    )

# Export the variable to the parent scope
set(THIRDPARTY_SOURCE_FILES ${THIRDPARTY_SOURCE_FILES} PARENT_SCOPE)
set(THIRDPARTY_INCLUDE_DIR ${THIRDPARTY_INCLUDE_DIR} PARENT_SCOPE)

message("########## ThirdParty/CMakeLists.txt - end ##########")
