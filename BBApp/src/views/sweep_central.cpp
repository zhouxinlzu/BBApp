#include "sweep_central.h"

#include <QBoxLayout>
#include <QToolBar>
#include <QSplitter>
#include <QTreeView>
#include <QKeyEvent>

#include "trace_view.h"
#include "../mainwindow.h"
#include "../model/session.h"
#include "../model/trace.h"
#include "../model/playback_toolbar.h"
#include "../widgets/entry_widgets.h"
#include "../widgets/audio_dialog.h"

SweepCentral::SweepCentral(Session *sPtr, QWidget *parent, Qt::WindowFlags f)
    : CentralWidget(parent, f),
      session_ptr(sPtr),
      trace(true)
{
    trace_view = new TraceView(session_ptr, this);
    connect(this, SIGNAL(updateView()), trace_view, SLOT(update()));

    toolBar = new QToolBar();
    toolBar->setObjectName("SweepToolBar");
    toolBar->setMovable(false);
    toolBar->setFloatable(false);
    toolBar->layout()->setContentsMargins(0, 0, 0, 0);
    toolBar->layout()->setSpacing(0);
    toolBar->addWidget(new FixedSpacer(QSize(10, TOOLBAR_H)));

    Label *waterfall_label = new Label(tr("Spectrogram"), toolBar);
    waterfall_label->resize(100, 25);
    toolBar->addWidget(waterfall_label);
    toolBar->addWidget(new FixedSpacer(QSize(10, TOOLBAR_H)));

    waterfall_combo = new ComboBox(toolBar);
    waterfall_combo->setFixedSize(150, 25);
    QStringList spectrogram_list;
    spectrogram_list << tr("Off") << tr("2-D") << tr("3-D");
    waterfall_combo->insertItems(0, spectrogram_list);
    toolBar->addWidget(waterfall_combo);
    connect(waterfall_combo, SIGNAL(currentIndexChanged(int)),
            trace_view, SLOT(enableWaterfall(int)));

    toolBar->addWidget(new FixedSpacer(QSize(10, TOOLBAR_H)));
    toolBar->addSeparator();
    toolBar->addWidget(new FixedSpacer(QSize(10, TOOLBAR_H)));

    persistence_check = new QCheckBox(tr("Persistence"));
    persistence_check->setObjectName("SH_CheckBox");
    persistence_check->setFixedSize(120, 25);
    toolBar->addWidget(persistence_check);
    connect(persistence_check, SIGNAL(stateChanged(int)),
            trace_view, SLOT(enablePersistence(int)));

    persistence_clear = new QPushButton(tr("Clear"), toolBar);
    persistence_clear->setObjectName("BBPushButton");
    persistence_clear->setFixedSize(100, TOOLBAR_H - 4);
    toolBar->addWidget(persistence_clear);
    connect(persistence_clear, SIGNAL(clicked()), trace_view, SLOT(clearPersistence()));

    if(!trace_view->HasOpenGL3()) {
        persistence_check->setEnabled(false);
        persistence_check->setToolTip(tr("Persistence requires OpenGL version 3.0 or greater"));
        persistence_clear->setEnabled(false);
        persistence_clear->setToolTip(tr("Persistence requires OpenGL version 3.0 or greater"));
    }

    QWidget *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    toolBar->addWidget(spacer);

    single_sweep = new QPushButton(tr("Single Trig"), toolBar);
    single_sweep->setObjectName("BBPushButton");
    single_sweep->setFixedSize(120, TOOLBAR_H - 4);
    toolBar->addWidget(single_sweep);

    continuous_sweep = new QPushButton(tr("Continuous"), toolBar);
    continuous_sweep->setObjectName("BBPushButton");
    continuous_sweep->setFixedSize(120, TOOLBAR_H - 4);
    toolBar->addWidget(continuous_sweep);

    connect(single_sweep, SIGNAL(clicked()), SLOT(singleSweepPressed()));
    connect(continuous_sweep, SIGNAL(clicked()), SLOT(continuousSweepPressed()));

    toolBar->addWidget(new FixedSpacer(QSize(10, TOOLBAR_H)));
    toolBar->addSeparator();
    toolBar->addWidget(new FixedSpacer(QSize(10, TOOLBAR_H)));

    preset_button = new QPushButton(tr("Preset"), toolBar);
    preset_button->setObjectName("BBPresetButton");
    preset_button->setFixedSize(120, TOOLBAR_H - 4);
    toolBar->addWidget(preset_button);

    connect(preset_button, SIGNAL(clicked()), this, SIGNAL(presetDevice()));

    toolBar->addWidget(new FixedSpacer(QSize(10, TOOLBAR_H)));

    trace_view->move(0, 0);
    trace_view->resize(size().width(), size().height() - 32);

    playback = new PlaybackToolBar(&session_ptr->prefs, this);
    playback->move(0, trace_view->height());
    playback->resize(size().width(), 32);

    sweeping = true;
    sweep_count = -1;
    reconfigure = false;

    connect(session_ptr->sweep_settings, SIGNAL(updated(const SweepSettings*)),
            this, SLOT(settingsChanged(const SweepSettings*)));
    connect(session_ptr->trace_manager, SIGNAL(updated()),
            this, SLOT(forceUpdateView()));
    connect(playback, SIGNAL(startPlaying(bool)),
            this, SLOT(playFromFile(bool)));

    //StartStreaming();
    setFocusPolicy(Qt::StrongFocus);
}

SweepCentral::~SweepCentral()
{
    playback->Stop();
    StopStreaming();

    //delete tool_bar;
    delete trace_view;
    delete playback;
}

void SweepCentral::changeMode(int new_state)
{
    StopStreaming();
    sweep_count = -1;

    session_ptr->sweep_settings->setMode((OperationalMode) new_state);

    if(new_state == BB_SWEEPING || new_state == BB_REAL_TIME) {
        StartStreaming();
    }
}

void SweepCentral::StartStreaming()
{
    sweeping = true;

    if(session_ptr->sweep_settings->Mode() == BB_REAL_TIME) {
        persistence_check->setChecked(true);
        trace_view->enablePersistence(Qt::Checked);
    }

    thread_handle = std::thread(&SweepCentral::SweepThread, this);

//    if(mode_to_start == BB_SWEEPING) {
//        thread_handle = std::thread(&SweepCentral::SweepThread, this);
//    } else if(mode_to_start == BB_REAL_TIME) {
//        thread_handle = std::thread(&SweepCen)
//    }
}

void SweepCentral::StopStreaming()
{
    if(thread_handle.joinable()) {
        sweeping = false;
        thread_handle.join();
    }
}

void SweepCentral::ResetView()
{
    persistence_check->setChecked(false);
    waterfall_combo->setCurrentIndex(WaterfallOFF);
}

void SweepCentral::GetViewImage(QImage &image)
{
    image = trace_view->grabFrameBuffer();
}

void SweepCentral::resizeEvent(QResizeEvent *)
{
    trace_view->resize(size().width(), size().height() - 32);
    playback->move(0, trace_view->height());
    playback->resize(size().width(), 32);
}

void SweepCentral::keyPressEvent(QKeyEvent *e)
{
    if(e->key() == Qt::Key_Left || e->key() == Qt::Key_Right) {
        session_ptr->trace_manager->BumpMarker(e->key() == Qt::Key_Right);
        trace_view->update();
    }
}

// Try new settings
// If new settings fail, revert to old settings
void SweepCentral::Reconfigure()
{
    if(!session_ptr->device->Reconfigure(
                session_ptr->sweep_settings, &trace)) {
        *session_ptr->sweep_settings = last_config;
//        session_ptr->device->Reconfigure(
//                    session_ptr->sweep_settings, &trace);
    } else {
        last_config = *session_ptr->sweep_settings;
    }

//    QString diagnostics;
//    diagnostics.sprintf("%.2f C  --  %.2f V", session_ptr->device->CurrentTemp(),
//                        session_ptr->device->Voltage());
//    MainWindow::GetStatusBar()->SetDiagnostics(diagnostics);

    if(sweep_count == 0) {
        sweep_count = 1;
    }
    reconfigure = false;
}

// Main sweep loop
void SweepCentral::SweepThread()
{
    Reconfigure();

    while(sweeping) {
        if(reconfigure) {
            Reconfigure();
            session_ptr->trace_manager->ClearAllTraces();
        }

        if(sweep_count) {
            if(!session_ptr->device->GetSweep(session_ptr->sweep_settings, &trace)) {
                MainWindow::GetStatusBar()->SetMessage(session_ptr->device->GetLastStatusString());
            }

            if(trace.IsFullSweep()) {
                playback->PutTrace(&trace);
            }

            session_ptr->trace_manager->UpdateTraces(&trace);
            emit updateView();

            // Non-negative sweep count means we only collect 'n' more sweeps
            if(sweep_count > 0 && trace.IsFullSweep()) {
                sweep_count--;
            }

            // Artificial sweep delay, should be used sparingly
            if((session_ptr->sweep_settings->Mode() == BB_SWEEPING) &&
                    (session_ptr->prefs.sweepDelay > 0)) {
                Sleep(session_ptr->prefs.sweepDelay);
            }
        } else {
            Sleep(10);
        }
    }
}

/*
 * Save current settings and title to temporaries
 * When finally complete, restore them
 */
void SweepCentral::PlaybackThread()
{
    SweepSettings playback_settings, temp_settings;
    QString playback_title, temp_title;

    temp_settings = *session_ptr->sweep_settings;
    temp_title = session_ptr->GetTitle();

    playback->GetPlaybackSettings(&playback_settings, playback_title);

    trace.SetSettings(playback_settings);
    *session_ptr->sweep_settings = playback_settings;
    session_ptr->SetTitle(playback_title);

    // During playback, updates to sweeps settings do nothing
    disconnect(session_ptr->sweep_settings, SIGNAL(updated(const SweepSettings*)),
               this, SLOT(settingsChanged(const SweepSettings*)));

    while(playback->GetTrace(&trace) && sweeping) {   
        session_ptr->trace_manager->UpdateTraces(&trace);
        trace_view->update();
    }

    // Restore sweep settings before the playback stop?
    session_ptr->SetTitle(temp_title);
    *session_ptr->sweep_settings = temp_settings;

    // Force stop the playback
    playback->Stop();

    // Reattach settings updates
    connect(session_ptr->sweep_settings, SIGNAL(updated(const SweepSettings*)),
            this, SLOT(settingsChanged(const SweepSettings*)));
}

void SweepCentral::settingsChanged(const SweepSettings *ss)
{
    // Do nothing if
    //if(session_ptr->sweep_settings->Mode() != BB_SWEEPING &&
    //        session_ptr->sweep_settings->mode != BB_REAL_TIME) return;

    // If reconfigure already queued, wait for it
    //while(reconfigure && session_ptr->device->IsOpen()) { Sleep(1); }
    // Queue another
    reconfigure = true;
}

void SweepCentral::singleSweepPressed()
{
    if(sweep_count <= 0) {
        sweep_count = 1;
    }
}

void SweepCentral::continuousSweepPressed()
{
    sweep_count = -1;
}

// Just update view
void SweepCentral::forceUpdateView()
{
    emit updateView();
}

void SweepCentral::playFromFile(bool play)
{
    StopStreaming();

    if(play) {
        sweeping = true;
        session_ptr->isInPlaybackMode = true;
        thread_handle = std::thread(&SweepCentral::PlaybackThread, this);
    } else {
        session_ptr->isInPlaybackMode = false;
        emit updateView();
        if(session_ptr->device->IsOpen()) {
            StartStreaming();
        }
    }
}
