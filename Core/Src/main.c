/* Includes */
#include "main.h"
#include "stm32g0xx_hal.h"
#include "stm32g0xx_hal_i2c.h"
#include <stdio.h>
#include <string.h>

/* Defines */
// Dead time configuration limits (in timer ticks)
#define MIN_DEAD_TIME 5
#define MAX_DEAD_TIME 255


// Frequency limits (in Hz)
#define MIN_FREQUENCY 7500     // 7 500 kHZ
#define MAX_FREQUENCY 400000  // Increased to 400kHz
uint32_t targetFreq=100000;

// PWM configuration
#define PWM_DUTY_CYCLE 499    // 49.9% duty cycle (scale 0-1000)
#define TIMER_CLOCK 64000000  // Timer clock frequency (64MHz)

// LCD defines
#define LCD_ADDR 0x27 //27 !!!
#define LCD_BACKLIGHT 0x08
#define LCD_ENABLE 0x04
#define LCD_COMMAND 0x00
#define LCD_DATA 0x01

/* Global Variables */
ADC_HandleTypeDef hadc1;
TIM_HandleTypeDef htim1;
I2C_HandleTypeDef hi2c1;

// Previous variables remain the same...
I2C_HandleTypeDef hi2c1;
char lcdBuffer[20];  // Buffer for LCD text 16(20),


// ADC readings storage
uint32_t adcDeadTime = 0;     // Dead time value from potentiometer
uint32_t adcFrequency = 0;    // Frequency value from potentiometer
//#define VREFINT_CAL_ADDR ((uint16_t*)0x1FFFF7BA) // Calibrating address Vrefint (z DS)

uint32_t deadTime=10;

/* Function Prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM1_Init(void);
void UpdatePWMParameters(void);



/* Function Prototypes */
// Previous prototypes remain the same...
static void MX_I2C1_Init(void);
void LCD_Init(void);
void LCD_SendCommand(uint8_t cmd);
void LCD_SendData(uint8_t data);
void LCD_SendString(char *str);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_Clear(void);
void UpdateDisplay(void);

int MeasureVdda(void) {
    uint16_t vrefint_adc = 0;     // variable ADC dla Vrefint
    uint16_t vrefint_cal = *VREFINT_CAL_ADDR; // Read calibrating Vrefint

    // Konfiguracja ADC dla kanału Vrefint
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_VREFINT;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    // Start konwersji ADC
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY) == HAL_OK) {
        vrefint_adc = HAL_ADC_GetValue(&hadc1);
    }
    HAL_ADC_Stop(&hadc1);

    // Oblicz Vdda
    float vdda = ((float)vrefint_cal / (float)vrefint_adc) * 4096;

    return (int) vdda;
}




/**
 * @brief I2C write function for LCD
 * @param data: byte to send
 * @param flags: command or data flags
 */
void LCD_Write(uint8_t data, uint8_t flags) {
    uint8_t up = data & 0xF0;
    uint8_t lo = (data << 4) & 0xF0;

    uint8_t data_arr[4];
    data_arr[0] = up|flags|LCD_BACKLIGHT|LCD_ENABLE;
    data_arr[1] = up|flags|LCD_BACKLIGHT;
    data_arr[2] = lo|flags|LCD_BACKLIGHT|LCD_ENABLE;
    data_arr[3] = lo|flags|LCD_BACKLIGHT;

    HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR << 1, data_arr, sizeof(data_arr), 100);
    HAL_Delay(1);
}

/**
 * @brief Send command to LCD
 */
void LCD_SendCommand(uint8_t cmd) {
    LCD_Write(cmd, LCD_COMMAND);
}

/**
 * @brief Send data to LCD
 */
void LCD_SendData(uint8_t data) {
    LCD_Write(data, LCD_DATA);
}

/**
 * @brief Initialize LCD
 */
void LCD_Init(void) {
    HAL_Delay(50);  // Wait for LCD to power up

    // Initialize 4-bit mode
    LCD_SendCommand(0x33);
    LCD_SendCommand(0x32);
    LCD_SendCommand(0x28); // 2 line, 5x8 matrix
    LCD_SendCommand(0x0C); // Display ON, cursor OFF
    LCD_SendCommand(0x06); // Increment cursor
    LCD_SendCommand(0x01); // Clear display
    HAL_Delay(2);
}

/**
 * @brief Send string to LCD
 */
void LCD_SendString(char *str) {
    while(*str) LCD_SendData(*str++);
}

/**
 * @brief Set LCD cursor position
 */
void LCD_SetCursor(uint8_t row, uint8_t col) {
    uint8_t row_offsets[] = {0x00, 0x40};
    LCD_SendCommand(0x80 | (col + row_offsets[row]));
}

/**
 * @brief Clear LCD display
 */
void LCD_Clear(void) {
    LCD_SendCommand(0x01);
    HAL_Delay(2);
}

/**
 * @brief Update LCD display with current settings
 */
void UpdateDisplay(void) {
    // Calculate actual frequency
    //float frequencyRatio = adcFrequency / 1024.0f;
    //long int currentFreq = MIN_FREQUENCY +    (uint32_t)((MAX_FREQUENCY - MIN_FREQUENCY) * frequencyRatio);
    uint32_t wholePart = targetFreq / 1000;  //  whole part in kHz
    uint32_t fractionalPart = targetFreq % 1000; // fractional part

    // Update first line - Frequency
    LCD_SetCursor(0, 0);
    sprintf(lcdBuffer, "Freq:%3lu.%-3lukHz", wholePart,fractionalPart); //sprintf(lcdBuffer, "Freq: %lu Hz    ", currentFreq);
    //sprintf(lcdBuffer, "Freq: %lu Hz    ", currentFreq);
    //sprintf(lcdBuffer, "Freq: %lu Hz    ", targetFreq );
    LCD_SendString(lcdBuffer);

    // Update second line - Dead time
    LCD_SetCursor(1, 0);
    sprintf(lcdBuffer, "DTim: %lu       ", deadTime); //adcDeadTime
    //int kkk;
    //kkk=MeasureVdda();
    //sprintf(lcdBuffer, "zas.%i", kkk ); //supply
    LCD_SendString(lcdBuffer);
}

/**
 * @brief I2C1 Initialization
 */
static void MX_I2C1_Init(void) {
    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x10707DBC;  // 100kHz at 64MHz
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }
}

/* Update GPIO initialization to include I2C pins */
static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();  // Enable GPIOB clock for I2C

    // Previous PWM GPIO configuration
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // I2C GPIO Configuration
    // PB6 -> SCL, PB7 -> SDA
    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF6_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // Enable I2C1 clock
    __HAL_RCC_I2C1_CLK_ENABLE();
}

/* Update main function */
int main(void) {
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_TIM1_Init();
    MX_I2C1_Init();  // Initialize I2C
    HAL_ADCEx_Calibration_Start(&hadc1);
    // Initialize LCD
    LCD_Init();
    LCD_Clear();

    // Initial display message
    LCD_SetCursor(0, 0);
    LCD_SendString("PWM Controller");
    LCD_SetCursor(1, 0);
    LCD_SendString("Initializing...");
    HAL_Delay(1000);

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);

    while (1) {
        HAL_ADC_Start(&hadc1);

        if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
            adcDeadTime = HAL_ADC_GetValue(&hadc1);
        }

        if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
            adcFrequency = HAL_ADC_GetValue(&hadc1);
        }

        HAL_ADC_Stop(&hadc1);
        UpdatePWMParameters();
        UpdateDisplay();  // Update LCD display
        HAL_Delay(250);  // Reduced delay for more frequent display updates
    }
}



/**
 * @brief Updates PWM parameters based on ADC readings
 * @note Calculates and sets new frequency and dead time values based on potentiometer positions
 */
void UpdatePWMParameters(void)
{
    // Calculate frequency based on ADC reading
    // ADC reading is 0-4095, scale it to our frequency range
    targetFreq = MIN_FREQUENCY +
        ((MAX_FREQUENCY - MIN_FREQUENCY) * adcFrequency) / 4095;

    // Calculate required timer period for target frequency
    // Timer Period = Timer Clock / Target Frequency
    uint32_t timerPeriod = (SystemCoreClock / targetFreq);

    // Update timer period
    __HAL_TIM_SET_AUTORELOAD(&htim1, timerPeriod - 1);

    // Calculate duty cycle
    uint32_t dutyCycle = (timerPeriod * PWM_DUTY_CYCLE) / 1000; // PWM_DUTY_CYCLE is in thousandths (e.g., 499 for 49.9%)
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, dutyCycle);

    // Calculate dead time
    // Scale ADC reading (0-4095) to dead time range (MIN_DEAD_TIME to MAX_DEAD_TIME)
    deadTime = MIN_DEAD_TIME +  ((adcDeadTime * (MAX_DEAD_TIME - MIN_DEAD_TIME)) / 4095);   //uint32_t deadTime

    // Update dead time register
    // Keep existing settings (MOE, AOE) while updating DTG
    uint32_t bdtr = htim1.Instance->BDTR;
    bdtr &= ~TIM_BDTR_DTG; // Clear old dead time value
    bdtr |= (deadTime & TIM_BDTR_DTG); // Set new dead time value
    htim1.Instance->BDTR = bdtr;
}


static void MX_TIM1_Init(void)
{
  __HAL_RCC_TIM1_CLK_ENABLE();

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 999;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 500;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_ENABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;

  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

  __HAL_TIM_DISABLE_OCxPRELOAD(&htim1, TIM_CHANNEL_1);

  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_ENABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_ENABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 100;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;

  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }

  TIM1->CCER |= TIM_CCER_CC1E | TIM_CCER_CC1NE;

  sConfigOC.Pulse = 553;
  HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1);

  TIM1->CCMR1 |= (TIM_OCMODE_PWM1 << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE;

  HAL_TIM_MspPostInit(&htim1);
}


/**
 * @brief ADC1 Initialization
 * @note Configures ADC for reading two potentiometers (PA0 and PA1)
 */


static void MX_ADC1_Init(void) {
    ADC_ChannelConfTypeDef sConfig = {0};

    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc1.Init.ContinuousConvMode = ENABLE;
    hadc1.Init.NbrOfConversion = 2;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
     hadc1.Init.DMAContinuousRequests = DISABLE;
     hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
     hadc1.Init.SamplingTimeCommon1 = ADC_SAMPLETIME_39CYCLES_5;
     hadc1.Init.SamplingTimeCommon2 = ADC_SAMPLETIME_39CYCLES_5;
     hadc1.Init.OversamplingMode = DISABLE;
     hadc1.Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_HIGH;

    HAL_ADC_Init(&hadc1);

    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    sConfig.Channel = ADC_CHANNEL_1;
    sConfig.Rank = ADC_REGULAR_RANK_2;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
    RCC_OscInitStruct.PLL.PLLN = 16;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV4;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV4;

    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

void Error_Handler(void) {
    __disable_irq();
    while (1) {}
}
