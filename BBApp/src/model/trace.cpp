#include "trace.h"
#include "../lib/bb_lib.h"

#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QDialog>

Trace::Trace(bool active, int size)
{
    _active = active;
    _update = true;
    _type = OFF;

    _size = 0;
    _minBuf = nullptr;
    _maxBuf = nullptr;

    _binSize = 0.0;
    _start = 0.0;

    msFromEpoch = 0;

    Alloc(size);
}

Trace::~Trace()
{
    Destroy();
}

// Same as old operator=
// It is selectively choosing which values to copy
// Basically copies all non-user configurable values
void Trace::Copy(const Trace &other)
{
    //_active = other._active;
    //_type = other._type;

    _binSize = other._binSize;
    _start = other._start;

    Alloc(other._size);

    for(int i = 0; i < _size; i++) {
        _minBuf[i] = other._minBuf[i];
        _maxBuf[i] = other._maxBuf[i];
    }
}

// Destroy buffers and set to null
void Trace::Destroy()
{
    if(_minBuf) {
        delete [] _minBuf;
        _minBuf = nullptr;
    }
    if(_maxBuf) {
        delete [] _maxBuf;
        _maxBuf = nullptr;
    }
}

void Trace::Alloc(int newSize)
{
    if(_size == newSize)
        return;

    // Sizes different, delete and re-alloc
    Destroy();

    _size = newSize;
    _minBuf = new float[_size];
    _maxBuf = new float[_size];

    for(int i = 0; i < _size; i++) {
        _minBuf[i] = 0.0;
        _maxBuf[i] = 0.0;
    }
}

// Clear does not delete the data, just set the size
//   to zero, which triggers a copy
void Trace::Clear() {
    _size = 0;
}

void Trace::SetType(TraceType type)
{
    if(_type == type)
        return;

    // Clear contents, next update will generate new values
    Clear();

    _type = type;
}

// Returns negative frequency if not active to prevent
//  marker from updating on non-active trace
void Trace::GetSignalPeak(double *freq, double *amp) const
{
//    if(!_active) {
//        if(freq) {
//            *freq = -1.0;
//        }
//        return;
//    }

    double max = -1000.0;
    int maxIndex = 0;

    // Find max amp and index
    for(int i = 0; i < _size; i++) {
        if(_maxBuf[i] > max) {
            max = _maxBuf[i];
            maxIndex = i;
        }
    }

    if(amp) *amp = max;
    if(freq) *freq = _start + maxIndex * _binSize;
}

// Mean
double Trace::GetMean() const
{
    double sum = 0.0;

    for(int i = 0; i < _size; i++)
        sum += _maxBuf[i];

    return sum / _size;
}

// Deviation squared
double Trace::GetVariance() const
{
    double mean = GetMean();
    double variance = 0.0;

    for(int i = 0; i < _size; i++) {
        double deviation = mean - _maxBuf[i];
        variance += (deviation*deviation);
    }

    return variance / _size;
}

double Trace::GetVarianceFromMean(const double mean) const
{
    double variance = 0.0;

    for(int i = 0; i < _size; i++) {
        double deviation = mean - _maxBuf[i];
        variance += (deviation*deviation);
    }

    return variance / _size;
}

// Get one standard deviation
double Trace::GetStandardDeviation() const
{
    return sqrt(GetVariance());
}

// Return a list of indices representing the acceptable peaks
// Clear the list before beginning
void Trace::GetPeakList(std::vector<int> &peak_index_list) const
{
    peak_index_list.clear();
    double mean = GetMean();
    double stddev = sqrt(GetVarianceFromMean(mean));

    // Find the "beginning" of a peak
    for(int loc = 0; loc < _size - 2; loc++) {
        // Next point must be above the entry
        if((_maxBuf[loc] > (mean + stddev * 1.2))
            && (_maxBuf[loc+1] > _maxBuf[loc])) {

            double peakVal = _maxBuf[loc];
            unsigned int ix_peak = loc;

            // Find range
            while((_maxBuf[loc] > (mean)) && loc < _size) {
                loc++;

                if(_maxBuf[loc] > peakVal) {
                    peakVal = _maxBuf[loc];
                    ix_peak = loc;
                }
            }

            peak_index_list.push_back(ix_peak);
        }
    }
}

void Trace::Update(const Trace &other)
{
    if(!_update) {
        return;
    }

    // Check if equal? if not, set equal
    if(_size != other.Length()) {
        Copy(other);
    }

    _start = other._start;
    _binSize = other._binSize;
    settings = *other.GetSettings();

    if(_type == OFF) {
        _active = false;
        return;
    } else {
        _active = true;
    }

    switch(_type) {
    case NORMAL:
        for(int i = 0; i < _size; i++) {
            _minBuf[i] = other._minBuf[i];
            _maxBuf[i] = other._maxBuf[i];
        }
        break;
    case MAX_HOLD:
        for(int i = 0; i < _size; i++) {
            _maxBuf[i] = _minBuf[i] = bb_lib::max2(_maxBuf[i], other._maxBuf[i]);
        }
        break;
    case MIN_HOLD:
        for(int i = 0; i < _size; i++) {
            _maxBuf[i] = _minBuf[i] = bb_lib::min2(_minBuf[i], other._minBuf[i]);
        }
        break;
    case MIN_AND_MAX:
        for(int i = 0; i < _size; i++) {
            _minBuf[i] = bb_lib::min2(_minBuf[i], other._minBuf[i]);
            _maxBuf[i] = bb_lib::max2(_maxBuf[i], other._maxBuf[i]);
        }
        break;
    case AVERAGE_10:
        for(int i = 0; i < _size; i++) {
            _minBuf[i] = _minBuf[i]*0.9 + other._minBuf[i]*0.1;
            _maxBuf[i] = _maxBuf[i]*0.9 + other._maxBuf[i]*0.1;
        }
        break;
    case AVERAGE_100:
        for(int i = 0; i < _size; i++) {
            _minBuf[i] = _minBuf[i]*0.99 + other._minBuf[i]*0.01;
            _maxBuf[i] = _maxBuf[i]*0.99 + other._maxBuf[i]*0.01;
        }
        break;
    }
}

// Returns true if successful(path exists)
// spacing == frequency spacing of output file
// Set first point to multiple of spacing
bool Trace::Export(const QString &path) const
{
    QFile file(path);

    if(!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QTextStream out(&file);

    double freq = _start;
    for(int i = 0; i < _size; i++, freq += _binSize) {
        out << freq / 1.0e6 << ", ";
        out << _minBuf[i] << ", " << _maxBuf[i] << "\n";
    }

    file.close();

    return true;
}

bool Trace::GetChannelPower(double ch_start, double ch_stop, double *power) const
{
    if(ch_start < StartFreq() || ch_stop > StopFreq()) {
        *power = 0.0;
        return false;
    }

    pfn_convert to_lin, to_log;
    if(GetSettings()->RefLevel().IsLogScale()) {
        to_lin = DBMtoMW;
        to_log = MWtoDBM;
    } else {
        to_lin = MVtoMV2;
        to_log = MV2toMV;
    }

    int index = (int)((ch_start - StartFreq()) / BinSize()) + 1;
    bb_lib::clamp<int>(index, 0, Length());

    double step_freq = StartFreq() + BinSize() * index;
    double sum = 0.0;

    while(step_freq < ch_stop && index < Length()) {
        sum += to_lin(Max()[index]);
        step_freq += BinSize();
        index++;
    }

    sum /= GetSettings()->GetWindowBandwidth();
    *power = to_log(sum);

    return true;
}

void Trace::ApplyOffset(double dB) {
    if(settings.RefLevel().IsLogScale()) {
        for(int i = 0; i < _size; i++) {
            _minBuf[i] += dB;
            _maxBuf[i] += dB;
        }
    } else {
        double scalar = pow(10, dB / 20.0);
        for(int i = 0; i < _size; i++) {
            _minBuf[i] *= scalar;
            _maxBuf[i] *= scalar;
        }
    }
}

ChannelPower::ChannelPower() :
    enabled(false)
{

}

ChannelPower::~ChannelPower()
{

}

void ChannelPower::Configure(bool ch_enable, double ch_width, double ch_spacing)
{
    enabled = ch_enable;
    width = ch_width;
    spacing = ch_spacing;
    in_view[0] = in_view[1] = in_view[2] = false;
    ch_power[0] = ch_power[1] = ch_power[2] = 0.0;
}

void ChannelPower::Update(const Trace *trace)
{
    if(!enabled) return;

    double center = trace->GetSettings()->Center();

    ch_start[0] = center - spacing - (width / 2.0);
    ch_stop[0] = ch_start[0] + width;
    ch_start[1] = center - (width / 2.0);
    ch_stop[1] = center + (width / 2.0);
    ch_start[2] = center + spacing - (width / 2.0);
    ch_stop[2] = ch_start[2] + width;

    for(int i = 0; i < 3; i++) {
        in_view[i] = trace->GetChannelPower(ch_start[i], ch_stop[i], &ch_power[i]);
    }
}

bool ChannelPower::IsChannelInView(int channel) const
{
    if(channel < 0 || channel > 2)
        return false;

    return in_view[channel];
}

double ChannelPower::GetChannelStart(int channel) const
{
    if(channel < 0 || channel > 2)
        return 0.0;

    return ch_start[channel];
}

double ChannelPower::GetChannelStop(int channel) const
{
    if(channel < 0 || channel > 2)
        return 0.0;

    return ch_stop[channel];
}

double ChannelPower::GetChannelPower(int channel) const
{
    if(channel < 0 || channel > 2)
        return 0.0;

    return ch_power[channel];
}