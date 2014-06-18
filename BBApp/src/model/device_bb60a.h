#ifndef DEVICE_BB60A_H
#define DEVICE_BB60A_H

#include "device.h"

class Preferences;

class DeviceBB60A : public Device {
public:
    DeviceBB60A(const Preferences *preferences);
    virtual ~DeviceBB60A();

    bool OpenDevice();
    bool CloseDevice();
    bool Abort();
    bool Preset();
    bool Reconfigure(const SweepSettings *s, Trace *t);
    bool GetSweep(const SweepSettings *s, Trace *t);

    bool IsPowered() const;

private:
    DISALLOW_COPY_AND_ASSIGN(DeviceBB60A)
};

#endif // DEVICE_BB60A_H