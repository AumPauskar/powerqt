#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "batterymanager.h"
#include <iostream>

#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    BatteryManager battery;

    const int level = battery.batteryLevel();

    ui->batteryLabel->setText(
        QString("Battery: %1%").arg(level)
        );
    qDebug() << "Going into the debug loop";
    bool success = battery.setChargeThreshold(80);

    if (success) {
        qDebug() << "Threshold changed successfully";
        std::cout << "Threshold changed successfully" << std::endl;
    } else {
        qDebug() << "Failed to change threshold";
        std::cout << "Failed to change successfully" << std::endl;
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}


