/* MMC56x3_wrapper.cpp - Implementation of the C wrapper for MMC56x3 */

#include "MMC56x3_wrapper.h"
#include "MMC56x3.hpp"

/* The opaque struct is actually a pointer to the C++ class */
struct MMC56x3_Instance 
{
    MMC56x3* instance;
};

MMC56x3_Instance* MMC56x3_Create(i2c_inst_t* i2c_instance, uint8_t address) 
{
    MMC56x3_Instance* wrapper = new MMC56x3_Instance;
    wrapper->instance = new MMC56x3(i2c_instance, address);
    return wrapper;
}

void MMC56x3_Destroy(MMC56x3_Instance* wrapper) 
{
    if (wrapper) 
    {
        delete wrapper->instance;
        delete wrapper;
    }
}

bool MMC56x3_Begin(MMC56x3_Instance* wrapper) 
{
    if (wrapper && wrapper->instance) 
    {
        return wrapper->instance->begin();
    }
    return false;
}

void MMC56x3_Reset(MMC56x3_Instance* wrapper) 
{
    if (wrapper && wrapper->instance) 
    {
        wrapper->instance->reset();
    }
}

void MMC56x3_SetDataRate(MMC56x3_Instance* wrapper, uint8_t rate) 
{
    if (wrapper && wrapper->instance) 
    {
        wrapper->instance->setDataRate(rate);
    }
}

bool MMC56x3_ReadData(MMC56x3_Instance* wrapper, MMC56x3_MagData* data) 
{
    if (wrapper && wrapper->instance) 
    {
        MMC56x3::MagData cppData;
        bool result = wrapper->instance->readData(cppData);
        if (result) 
        {
            data->x = cppData.x;
            data->y = cppData.y;
            data->z = cppData.z;
            data->temperature = cppData.temperature;
        }
        return result;
    }
    return false;
}

/* New functions to expose more C++ functionality */

float MMC56x3_ReadTemperature(MMC56x3_Instance* wrapper) 
{
    if (wrapper && wrapper->instance) 
    {
        return wrapper->instance->readTemperature();
    }
    return 0.0f; // Return 0 for failure case 
}

void MMC56x3_MagnetSetReset(MMC56x3_Instance* wrapper) 
{
    if (wrapper && wrapper->instance) 
    {
        wrapper->instance->magnetSetReset();
    }
}

void MMC56x3_SetContinuousMode(MMC56x3_Instance* wrapper, bool mode) 
{
    if (wrapper && wrapper->instance) 
    {
        wrapper->instance->setContinuousMode(mode);
    }
}

bool MMC56x3_IsContinuousMode(MMC56x3_Instance* wrapper) 
{
    if (wrapper && wrapper->instance) 
    {
        return wrapper->instance->isContinuousMode();
    }
    return false;
}