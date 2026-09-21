#include "batterymanager.h"

#include <QFile>
#include <QTextStream>

BatteryManager::BatteryManager()
    : batteryPath("/sys/class/power_supply/BAT1/capacity")
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