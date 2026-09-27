# PowerQt

A lightweight, modern Qt6 desktop application for monitoring laptop battery health and controlling battery charge thresholds on Linux.

By setting charge thresholds (e.g., 60% or 80%), you can significantly reduce battery wear and extend the lifespan of your laptop's battery when plugged into AC power.

---

## Features

- **Battery Overview**: Real-time display of battery charge level (%), charging status, and active charge threshold limit.
- **One-Click Charge Limit Switching**:
  - **100%**: Full capacity (travel mode).
  - **80%**: Balanced capacity for everyday use.
  - **60%**: Maximum longevity mode for continuous AC/desk use.
- **Configurable Settings Page**:
  - Automatic detection of connected batteries (`BAT0`, `BAT1`, etc.).
  - Folder chooser to browse and select battery sysfs directories.
  - Manual overrides for specific sysfs file paths (`capacity`, `status`, `charge_control_end_threshold`).
  - Persistent settings stored in `config.json`.
- **Permission Handling**:
  - Direct write support with seamless `udev` integration (no password prompts).
  - Built-in Polkit (`pkexec`) graphical fallback prompt if write permissions are not pre-configured.

---

## Prerequisites

Ensure the following dependencies are installed on your Linux distribution:

### Arch Linux / Manjaro
```bash
sudo pacman -S base-devel cmake qt6-base polkit
```

### Ubuntu / Debian (23.04+) / Pop!_OS
```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev qt6-base-dev-tools libpolkit-gobject-1-dev
```

### Fedora
```bash
sudo dnf install gcc-c++ cmake qt6-qtbase-devel polkit
```

---

## Permissions Setup (Recommended)

Writing to Linux sysfs nodes (such as `charge_control_end_threshold`) requires root privileges by default. While **PowerQt** will automatically prompt for your password via `pkexec` if needed, you can configure a `udev` rule to allow seamless, passwordless threshold changes:

1. Create a udev rule file:
   ```bash
   sudo nano /etc/udev/rules.d/99-battery-threshold.rules
   ```

2. Add the following rule:
   ```udev
   SUBSYSTEM=="power_supply", ATTR{charge_control_end_threshold}!="", MODE="0666"
   ```

3. Reload and trigger the rule:
   ```bash
   sudo udevadm control --reload-rules && sudo udevadm trigger --subsystem-match=power_supply
   ```

---

## Building a Portable Application

You can build an optimized release binary using CMake:

```bash
# 1. Configure the project in Release mode
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release

# 2. Compile the application
cmake --build build-release
```

The compiled binary will be located at:
```bash
./build-release/powerqt
```

---

## Running the Application

### Running Portably
You can run the portable binary directly from the terminal or create a desktop shortcut:
```bash
./build-release/powerqt
```

PowerQt automatically looks for `config.json` in:
1. The current working directory
2. The same directory as the executable (`./build-release/config.json`)
3. The parent directory of the executable
4. Built-in defaults (`/sys/class/power_supply/BAT1/` or `BAT0`)

To keep your custom configuration bundled with the portable binary, copy `config.json` next to the executable:
```bash
cp config.json build-release/
```

### Creating an Install / Distributable Bundle
To generate a self-contained installation directory using CMake:
```bash
cmake --install build-release --prefix dist
```
This deploys the binary and runtime dependencies into the `dist/` directory.

---

## Configuration (`config.json`)

The application stores sysfs node paths in JSON format:

```json
{
    "batteryLevel": "/sys/class/power_supply/BAT1/capacity",
    "batteryStatus": "/sys/class/power_supply/BAT1/status",
    "batteryThreshold": "/sys/class/power_supply/BAT1/charge_control_end_threshold"
}
```

You can customize these paths directly in the file or through the **Settings** tab in the application.

---

## Troubleshooting

- **Battery shows -1% or Unknown**:
  Open the **Settings** tab, click **Scan** to detect your battery, select it from the dropdown, and click **Save Settings**.
- **Charge limit does not change**:
  Ensure your laptop kernel driver supports `charge_control_end_threshold` (common on ASUS, Lenovo ThinkPad, Huawei, Framework, and other modern laptops). Check with:
  ```bash
  cat /sys/class/power_supply/BAT1/charge_control_end_threshold
  ```
