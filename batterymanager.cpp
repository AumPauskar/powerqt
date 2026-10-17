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
#include <cmath>

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

double BatteryManager::powerConsumptionWatts() const
{
    double watts = 0.0;
    bool foundPowerReading = false;

    const QFileInfoList batteries = QDir("/sys/class/power_supply").entryInfoList(
        QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &battery : batteries) {
        QFile typeFile(battery.absoluteFilePath() + "/type");
        if (typeFile.open(QIODevice::ReadOnly | QIODevice::Text)
            && QTextStream(&typeFile).readLine().trimmed().compare("Battery", Qt::CaseInsensitive) != 0) {
            continue;
        }

        QFile statusFile(battery.absoluteFilePath() + "/status");
        if (statusFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QString status = QTextStream(&statusFile).readLine().trimmed();
            if (status.compare("Charging", Qt::CaseInsensitive) == 0
                || status.compare("Full", Qt::CaseInsensitive) == 0) {
                continue;
            }
        }

        QFile powerFile(battery.absoluteFilePath() + "/power_now");
        bool ok = false;
        qint64 microwatts = 0;
        if (powerFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            microwatts = QTextStream(&powerFile).readLine().trimmed().toLongLong(&ok);
        }

        if (!ok) {
            QFile currentFile(battery.absoluteFilePath() + "/current_now");
            QFile voltageFile(battery.absoluteFilePath() + "/voltage_now");
            bool currentOk = false;
            bool voltageOk = false;
            qint64 microamps = 0;
            qint64 microvolts = 0;
            if (currentFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                microamps = QTextStream(&currentFile).readLine().trimmed().toLongLong(&currentOk);
            }
            if (voltageFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                microvolts = QTextStream(&voltageFile).readLine().trimmed().toLongLong(&voltageOk);
            }
            if (currentOk && voltageOk) {
                watts += (std::abs(static_cast<double>(microamps))
                          * std::abs(static_cast<double>(microvolts))) / 1'000'000'000'000.0;
                foundPowerReading = true;
            }
        } else {
            watts += std::abs(static_cast<double>(microwatts)) / 1'000'000.0;
            foundPowerReading = true;
        }
    }

    return foundPowerReading ? watts : -1.0;
}

QString BatteryManager::fanSpeed() const
{
    QStringList readings;
    const QDir hwmonRoot("/sys/class/hwmon");
    const QFileInfoList controllers = hwmonRoot.entryInfoList(
        QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    for (const QFileInfo &controller : controllers) {
        const QDir controllerDir(controller.absoluteFilePath());
        const QFileInfoList fanFiles = controllerDir.entryInfoList(
            {"fan*_input"}, QDir::Files | QDir::System, QDir::Name);
        for (const QFileInfo &fanFile : fanFiles) {
            QFile file(fanFile.absoluteFilePath());
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                continue;
            }

            bool ok = false;
            const int rpm = QTextStream(&file).readLine().trimmed().toInt(&ok);
            if (ok) {
                readings.append(QString("Fan %1: %2 RPM").arg(readings.size() + 1).arg(rpm));
            }
        }
    }

    return readings.join("\n");
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
