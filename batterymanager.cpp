#include "batterymanager.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>

BatteryManager::BatteryManager()
    : batteryPath("/sys/class/power_supply/BAT1/capacity")
    , thresholdPath("/sys/class/power_supply/BAT1/charge_control_end_threshold")
{
}

int BatteryManager::batteryLevel() const
{
    QFile file(batteryPath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return -1;
    }

    QTextStream stream(&file);

    return stream.readLine().toInt();
}

bool BatteryManager::setChargeThreshold(int threshold)
{
    QFile file(thresholdPath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Failed to open threshold file:"
                 << file.errorString();
        return false;
    }

    QTextStream stream(&file);
    stream << threshold;

    return true;
}