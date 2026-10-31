# STM32G0 Dual-Channel PWM Controller with Dynamic Dead Time

This firmware implements a high-frequency, dual-channel complementary PWM controller with dynamically adjustable frequency and dead time. It is designed for the STM32G030 microcontroller family using the STMicroelectronics HAL library. The system samples two analog inputs (potentiometers) to seamlessly shift the output switching profiles in real time while outputting live telemetry data to an I2C-driven character LCD display.

## 🛠️ Hardware Requirements & Pinout
* **Microcontroller:** STM32G030 (e.g., STM32G030F6P6 / K8T6)
* **Peripherals used:**
  * **TIM1:** Advanced-control timer for complementary PWM generation with Dead-Time Insertion (DTI).
  * **ADC:** Used in scan mode (or with DMA) to read the two potentiometers.
  * **I2C1:** Used to drive the HD44780 LCD (via PCF8574 I2C adapter).

### Pin Mapping (Example - update with your actual configuration)

| Peripheral | Pin | Function | Connection |
|---|---|---|---|
| **TIM1_CH1** | PA8 | PWM Output A | Power Stage / Scope Ch1 |
| **TIM1_CH1N**| PA7 | Complementary PWM B | Power Stage / Scope Ch2 |
| **ADC_IN0**  | PA0 | Analog Input 1 | Potentiometer 1 (Frequency) |
| **ADC_IN1**  | PA1 | Analog Input 2 | Potentiometer 2 (Dead Time) |
| **I2C1_SCL** | PA9 | I2C Clock | LCD SCL |
| **I2C1_SDA** | PA10| I2C Data | LCD SDA |

## 🚀 Key Features
* **Complementary PWM Generation:** High-frequency switching profiles with hardware-enforced dead time to prevent shoot-through currents.
* **Dynamic Adjustment:** Real-time updates to TIM1 registers (`ARR` for frequency, `BDTR` for dead time) based on ADC inputs.
* **Live Telemetry:** Smooth LCD refresh displaying the current switching frequency (Hz/kHz) and dead-time duration (ns/µs).

## 💻 Software & Toolchain
* **IDE:** STM32CubeIDE (v1.x.x)
* **Library:** STM32Cube FW_G0 V1.x.x (HAL Drivers)

## 🔧 How to Build and Flash
1. Clone this repository:
   ```bash
   git clone https://github.com
   ```
2. Open **STM32CubeIDE** and select **Import** -> **Existing Projects into Workspace**.
3. Select the root directory of this project.
4. Click **Build** (Hammer icon) to compile the source code.
5. Connect your ST-LINK programmer and click **Run** / **Debug** to flash the target STM32G030 microcontroller.
