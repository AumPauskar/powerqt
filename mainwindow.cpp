#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "batterymanager.h"

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
}

MainWindow::~MainWindow()
{
    delete ui;
}


