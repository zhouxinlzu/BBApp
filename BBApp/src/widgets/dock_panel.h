#ifndef DOCKPANEL_H
#define DOCKPANEL_H

#include <QDockWidget>
#include <QScrollArea>
#include <QVector>
#include <QSettings>

#include "../lib/macros.h"
#include "dock_page.h"

class DockPanel : public QDockWidget {
    Q_OBJECT

public:
    DockPanel(const QString &title,
              QWidget *parent);
    ~DockPanel() {}

    void AddPage(DockPage *page);
    QScrollArea* GetPanelWidget() { return scrollArea; }

    // Save and restore the collapsed state of the DockPages
    void SaveState(QSettings &);
    void RestoreState(const QSettings &);

protected:

private:
    QScrollArea *scrollArea;
    QWidget *scrollWidget;
    QVector<DockPage*> tabs;

public slots:
    void tabsChanged();

private slots:


private:
    DISALLOW_COPY_AND_ASSIGN(DockPanel)
};

#endif // DOCKPANEL_H