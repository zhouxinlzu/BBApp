#include "device_traits.h"

#include "sa_api.h"
#include "bb_api.h"

#include <QtGlobal>

static double max_bw_table_bb[] = {
    27.0e6,
    17.8e6,
    8.0e6,
    3.75e6,
    2.0e6,
    1.0e6,
    0.5e6,
    0.25e6
};

static double max_bw_table_sa[] = {
    250.0e3,
    225.0e3,
    100.0e3,
    50.0e3,
    20.0e3,
    12.0e3,
    5.0e3,
    3.0e3
};

DeviceType device_traits::type = DeviceTypeBB60C;

void device_traits::set_device_type(DeviceType new_type)
{
    type = new_type;
}

double device_traits::min_span()
{
    switch(type) {
    case DeviceTypeSA44: case DeviceTypeSA124: return 10.0;
    case DeviceTypeBB60A: case DeviceTypeBB60C: return BB_MIN_SPAN;
    }
    return 100.0;
}

double device_traits::min_frequency()
{
    switch(type) {
    case DeviceTypeSA44: case DeviceTypeSA124: return SA_MIN_FREQ;
    case DeviceTypeBB60A: case DeviceTypeBB60C: return BB60_MIN_FREQ;
    }
    return BB60_MIN_FREQ;
}

double device_traits::min_iq_frequency()
{
    switch(type) {
    case DeviceTypeSA44: case DeviceTypeSA124: return SA_MIN_FREQ;
    case DeviceTypeBB60A: case DeviceTypeBB60C: return 20.0e6;
    }
    return 20.0e6;
}

double device_traits::best_start_frequency()
{
    switch(type) {
    case DeviceTypeSA44: case DeviceTypeSA124: return 100.0e3;
    case DeviceTypeBB60A: case DeviceTypeBB60C: return 11.0e6;
    }
    return 10.0e6;
}

double device_traits::max_frequency()
{
    switch(type) {
    case DeviceTypeSA44: return SA_MAX_FREQ;
    case DeviceTypeSA124: return 12.4e9;
    case DeviceTypeBB60A: case DeviceTypeBB60C: return 6.0e9;
    }
    return 6.0e9;
}

double device_traits::min_real_time_rbw()
{
    switch(type) {
    case DeviceTypeSA44: case DeviceTypeSA124: return SA_MIN_RT_RBW;
    case DeviceTypeBB60A: case DeviceTypeBB60C: return BB_MIN_RT_RBW;
    }
    return BB_MIN_RT_RBW;
}

double device_traits::max_real_time_rbw()
{
    switch(type) {
    case DeviceTypeSA44: case DeviceTypeSA124: return SA_MAX_RT_RBW;
    case DeviceTypeBB60A: case DeviceTypeBB60C: return BB_MAX_RT_RBW;
    }
    return BB_MAX_RT_RBW;
}

double device_traits::min_real_time_span()
{
    switch(type) {
    case DeviceTypeSA44: case DeviceTypeSA124: return 100.0e3;
    case DeviceTypeBB60A: case DeviceTypeBB60C: return BB_MIN_RT_SPAN;
    }
    return BB_MIN_RT_SPAN;
}

double device_traits::max_real_time_span()
{
    switch(type) {
    case DeviceTypeSA44: case DeviceTypeSA124: return 250.0e3;
    case DeviceTypeBB60A: return BB60A_MAX_RT_SPAN;
    case DeviceTypeBB60C: return BB60C_MAX_RT_SPAN;
    }
    return BB_MIN_RT_SPAN;
}

double device_traits::min_iq_bandwidth()
{
    switch(type) {
    case DeviceTypeSA44: case DeviceTypeSA124: return 1.0e3;
    case DeviceTypeBB60A: case DeviceTypeBB60C: return 100.0e3;
    }
    return 100.0e3;
}

double device_traits::max_iq_bandwidth(int decimation_order)
{
    Q_ASSERT(decimation_order >= 0 && decimation_order <= 7);

    switch(type) {
    case DeviceTypeSA44: case DeviceTypeSA124:
        return max_bw_table_sa[decimation_order];
    case DeviceTypeBB60A: case DeviceTypeBB60C:
        return max_bw_table_bb[decimation_order];
    }
    return max_bw_table_bb[decimation_order];
}

int device_traits::max_atten()
{
    switch(type) {
    case DeviceTypeSA44: case DeviceTypeSA124: return 2;
    case DeviceTypeBB60A: case DeviceTypeBB60C: return 3;
    }
    return 3;
}

int device_traits::max_gain()
{
    switch(type) {
    case DeviceTypeSA44: case DeviceTypeSA124: return 2;
    case DeviceTypeBB60A: case DeviceTypeBB60C: return 3;
    }
    return 3;
}

double device_traits::sample_rate()
{
    switch(type) {
    case DeviceTypeSA44: case DeviceTypeSA124: return 486.111e3;
    case DeviceTypeBB60A: case DeviceTypeBB60C: return 40.0e6;
    }
    return BB_SAMPLERATE;
}

bool device_traits::default_spur_reject()
{
    switch(type) {
    case DeviceTypeSA44: case DeviceTypeSA124: return true;
    case DeviceTypeBB60A: case DeviceTypeBB60C: return false;
    }
    return false;
}

int device_traits::mod_analysis_decimation_order()
{
    switch(type) {
    case DeviceTypeSA44: case DeviceTypeSA124: return 0;
    case DeviceTypeBB60A: case DeviceTypeBB60C: return 7;
    }
    return 7;
}

int device_traits::audio_rate()
{
    switch(type) {
    case DeviceTypeSA44: case DeviceTypeSA124: return 30382;
    case DeviceTypeBB60A: case DeviceTypeBB60C: return 32000;
    }
    return 32000;
}