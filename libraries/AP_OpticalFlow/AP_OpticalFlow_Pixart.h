#pragma once

#include "OpticalFlow.h"
#include <AP_HAL/utility/OwnPtr.h>

class AP_OpticalFlow_Pixart : public OpticalFlow_backend
{
public:
    /// constructor
    AP_OpticalFlow_Pixart(OpticalFlow &_frontend);

    // init - initialise the sensor
    void init();

    // update - read latest values from sensor and fill in x,y and totals.
    void update(void);

private:
    AP_HAL::OwnPtr<AP_HAL::SPIDevice> _dev;

    struct RegData {
        uint8_t reg;
        uint8_t value;
    };
    
    static const uint8_t srom_data[];
    static const uint8_t srom_id;
    static const RegData init_data[];

    void reg_write(uint8_t reg, uint8_t value);
    uint8_t reg_read(uint8_t reg);
    int16_t reg_read16s(uint8_t reg);
    uint16_t reg_read16u(uint8_t reg);

    void srom_download(void);
    void load_configuration(void);

    bool timer(void);

    int32_t sum_x;
    int32_t sum_y;
    uint32_t last_print_ms;
};
