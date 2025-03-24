idf_component_register(
    SRCS "Source/CAN_HAL.c"
    INCLUDE_DIRS "Include"
    REQUIRES driver esp_common freertos Common
)