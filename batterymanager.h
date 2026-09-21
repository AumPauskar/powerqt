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
    QString batteryPath;
    QString thresholdPath;
};

#endif // BATTERYMANAGER_H