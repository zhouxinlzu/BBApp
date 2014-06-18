#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QApplication>
#include <QSettings>
#include <QFile>

#include "lib/bb_lib.h"

// Keep everything public for now
class Preferences {
public:
    Preferences() { Load(); }
    ~Preferences() { Save(); }

    void SetDefault() {
        programStyle = LIGHT_STYLE_SHEET;

        playbackDelay = 64;
        playbackMaxFileSize = 4;

        trace_width = 1.0;
        graticule_width = 1.0;
        graticule_stipple = true;

        sweepDelay = 0;
        realTimeAccumulation = 32.0;
    }

    void Load() {
        QSettings s(QSettings::IniFormat, QSettings::UserScope,
                    "SignalHound", "Preferences");

        int style = s.value("ProgramStyle", LIGHT_STYLE_SHEET).toInt();
        SetProgramStyle(style);

        playbackDelay = s.value("PlaybackPrefs/Delay", 64).toInt();
        playbackMaxFileSize = s.value("PlaybackPrefs/MaxFileSize", 4).toInt();

        trace_width = s.value("ViewPrefs/TraceWidth", 1.0).toFloat();
        graticule_width = s.value("ViewPrefs/GraticuleWidth", 1.0).toFloat();
        graticule_stipple = s.value("ViewPrefs/GraticuleStipple", true).toBool();

        sweepDelay = s.value("SweepPrefs/Delay", 0).toInt();
        realTimeAccumulation = s.value("SweepPrefs/RealTimeUpdate", 32.0).toDouble();
    }

    void Save() const {
        QSettings s(QSettings::IniFormat, QSettings::UserScope,
                    "SignalHound", "Preferences");

        s.setValue("ProgramStyle", programStyle);

        s.setValue("PlaybackPrefs/Delay", playbackDelay);
        s.setValue("PlaybackPrefs/MaxFileSize", playbackMaxFileSize);

        s.setValue("ViewPrefs/TraceWidth", trace_width);
        s.setValue("ViewPrefs/GraticuleWidth", graticule_width);
        s.setValue("ViewPrefs/GraticuleStipple", graticule_stipple);

        s.setValue("SweepPrefs/Delay", sweepDelay);
        s.setValue("SweepPrefs/RealTimeUpdate", realTimeAccumulation);
    }

    void SetProgramStyle(int newStyle) {
        QFile styleSheet;

        programStyle = newStyle;

        switch(programStyle) {
        case DARK_STYLE_SHEET:
            styleSheet.setFileName(":/style_sheet_dark.css");
            break;
        case LIGHT_STYLE_SHEET:
        default:
            styleSheet.setFileName(":/style_sheet_light.css");
            break;
        }

        if(!styleSheet.open(QFile::ReadOnly)) {
            return;
        }

        QString styleString = styleSheet.readAll();
        styleSheet.close();

        static_cast<QApplication*>
                (QApplication::instance())->
                setStyleSheet(styleString);

    }

    int programStyle;

    int playbackDelay; // In ms [32, 2048]
    int playbackMaxFileSize; // In GB [1, 256]

    // Range for trace_width [1.0, 5.0]
    float trace_width;
    float graticule_width;
    bool graticule_stipple;

    // Arbitrary sweep delay
    int sweepDelay; // In ms [0, 2048]
    double realTimeAccumulation; // In ms [1.0, 128.0]
};

#endif // PREFERENCES_H