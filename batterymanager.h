#ifndef BATTERYMANAGER_H
#define BATTERYMANAGER_H

#include <QString>

class BatteryManager
{
public:
    BatteryManager();

    int batteryLevel() const;

    bool setChargeThreshold(int threshold);

private:
    QString batteryLevelPath;
    QString batteryStatusPath;
    QString batteryThresholdPath;
};

#endif // BATTERYMANAGER_H