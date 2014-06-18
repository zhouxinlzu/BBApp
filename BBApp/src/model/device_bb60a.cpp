#include "device_bb60a.h"
#include "../model/sweep_settings.h"
#include "../model/trace.h"

#include "../mainwindow.h"

#define STATUS_CHECK(status) \
    lastStatus = status; \
    if(lastStatus < bbNoError) { \
        return false; \
    } /* end */

// Maps software mode to device mode
//static int g_mode_map[] = {
//    SA_IDLE,          // idle
//    SA_SWEEPING,      // sweep
//    SA_REAL_TIME,     // rt
//    SA_IQ,            // zs
//    SA_IQ,            // audio
//    SA_IDLE           // sna // todo
//};

DeviceBB60A::DeviceBB60A(const Preferences *preferences) :
    Device(preferences)
{
    id = -1;
    open = false;
    serial_number = 0;
    firmware_ver = 0;
    lastStatus = bbNoError;

    timebase_reference = TIMEBASE_INTERNAL;
}

DeviceBB60A::~DeviceBB60A()
{
    CloseDevice();
}

bool DeviceBB60A::OpenDevice()
{
    if(open) {
        lastStatus = bbNoError;
        return true;
    }

    STATUS_CHECK(bbOpenDevice(&id));

    bbGetSerialNumber(id, &serial_number);
    bbGetFirmwareVersion(id, &firmware_ver);
    bbGetDeviceType(id, &device_type);

    open = true;
    return true;
}

bool DeviceBB60A::CloseDevice()
{
    if(!open) {
        lastStatus = bbDeviceNotOpenErr;
        return false;
    }

    bbCloseDevice(id);

    id = -1;
    open = false;
    device_type = BB_DEVICE_NONE;
    serial_number = 0;
    firmware_ver = 0;

    return true;
}

bool DeviceBB60A::Abort()
{
    if(!open) {
        lastStatus = bbDeviceNotOpenErr;
        return false;
    }

    STATUS_CHECK(bbAbort(id));
    return true;
}

bool DeviceBB60A::Preset()
{
    if(!open) {
        lastStatus = bbDeviceNotOpenErr;
        return false;
    }

    bbAbort(id);
    bbPreset(id);
    return true;
}

bool DeviceBB60A::Reconfigure(const SweepSettings *s, Trace *t)
{
    bbAbort(id);

    int scale = s->RefLevel().IsLogScale() ?
                BB_LOG_SCALE : BB_LIN_SCALE;
    int detector = (s->Detector() == BB_AVERAGE) ?
                BB_AVERAGE : BB_MIN_AND_MAX;
    int rbw_type = (s->NativeRBW()) ?
                BB_NATIVE_RBW : BB_NON_NATIVE_RBW;
    int rejection = (s->Rejection()) ?
                BB_SPUR_REJECT : BB_NO_SPUR_REJECT;
    double sweep_time = (s->Mode() == BB_REAL_TIME) ?
                (prefs->realTimeAccumulation / 1000.0) : s->SweepTime().Val();

    double reference = s->RefLevel().ConvertToUnits(AmpUnits::DBM);

    int port_one_mask;
    switch(timebase_reference) {
    case TIMEBASE_INTERNAL:
        port_one_mask = 0x0;
        break;
    case TIMEBASE_EXT_AC:
        port_one_mask = BB_PORT1_EXT_REF_IN | BB_PORT1_AC_COUPLED;
        break;
    case TIMEBASE_EXT_DC:
        port_one_mask = BB_PORT1_EXT_REF_IN | BB_PORT1_DC_COUPLED;
        break;
    }

    STATUS_CHECK(bbConfigureIO(id, port_one_mask, 0x0));

    // Scale based on amp unit type
    STATUS_CHECK(bbConfigureAcquisition(id, detector, scale));
    STATUS_CHECK(bbConfigureCenterSpan(id, s->Center(), s->Span()));
    STATUS_CHECK(bbConfigureLevel(id, reference, (s->Attenuation()-1) * 10));
    STATUS_CHECK(bbConfigureSweepCoupling(id, s->RBW(), s->VBW(), sweep_time,
                                          rbw_type, rejection));
    STATUS_CHECK(bbConfigureWindow(id, BB_NUTALL));
    STATUS_CHECK(bbConfigureProcUnits(id, s->ProcessingUnits()));
    STATUS_CHECK(bbConfigureGain(id, s->Gain() - 1));

    STATUS_CHECK(bbInitiate(id, s->Mode(), 0));

    unsigned int traceSize;
    double binSize, startFreq;
    STATUS_CHECK(bbQueryTraceInfo(id, &traceSize, &binSize, &startFreq));

    t->SetSettings(*s);
    t->SetSize(traceSize);
    t->SetFreq(binSize, startFreq);

    bbGetDeviceDiagnostics(id, &last_temp, &voltage, &current);

    return true;
}

bool DeviceBB60A::GetSweep(const SweepSettings *s, Trace *t)
{
    // Get new diagnostic info, determine whether to update the
    //  diagnostic string
    UpdateDiagnostics();

    // Print new diagnostics only when a value changed
    if(update_diagnostics_string) {
        QString diagnostics;
        diagnostics.sprintf("%.2f C  --  %.2f V", CurrentTemp(), Voltage());
        MainWindow::GetStatusBar()->SetDiagnostics(diagnostics);
        update_diagnostics_string = false;
    }

    if((fabs(current_temp - last_temp) > 2.0) || reconfigure_on_next) {
        Reconfigure(s, t);
        reconfigure_on_next = false;
    }

    // Manually handle some errors, and populate variables in the event of warnings
    lastStatus = bbFetchTrace_32f(id, t->Length(), t->Min(), t->Max());
    if(lastStatus == bbDeviceConnectionErr) {
        // True connection issue, throw signal
        // emit connectionIssue() throw warning
        emit connectionIssues();
        return false;
    } else if(lastStatus == bbUSBTimeoutErr) {
        // In the event of a timeout, and the device is in a sweep mode, try one more time
        if(s->Mode() == BB_SWEEPING) {
            lastStatus = bbFetchTrace_32f(id, t->Length(), t->Min(), t->Max());
            if(lastStatus < bbNoError) {
                // Second error in a row, disconnect device with issues
                emit connectionIssues();
                return false;
            }
        }
    }

    // After error checks, do warning checks
    if(lastStatus == bbADCOverflow) {
        adc_overflow = true;
    } else {
        adc_overflow = false;
    }

    return true;
}

bool DeviceBB60A::IsPowered() const
{
    if(device_type == BB_DEVICE_BB60A) {
        if(voltage < 4.4) {
            return false;
        }
    } else if(device_type == BB_DEVICE_BB60C) {
        if(voltage < 4.45) {
            return false;
        }
    }

    return true;
}