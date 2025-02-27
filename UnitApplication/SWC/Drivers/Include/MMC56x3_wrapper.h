/* MMC56x3_wrapper.h - C-compatible wrapper for the MMC56x3 driver */

#ifndef MMC56X3_WRAPPER_H
#define MMC56X3_WRAPPER_H

#include <stdint.h>
#include "hardware/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declare the C++ class as an opaque type for C code */
typedef struct MMC56x3_Instance MMC56x3_Instance;

/* Structure to hold magnetometer data (C-compatible) */
typedef struct 
{
    float x;
    float y;
    float z;
    float temperature;
} MMC56x3_MagData;

/* C-compatible interface functions */
MMC56x3_Instance* MMC56x3_Create(i2c_inst_t* i2c_instance, uint8_t address);
void MMC56x3_Destroy(MMC56x3_Instance* instance);
bool MMC56x3_Begin(MMC56x3_Instance* instance);
void MMC56x3_Reset(MMC56x3_Instance* instance);
void MMC56x3_SetDataRate(MMC56x3_Instance* instance, uint8_t rate);
bool MMC56x3_ReadData(MMC56x3_Instance* instance, MMC56x3_MagData* data);

/* Additional functions to expose more C++ functionality */
float MMC56x3_ReadTemperature(MMC56x3_Instance* instance);
void MMC56x3_MagnetSetReset(MMC56x3_Instance* instance);
void MMC56x3_SetContinuousMode(MMC56x3_Instance* instance, bool mode);
bool MMC56x3_IsContinuousMode(MMC56x3_Instance* instance);

/* Constants from the C++ header */
#define MMC56X3_I2C_ADDR 0x30
#define MMC56X3_CTRL_BW_ODR_20 0x00
#define MMC56X3_CTRL_BW_ODR_50 0x04
#define MMC56X3_CTRL_BW_ODR_100 0x08

/* Additional constants */
#define MMC56X3_PRODUCT_ID 0x10
#define MMC56X3_CTRL2_HPOWER 0x80

#ifdef __cplusplus
}
#endif

#endif /* MMC56X3_WRAPPER_H */