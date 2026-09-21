#ifndef BATTERYMANAGER_H
#define BATTERYMANAGER_H

#include <QString>

class BatteryManager
{
public:
    BatteryManager();

    int batteryLevel() const;

private:
    QString batteryPath;
};

#endif // BATTERYMANAGER_H