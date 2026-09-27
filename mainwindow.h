#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "batterymanager.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void updateBatteryInfo();
    void loadSettingsToUi();
    void onSaveSettingsClicked();
    void onResetSettingsClicked();
    void onBrowseBatteryDirClicked();
    void onDetectBatteriesClicked();
    void onBatteryComboActivated(int index);
    void onBrowseLevelClicked();
    void onBrowseStatusClicked();
    void onBrowseThresholdClicked();

private:
    Ui::MainWindow *ui;
    BatteryManager m_battery;
};
#endif // MAINWINDOW_H
