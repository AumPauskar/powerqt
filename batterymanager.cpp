#include "batterymanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTextStream>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

BatteryManager::BatteryManager()
{
    // Sensible defaults based on standard Linux power supply paths
    batteryLevelPath = "/sys/class/power_supply/BAT1/capacity";
    batteryStatusPath = "/sys/class/power_supply/BAT1/status";
    batteryThresholdPath = "/sys/class/power_supply/BAT1/charge_control_end_threshold";

    if (!QFile::exists(batteryLevelPath) && QFile::exists("/sys/class/power_supply/BAT0/capacity")) {
        batteryLevelPath = "/sys/class/power_supply/BAT0/capacity";
        batteryStatusPath = "/sys/class/power_supply/BAT0/status";
        batteryThresholdPath = "/sys/class/power_supply/BAT0/charge_control_end_threshold";
    }

    // Check multiple candidate locations for config.json
    const QStringList candidatePaths = {
        "config.json",
        QCoreApplication::applicationDirPath() + "/config.json",
        QCoreApplication::applicationDirPath() + "/../config.json",
        "config/battery.json"
    };

    for (const QString &path : candidatePaths) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QByteArray data = file.readAll();
            file.close();

            const QJsonDocument document = QJsonDocument::fromJson(data);
            if (document.isObject()) {
                const QJsonObject object = document.object();

                if (object.contains("batteryLevel")) {
                    batteryLevelPath = object["batteryLevel"].toString();
                }
                if (object.contains("batteryStatus")) {
                    batteryStatusPath = object["batteryStatus"].toString();
                }
                if (object.contains("batteryThreshold")) {
                    batteryThresholdPath = object["batteryThreshold"].toString();
                }
                break;
            }
        }
    }
}

int BatteryManager::batteryLevel() const
{
    QFile file(batteryLevelPath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return -1;
    }

    QTextStream stream(&file);

    return stream.readLine().trimmed().toInt();
}

QString BatteryManager::batteryStatus() const
{
    QFile file(batteryStatusPath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QTextStream stream(&file);

    return stream.readLine().trimmed();
}

int BatteryManager::chargeThreshold() const
{
    QFile file(batteryThresholdPath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return -1;
    }

    QTextStream stream(&file);
    bool ok = false;
    int threshold = stream.readLine().trimmed().toInt(&ok);
    return ok ? threshold : -1;
}

bool BatteryManager::setChargeThreshold(int threshold)
{
    // 1. Try direct write (succeeds if udev rule is configured or app has write permissions)
    QFile file(batteryThresholdPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << threshold << "\n";
        return true;
    }

    qDebug() << "Direct write failed (" << file.errorString() << "), attempting pkexec fallback...";

    // 2. Fallback to pkexec to elevate privileges via Polkit GUI dialog
    QProcess process;
    const QString command = QString("echo %1 > %2").arg(threshold).arg(batteryThresholdPath);
    process.start("pkexec", QStringList() << "sh" << "-c" << command);
    process.waitForFinished();

    if (process.exitCode() == 0) {
        qDebug() << "Threshold successfully updated via pkexec";
        return true;
    }

    qDebug() << "pkexec failed with exit code:" << process.exitCode();
    return false;
}