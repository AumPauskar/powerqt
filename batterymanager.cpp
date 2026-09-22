#include "batterymanager.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

BatteryManager::BatteryManager()
{
    QFile file("config/battery.json");

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Could not open battery configuration:"
                 << file.errorString();
        return;
    }

    const QByteArray data = file.readAll();
    file.close();

    const QJsonDocument document = QJsonDocument::fromJson(data);

    if (!document.isObject()) {
        qDebug() << "Battery configuration is not a JSON object";
        return;
    }

    const QJsonObject object = document.object();

    batteryLevelPath =
        object["batteryLevel"].toString();

    batteryStatusPath =
        object["batteryStatus"].toString();

    batteryThresholdPath =
        object["batteryThreshold"].toString();
}

int BatteryManager::batteryLevel() const
{
    QFile file(batteryLevelPath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return -1;
    }

    QTextStream stream(&file);

    return stream.readLine().toInt();
}

bool BatteryManager::setChargeThreshold(int threshold)
{
    QFile file(batteryThresholdPath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Failed to open threshold file:"
                 << file.errorString();
        return false;
    }

    QTextStream stream(&file);
    stream << threshold;

    return true;
}