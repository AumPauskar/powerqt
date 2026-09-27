#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QSignalBlocker>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    updateBatteryInfo();

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

MainWindow::~MainWindow()
{
    delete ui;
}


