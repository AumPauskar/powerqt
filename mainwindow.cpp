#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QButtonGroup>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QLayoutItem>
#include <QProcess>
#include <QRadioButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QStandardPaths>
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

    const double watts = m_battery.powerConsumptionWatts();
    if (watts >= 0.0) {
        ui->batteryPower->setText(QString("Power Consumption: %1 W").arg(watts, 0, 'f', 2));
    } else {
        ui->batteryPower->setText("Power Consumption: Unknown");
    }

    const QString fanSpeed = m_battery.fanSpeed();
    ui->fanSpeed->setText(fanSpeed.isEmpty() ? "Fan Speed: Unavailable" : fanSpeed);
    ui->cpuClock->setText(m_battery.cpuClockSpeed());
    ui->activeCpus->setText(QString("Active CPUs: %1").arg(m_battery.activeCpuCount()));
    ui->parkedCpus->setText(QString("Parked CPUs: %1").arg(m_battery.parkedCpuCount()));

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

    updatePowerProfile();
}

void MainWindow::updatePowerProfile()
{
    const QString executable = QStandardPaths::findExecutable("powerprofilesctl");
    if (executable.isEmpty()) {
        setPowerProfileButtonsEnabled(false);
        ui->powerProfileStatus->setText("powerprofilesctl is not available");
        return;
    }

    QProcess listProcess;
    listProcess.start(executable, {"list"});
    if (!listProcess.waitForFinished(1000) || listProcess.exitCode() != 0) {
        setPowerProfileButtonsEnabled(false);
        ui->powerProfileStatus->setText("Unable to read available power profiles");
        return;
    }

    QStringList profiles;
    const QRegularExpression profilePattern("^\\s*\\*?\\s*([a-zA-Z0-9_-]+):");
    const QStringList lines = QString::fromLocal8Bit(listProcess.readAllStandardOutput())
                                  .split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const int firstText = line.indexOf(QRegularExpression("\\S"));
        if (firstText > 2) {
            continue;
        }
        const QRegularExpressionMatch match = profilePattern.match(line);
        if (!match.hasMatch()) {
            continue;
        }
        const QString profile = match.captured(1);
        if (!profile.endsWith("Driver") && profile != "Degraded" && !profiles.contains(profile)) {
            profiles.append(profile);
        }
    }

    if (profiles.isEmpty()) {
        setPowerProfileButtonsEnabled(false);
        ui->powerProfileStatus->setText("No power profiles available");
        return;
    }
    rebuildPowerProfileButtons(profiles);

    QProcess getProcess;
    getProcess.start(executable, {"get"});
    if (!getProcess.waitForFinished(1000) || getProcess.exitCode() != 0) {
        setPowerProfileButtonsEnabled(false);
        ui->powerProfileStatus->setText("Unable to read active power profile");
        return;
    }

    const QString activeProfile = QString::fromLocal8Bit(getProcess.readAllStandardOutput()).trimmed();
    const QSignalBlocker blocker(m_powerProfileButtonGroup);
    const int activeIndex = m_powerProfileNames.indexOf(activeProfile);
    if (activeIndex >= 0) {
        if (QAbstractButton *button = m_powerProfileButtonGroup->button(activeIndex)) {
            button->setChecked(true);
            ui->powerProfileStatus->setText(QString("Active: %1").arg(button->text()));
        }
    } else {
        ui->powerProfileStatus->setText(QString("Active: %1").arg(activeProfile.isEmpty() ? "Unknown" : activeProfile));
    }
    ui->powerProfileStatus->setStyleSheet("");
    setPowerProfileButtonsEnabled(true);
}

void MainWindow::rebuildPowerProfileButtons(const QStringList &profiles)
{
    if (profiles == m_powerProfileNames && m_powerProfileButtonGroup) {
        return;
    }

    QGridLayout *layout = ui->powerProfileButtonsLayout;
    while (QLayoutItem *item = layout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    delete m_powerProfileButtonGroup;
    m_powerProfileButtonGroup = new QButtonGroup(this);
    m_powerProfileNames = profiles;

    const int columns = profiles.size() > 4 ? 2 : 1;
    for (int i = 0; i < profiles.size(); ++i) {
        QString label = profiles.at(i);
        label.replace('-', ' ');
        label = label.left(1).toUpper() + label.mid(1);
        auto *button = new QRadioButton(label, ui->powerManagementGroup);
        m_powerProfileButtonGroup->addButton(button, i);
        layout->addWidget(button, i / columns, i % columns);
    }
    connect(m_powerProfileButtonGroup, &QButtonGroup::idClicked,
            this, &MainWindow::onPowerProfileActivated);
}

void MainWindow::setPowerProfileButtonsEnabled(bool enabled)
{
    if (!m_powerProfileButtonGroup) {
        return;
    }
    for (QAbstractButton *button : m_powerProfileButtonGroup->buttons()) {
        button->setEnabled(enabled);
    }
}

void MainWindow::onPowerProfileActivated(int index)
{
    if (index < 0 || index >= m_powerProfileNames.size()) {
        return;
    }

    const QString profile = m_powerProfileNames.at(index);
    QProcess process;
    process.start(QStandardPaths::findExecutable("powerprofilesctl"), {"set", profile});
    if (process.waitForFinished(2000) && process.exitCode() == 0) {
        ui->powerProfileStatus->setStyleSheet("color: green;");
        ui->powerProfileStatus->setText(QString("Active: %1").arg(m_powerProfileButtonGroup->button(index)->text()));
    } else {
        ui->powerProfileStatus->setStyleSheet("color: red;");
        ui->powerProfileStatus->setText("Failed to change power profile");
        updatePowerProfile();
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
