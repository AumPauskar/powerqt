#ifndef BATTERYMANAGER_H
#define BATTERYMANAGER_H

#include <QString>

class BatteryManager
{
public:
    BatteryManager();

    int batteryLevel() const;
    QString batteryStatus() const;
    int chargeThreshold() const;

    bool setChargeThreshold(int threshold);

    QString getBatteryLevelPath() const { return batteryLevelPath; }
    QString getBatteryStatusPath() const { return batteryStatusPath; }
    QString getBatteryThresholdPath() const { return batteryThresholdPath; }
    QString getConfigFilePath() const { return configFilePath; }

    void setPaths(const QString &levelPath, const QString &statusPath, const QString &thresholdPath);
    void setBatteryDirectory(const QString &batteryDir);
    bool saveConfig(const QString &filePath = QString());
    bool loadConfig(const QString &filePath = QString());

    static QStringList detectBatteries();

private:
    QString batteryLevelPath;
    QString batteryStatusPath;
    QString batteryThresholdPath;
    QString configFilePath;
};

#endif // BATTERYMANAGER_H