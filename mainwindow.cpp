#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QSignalBlocker>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    updateBatteryInfo();
    loadSettingsToUi();

    // Dashboard threshold controls
    connect(ui->batt100, &QRadioButton::clicked, this, [this]() {
        if (!m_battery.setChargeThreshold(100)) {
            qDebug() << "Failed to change threshold to 100%";
        }
        updateBatteryInfo();
    });

    connect(ui->batt80, &QRadioButton::clicked, this, [this]() {
        if (!m_battery.setChargeThreshold(80)) {
            qDebug() << "Failed to change threshold to 80%";
        }
        updateBatteryInfo();
    });

    connect(ui->batt60, &QRadioButton::clicked, this, [this]() {
        if (!m_battery.setChargeThreshold(60)) {
            qDebug() << "Failed to change threshold to 60%";
        }
        updateBatteryInfo();
    });

    // Settings page controls
    connect(ui->btnDetectBatteries, &QPushButton::clicked, this, &MainWindow::onDetectBatteriesClicked);
    connect(ui->batterySelectCombo, &QComboBox::activated, this, &MainWindow::onBatteryComboActivated);
    connect(ui->btnBrowseBatteryDir, &QPushButton::clicked, this, &MainWindow::onBrowseBatteryDirClicked);
    connect(ui->btnBrowseLevel, &QPushButton::clicked, this, &MainWindow::onBrowseLevelClicked);
    connect(ui->btnBrowseStatus, &QPushButton::clicked, this, &MainWindow::onBrowseStatusClicked);
    connect(ui->btnBrowseThreshold, &QPushButton::clicked, this, &MainWindow::onBrowseThresholdClicked);
    connect(ui->btnSaveSettings, &QPushButton::clicked, this, &MainWindow::onSaveSettingsClicked);
    connect(ui->btnResetSettings, &QPushButton::clicked, this, &MainWindow::onResetSettingsClicked);

    // Auto-update file paths when user finishes typing a battery directory
    connect(ui->batteryDirEdit, &QLineEdit::editingFinished, this, [this]() {
        const QString dir = ui->batteryDirEdit->text().trimmed();
        if (!dir.isEmpty() && QDir(dir).exists()) {
            ui->levelPathEdit->setText(dir + "/capacity");
            ui->statusPathEdit->setText(dir + "/status");
            ui->thresholdPathEdit->setText(dir + "/charge_control_end_threshold");
        }
    });

    // Periodic dashboard refresh timer
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateBatteryInfo);
    timer->start(5000);
}

void MainWindow::updateBatteryInfo()
{
    const int level = m_battery.batteryLevel();
    if (level >= 0) {
        ui->batteryLabel->setText(QString("Battery: %1%").arg(level));
    } else {
        ui->batteryLabel->setText("Battery: Unknown");
    }

    const QString status = m_battery.batteryStatus();
    if (!status.isEmpty()) {
        ui->batteryStatus->setText(QString("Battery Status: %1").arg(status));
    } else {
        ui->batteryStatus->setText("Battery Status: Unknown");
    }

    const int threshold = m_battery.chargeThreshold();
    if (threshold >= 0) {
        ui->batteryThreshold->setText(QString("Charge limit: %1%").arg(threshold));

        const QSignalBlocker b1(ui->batt100);
        const QSignalBlocker b2(ui->batt80);
        const QSignalBlocker b3(ui->batt60);

        ui->batt100->setChecked(threshold == 100);
        ui->batt80->setChecked(threshold == 80);
        ui->batt60->setChecked(threshold == 60);
    } else {
        ui->batteryThreshold->setText("Charge limit: Unknown");
    }
}

void MainWindow::loadSettingsToUi()
{
    ui->levelPathEdit->setText(m_battery.getBatteryLevelPath());
    ui->statusPathEdit->setText(m_battery.getBatteryStatusPath());
    ui->thresholdPathEdit->setText(m_battery.getBatteryThresholdPath());

    QFileInfo fi(m_battery.getBatteryLevelPath());
    if (fi.exists() || fi.dir().exists()) {
        ui->batteryDirEdit->setText(fi.dir().absolutePath());
    }

    onDetectBatteriesClicked();
}

void MainWindow::onDetectBatteriesClicked()
{
    const QStringList detected = BatteryManager::detectBatteries();
    ui->batterySelectCombo->clear();

    if (detected.isEmpty()) {
        ui->batterySelectCombo->addItem("No batteries detected");
        ui->batterySelectCombo->setEnabled(false);
    } else {
        ui->batterySelectCombo->setEnabled(true);
        for (const QString &bat : detected) {
            ui->batterySelectCombo->addItem(bat);
        }

        const QString currentDir = ui->batteryDirEdit->text();
        int idx = ui->batterySelectCombo->findText(currentDir);
        if (idx >= 0) {
            ui->batterySelectCombo->setCurrentIndex(idx);
        }
    }
}

void MainWindow::onBatteryComboActivated(int index)
{
    const QString selectedDir = ui->batterySelectCombo->itemText(index);
    if (selectedDir.isEmpty() || selectedDir.startsWith("No batteries")) {
        return;
    }
    ui->batteryDirEdit->setText(selectedDir);
    ui->levelPathEdit->setText(selectedDir + "/capacity");
    ui->statusPathEdit->setText(selectedDir + "/status");
    ui->thresholdPathEdit->setText(selectedDir + "/charge_control_end_threshold");
    ui->settingsStatusLabel->setStyleSheet("");
    ui->settingsStatusLabel->setText("Paths updated from selection. Click Save Settings to apply.");
}

void MainWindow::onBrowseBatteryDirClicked()
{
    QString initialDir = ui->batteryDirEdit->text();
    if (initialDir.isEmpty() || !QDir(initialDir).exists()) {
        initialDir = "/sys/class/power_supply";
    }

    const QString dir = QFileDialog::getExistingDirectory(this, "Select Battery Directory", initialDir);
    if (!dir.isEmpty()) {
        ui->batteryDirEdit->setText(dir);
        ui->levelPathEdit->setText(dir + "/capacity");
        ui->statusPathEdit->setText(dir + "/status");
        ui->thresholdPathEdit->setText(dir + "/charge_control_end_threshold");
        ui->settingsStatusLabel->setStyleSheet("");
        ui->settingsStatusLabel->setText("Selected battery directory. Click Save Settings to apply.");
    }
}

void MainWindow::onBrowseLevelClicked()
{
    const QString file = QFileDialog::getOpenFileName(this, "Select Capacity File", ui->levelPathEdit->text());
    if (!file.isEmpty()) {
        ui->levelPathEdit->setText(file);
    }
}

void MainWindow::onBrowseStatusClicked()
{
    const QString file = QFileDialog::getOpenFileName(this, "Select Status File", ui->statusPathEdit->text());
    if (!file.isEmpty()) {
        ui->statusPathEdit->setText(file);
    }
}

void MainWindow::onBrowseThresholdClicked()
{
    const QString file = QFileDialog::getOpenFileName(this, "Select Threshold File", ui->thresholdPathEdit->text());
    if (!file.isEmpty()) {
        ui->thresholdPathEdit->setText(file);
    }
}

void MainWindow::onSaveSettingsClicked()
{
    const QString levelPath = ui->levelPathEdit->text().trimmed();
    const QString statusPath = ui->statusPathEdit->text().trimmed();
    const QString thresholdPath = ui->thresholdPathEdit->text().trimmed();

    m_battery.setPaths(levelPath, statusPath, thresholdPath);

    if (m_battery.saveConfig()) {
        ui->settingsStatusLabel->setStyleSheet("color: green; font-weight: bold;");
        ui->settingsStatusLabel->setText("Settings saved successfully!");
    } else {
        ui->settingsStatusLabel->setStyleSheet("color: red; font-weight: bold;");
        ui->settingsStatusLabel->setText("Failed to save settings to config.json");
    }

    updateBatteryInfo();
}

void MainWindow::onResetSettingsClicked()
{
    m_battery.setBatteryDirectory("/sys/class/power_supply/BAT1");
    loadSettingsToUi();
    ui->settingsStatusLabel->setStyleSheet("");
    ui->settingsStatusLabel->setText("Reset to default BAT1 paths. Click Save Settings to apply.");
}

MainWindow::~MainWindow()
{
    delete ui;
}


