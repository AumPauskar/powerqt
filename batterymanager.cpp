#include "batterymanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
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

    loadConfig();
}

bool BatteryManager::loadConfig(const QString &filePath)
{
    QStringList candidatePaths;
    if (!filePath.isEmpty()) {
        candidatePaths << filePath;
    } else {
        candidatePaths << "config.json"
                       << QCoreApplication::applicationDirPath() + "/config.json"
                       << QCoreApplication::applicationDirPath() + "/../config.json"
                       << QStandardPaths::locate(QStandardPaths::GenericDataLocation, "powerqt/config.json")
                       << "/usr/share/powerqt/config.json"
                       << "config/battery.json";
    }

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
                configFilePath = QFileInfo(file).absoluteFilePath();
                return true;
            }
        }
    }
    return false;
}

bool BatteryManager::saveConfig(const QString &filePath)
{
    QString targetPath = filePath.isEmpty() ? configFilePath : filePath;
    if (targetPath.isEmpty()) {
        targetPath = "config.json";
    }

    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Failed to open config file for writing:" << file.errorString();
        return false;
    }

    QJsonObject object;
    object["batteryLevel"] = batteryLevelPath;
    object["batteryStatus"] = batteryStatusPath;
    object["batteryThreshold"] = batteryThresholdPath;

    QJsonDocument doc(object);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    configFilePath = QFileInfo(file).absoluteFilePath();
    return true;
}

void BatteryManager::setPaths(const QString &levelPath, const QString &statusPath, const QString &thresholdPath)
{
    batteryLevelPath = levelPath;
    batteryStatusPath = statusPath;
    batteryThresholdPath = thresholdPath;
}

void BatteryManager::setBatteryDirectory(const QString &batteryDir)
{
    QString base = batteryDir;
    if (base.endsWith('/')) {
        base.chop(1);
    }
    batteryLevelPath = base + "/capacity";
    batteryStatusPath = base + "/status";
    batteryThresholdPath = base + "/charge_control_end_threshold";
}

QStringList BatteryManager::detectBatteries()
{
    QStringList batteries;
    QDir dir("/sys/class/power_supply");
    if (!dir.exists()) {
        return batteries;
    }

    const QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &info : entries) {
        // Check "type" file
        QFile typeFile(info.absoluteFilePath() + "/type");
        if (typeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString type = QTextStream(&typeFile).readLine().trimmed();
            if (type.compare("Battery", Qt::CaseInsensitive) == 0) {
                batteries << info.absoluteFilePath();
                continue;
            }
        }
        if (info.fileName().startsWith("BAT", Qt::CaseInsensitive)) {
            batteries << info.absoluteFilePath();
        }
    }
    return batteries;
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