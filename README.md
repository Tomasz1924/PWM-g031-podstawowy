STM32G0 Dual-Channel PWM Controller with Dynamic Dead Time

This firmware implements a high-frequency, 
dual-channel complementary PWM controller with dynamically adjustable frequency and dead time. 
It is designed for the STM32G030 microcontroller family using the STMicroelectronics HAL library. 
The system samples two analog inputs (potentiometers) to seamlessly shift the output 
switching profiles in real time while outputting live telemetry data to an I2C-driven character LCD display.
