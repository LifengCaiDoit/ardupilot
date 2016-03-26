/*
  SITL handling

  This simulates a compass

  Andrew Tridgell November 2011
 */

#include <AP_HAL/AP_HAL.h>
#if CONFIG_HAL_BOARD == HAL_BOARD_SITL

#include "AP_HAL_SITL.h"
#include "AP_HAL_SITL_Namespace.h"
#include "HAL_SITL_Class.h"

#include <AP_Math/AP_Math.h>
#include <AP_Compass/AP_Compass.h>
#include <AP_Declination/AP_Declination.h>
#include <SITL/SITL.h>

extern const AP_HAL::HAL& hal;

using namespace HALSITL;

/*
  setup the compass with new input
  all inputs are in degrees
 */
void SITL_State::_update_compass(float rollDeg, float pitchDeg, float yawDeg)
{
    static uint32_t last_update;

    if (_compass == NULL) {
        // no compass in this sketch
        return;
    }
    yawDeg += _sitl->mag_error;
    if (yawDeg > 180.0f) {
        yawDeg -= 360.0f;
    }
    if (yawDeg < -180.0f) {
        yawDeg += 360.0f;
    }

    // get earth field
    _update_Bfield();

    // rotate into body frame
    _compass->setHIL(0, radians(rollDeg), radians(pitchDeg), radians(yawDeg), &_Bearth);
    _compass->setHIL(1, radians(rollDeg), radians(pitchDeg), radians(yawDeg), &_Bearth);
    Vector3f noise = _rand_vec3f() * _sitl->mag_noise;
    Vector3f motor = _sitl->mag_mot.get() * _current;
    Vector3f new_mag_data = _compass->getHIL(0) + noise + motor;

    // 100Hz
    uint32_t now = AP_HAL::millis();
    if ((now - last_update) < 10) {
        return;
    }
    last_update = now;

    // add delay
    uint32_t best_time_delta_mag = 1000; // initialise large time representing buffer entry closest to current time - delay.
    uint8_t best_index_mag = 0; // initialise number representing the index of the entry in buffer closest to delay.

    // storing data from sensor to buffer
    if (now - last_store_time_mag >= 10) { // store data every 10 ms.
        last_store_time_mag = now;
        if (store_index_mag > mag_buffer_length-1) { // reset buffer index if index greater than size of buffer
            store_index_mag = 0;
        }
        buffer_mag[store_index_mag].data = new_mag_data; // add data to current index
        buffer_mag[store_index_mag].time = last_store_time_mag; // add time to current index
        store_index_mag = store_index_mag + 1; // increment index
    }

    // return delayed measurement
    delayed_time_mag = now - _sitl->mag_delay; // get time corresponding to delay
    // find data corresponding to delayed time in buffer
    for (uint8_t i=0; i<=mag_buffer_length-1; i++) {
        time_delta_mag = abs(delayed_time_mag - buffer_mag[i].time); // find difference between delayed time and time stamp in buffer
        // if this difference is smaller than last delta, store this time
        if (time_delta_mag < best_time_delta_mag) {
            best_index_mag = i;
            best_time_delta_mag = time_delta_mag;
        }
    }
    if (best_time_delta_mag < 1000) { // only output stored state if < 1 sec retrieval error
        new_mag_data = buffer_mag[best_index_mag].data;
    }

    new_mag_data -= _sitl->mag_ofs.get();

    _compass->setHIL(0, new_mag_data);
    _compass->setHIL(1, new_mag_data);
}

// calculate the magnetic field environment at the vehicles location
void SITL_State::_update_Bfield(void)
{
    // assume an earth field strength of 400
    _Bearth.x = 400.0f;
    _Bearth.y = 0.0f;
    _Bearth.z = 0.0f;

    // rotate _Bearth for inclination and declination. -66 degrees
    // is the inclination in Canberra, Australia
    Matrix3f R;
    R.from_euler(0, ToRad(66), _compass->get_declination());
    _Bearth = R * _Bearth;

    // add local ground based magnetic distortion effect assuming 1/R^2 reduction with height
    Vector3f ground_vec = _sitl->mag_gnd;
    ground_vec = ground_vec * sq(_sitl->mag_gnd_hgt / (height_agl() + _sitl->mag_gnd_hgt));
    _Bearth += ground_vec;
}

#endif
