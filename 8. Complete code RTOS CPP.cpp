 

/* USER CODE BEGIN Header */ 

/** 

  ****************************************************************************** 

  * @file           : main.c 

  * @brief          : Main program body 

  ****************************************************************************** 

  * @attention 

  * 

  * Copyright (c) 2025 STMicroelectronics. 

  * All rights reserved. 

  * 

  * This software is licensed under terms that can be found in the LICENSE file 

  * in the root directory of this software component. 

  * If no LICENSE file comes with this software, it is provided AS-IS. 

  * 

  ****************************************************************************** 

  */ 

/* USER CODE END Header */ 

/* Includes ------------------------------------------------------------------*/ 

#include <cstring> 

#include <cstdio> 

#include "main.h" 

#include "gpio.h" 

 

extern "C" { 

#include "stm32f405xx.h" 

#include "lcd.h" 

#include "cmn.h" 

#include "FreeRTOS.h" 

#include "task.h" 

#include "semphr.h" 

#define AIN1_PIN    12  // PA12 - Motor 1 

#define AIN2_PIN    4   // PA4  - Motor 1 

#define BIN1_PIN    3   // PA3 - Motor 2 

#define BIN2_PIN    2   // PA2 - Motor 2 

#define TEMP_ADC_CH 5   // PA5 = ADC1_IN5 

#define LDR_PIN     11  // PA11 - LDR input 

#define LED_PIN     6   // PA6 - LED output 

#define LED1_PIN    6   // PC6 

#define LED2_PIN    7   // PC7 

#define BUZZER_PIN  7   // PA7 - Buzzer 

void toggle_pin(GPIO_TypeDef* port, uint8_t pin) { 

    port->ODR ^= (1 << pin); 

} 

} 

 

 

 

SemaphoreHandle_t passkey_semaphore; 

volatile uint16_t result=0; 

 

volatile uint16_t adc2_val = 0; 

volatile int temperature = 0.0; 

volatile float voltage = 0.0; 

volatile float t1 = 0.0; 

volatile int gas_percent = 0.0; 

 

class Keypad { 

public: 

    static const char keymap[4][4]; 

    void init() { 

        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; 

        for (int i = 0; i < 4; i++) { 

            GPIOB->MODER &= ~(3 << (i * 2)); 

            GPIOB->MODER |= (1 << (i * 2)); 

            GPIOB->ODR |= (1 << i); 

        } 

        for (int i = 4; i < 8; i++) { 

            GPIOB->MODER &= ~(3 << (i * 2)); 

            GPIOB->PUPDR &= ~(3 << (i * 2)); 

            GPIOB->PUPDR |= (1 << (i * 2)); 

        } 

 

 

 

 

    } 

    char scan() { 

        for (int row = 0; row < 4; row++) { 

            GPIOB->ODR |= 0x0F; 

            GPIOB->ODR &= ~(1 << row); 

            vTaskDelay(pdMS_TO_TICKS(2)); 

            for (int col = 0; col < 4; col++) { 

                if ((GPIOB->IDR & (1 << (col + 4))) == 0) { 

                    vTaskDelay(pdMS_TO_TICKS(20)); 

                    while ((GPIOB->IDR & (1 << (col + 4))) == 0); 

                    return keymap[row][col]; 

                } 

            } 

        } 

        return 0; 

    } 

}; 

 

const char Keypad::keymap[4][4] = { 

    {'1','2','3','A'}, 

    {'4','5','6','B'}, 

    {'7','8','9','C'}, 

    {'*','0','#','D'} 

}; 

 

class MotorController { 

public: 

    void init() { 

        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOCEN; 

        GPIOA->MODER &= ~(3 << (LDR_PIN * 2)); 

        GPIOA->PUPDR &= ~(3 << (LDR_PIN * 2)); 

 

        GPIOA->MODER &= ~((3 << (AIN1_PIN * 2)) | (3 << (AIN2_PIN * 2))); 

        GPIOA->MODER |= (1 << (AIN1_PIN * 2)) | (1 << (AIN2_PIN * 2)); 

 

        GPIOA->MODER &= ~((3 << (BIN1_PIN * 2)) | (3 << (BIN2_PIN * 2))); 

        GPIOA->MODER |= (1 << (BIN1_PIN * 2)) | (1 << (BIN2_PIN * 2)); 

 

        GPIOA->MODER &= ~((3 << (LED_PIN * 2)) | (3 << (BUZZER_PIN * 2))); 

        GPIOA->MODER |= (1 << (LED_PIN * 2)) | (1 << (BUZZER_PIN * 2)); 

 

        GPIOC->MODER &= ~((3 << (2 * LED1_PIN)) | (3U << (2 * LED2_PIN))); 

        GPIOC->MODER |= (1 << (2 * LED1_PIN)) | (1U << (2 * LED2_PIN)); 

 

    } 

 

    void motor1_on()  { GPIOA->ODR |= (1 << 12); GPIOA->ODR &= ~(1 << 4); } 

    void motor1_off() { GPIOA->ODR &= ~((1 << 12) | (1 << 4)); } 

    void motor2_on()  { GPIOA->ODR |= (1 << 3); GPIOA->ODR &= ~(1 << 2); } 

    void motor2_off() { GPIOA->ODR &= ~((1 << 3) | (1 << 2)); } 

}; 

 

class ADCReader { 

public: 

    void init_ADC1() { 

        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; 

        GPIOA->MODER |= (3 << (5 * 2)); 

        RCC->APB2ENR |= RCC_APB2ENR_ADC1EN; 

        ADC1->SQR3 = 5; 

        ADC1->CR2 |= (1 << 1) | (1 << 0); 

    } 

    void init_ADC2() { 

        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; 

        GPIOC->MODER |= (3 << (2 * 2)); 

        RCC->APB2ENR |= RCC_APB2ENR_ADC2EN; 

        ADC2->SQR3 = 12; 

        ADC2->CR2 |= (1 << 1) | (1 << 0); 

    } 

    uint16_t read_ADC1() { 

        ADC1->CR2 |= (1 << 30); 

        while (!(ADC1->SR & (1 << 1))); 

        return ADC1->DR; 

    } 

    uint16_t read_ADC2() { 

        ADC2->CR2 |= (1 << 30); 

        while (!(ADC2->SR & (1 << 1))); 

        return ADC2->DR; 

    } 

}; 

 

class LEDBarGraph { 

public: 

    void init() { 

        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; 

        for (int i = 0; i <= 5; ++i) { 

            GPIOC->MODER |= (1 << (i * 2)); 

        } 

    } 

    void display(uint16_t adc_value) { 

        GPIOC->ODR &= ~0x3F; 

        int level = adc_value / 410; 

        switch (level) { 

            case 9: GPIOC->ODR |= (1 << 0); 

            case 8: GPIOC->ODR |= (1 << 0); 

            case 7: GPIOC->ODR |= (1 << 1); 

            case 6: GPIOC->ODR |= (1 << 1); 

            case 5: GPIOC->ODR |= (1 << 3); 

            case 4: GPIOC->ODR |= (1 << 3); 

            case 3: GPIOC->ODR |= (1 << 4); 

            case 2: GPIOC->ODR |= (1 << 4); 

            case 1: GPIOC->ODR |= (1 << 5); 

            case 0: GPIOC->ODR |= (1 << 5); 

            default: break; 

        } 

    } 

}; 

 

class SecuritySystem { 

    char passkey[5] = "1234"; 

    char entered[5]; 

    int key_index = 0; 

    Keypad& keypad; 

public: 

    void init() { 

        // Enable GPIOB and GPIOA clocks 

        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN; 

 

        // Configure PB8 as output (correct passkey LED) 

        GPIOB->MODER &= ~(3 << (2 * 8));       // Clear mode bits 

        GPIOB->MODER |=  (1 << (2 * 8));       // Output mode 

        GPIOB->OTYPER &= ~(1 << 8);            // Push-pull 

        GPIOB->OSPEEDR |= (3 << (2 * 8));      // High speed 

        GPIOB->PUPDR &= ~(3 << (2 * 8));       // No pull-up/pull-down 

 

        // Configure PA15 as output (wrong passkey LED) 

        GPIOC->MODER &= ~(3 << (2 * 10));      // Clear mode bits 

        GPIOC->MODER |=  (1 << (2 * 10));      // Output mode 

 

 

        // Turn off both LEDs initially 

        GPIOB->ODR &= ~(1 << 8); 

 

    } 

 

 

    SecuritySystem(Keypad& kp) : keypad(kp) {} 

    void wait_for_passkey() { 

        keypad.init(); 

        lprint(0x80, (char*)"Welcome"); 

        while (true) { 

            char key = keypad.scan(); 

            if (key) { 

                if (key == '*') { 

                    key_index = 0; 

                    memset(entered, 0, sizeof(entered)); 

                    lprint(0x80, (char*)"Input Your Passkey"); 

                    lprint(0xC0, (char*)"                "); 

                } else if (key == '#' && key_index > 0) { 

                    entered[key_index] = '\0'; 

                    if (strcmp(entered, passkey) == 0) { 

                        lprint(0x80, (char*)"Please Come In   "); 

                        GPIOB->ODR |= (1 << 8);     // Correct passkey LED ON 

                        GPIOC->ODR &= ~(1 << 10);   // Wrong passkey LED OFF 

                        vTaskDelay(pdMS_TO_TICKS(500)); 

                        xSemaphoreGive(passkey_semaphore); 

                        break; 

                    } else { 

                        lprint(0x80, (char*)"Wrong Passkey!   "); 

                        lprint(0xC0, (char*)"Try Again!       "); 

                        GPIOC->ODR |= (1 << 10);    // Wrong passkey LED ON 

                        GPIOB->ODR &= ~(1 << 8);    // Correct passkey LED OFF 

                        vTaskDelay(pdMS_TO_TICKS(500)); 

                        key_index = 0; 

                        LcdFxn(0,0x01); 

                        lprint(0x80, (char*)"press *          "); 

                    } 

                } else if (key_index < 4) { 

                    entered[key_index++] = key; 

                    char buf[17] = "Entered: "; 

                    strncpy(&buf[9], entered, key_index); 

                    buf[9 + key_index] = '\0'; 

                    lprint(0xC0, buf); 

                } 

            } 

        } 

    } 

}; 

 

class MainController { 

public: 

    Keypad keypad; 

    MotorController motors; 

    ADCReader adc; 

    LEDBarGraph leds; 

    SecuritySystem security; 

 

    MainController() : security(keypad) {} 

 

    void run() { 

        motors.init(); 

        adc.init_ADC1(); 

        adc.init_ADC2(); 

        leds.init(); 

        char buffer[20]; 

        LcdFxn(0,0x01); 

        while (true) { 

            uint16_t adc1_val = adc.read_ADC1(); 

            adc2_val = adc.read_ADC2(); 

            voltage = (adc1_val / 4095.0) * 3.3; 

            t1 = -50.0 * voltage; 

            temperature = t1 + 104.5; 

            gas_percent = (adc2_val * 100) / 4095; 

 

            std::sprintf(buffer, "Temp: %d C    ", temperature); 

            lprint(0x80, buffer); 

            if (temperature > 26.0 && (GPIOA->IDR & (1 << LDR_PIN))) 

                    { 

            	        motors.motor2_on(); 

                        lprint(0xC0, (char*)"Smoke Detected         "); 

                        toggle_pin(GPIOA, BUZZER_PIN); 

                        GPIOC->ODR |= (1 << LED1_PIN); 

                        motors.motor1_off(); 

                        GPIOA->ODR &= ~(1 << LED_PIN); 

                    } 

                    else if (temperature < 27.0 && !(GPIOA->IDR & (1 << LDR_PIN))) 

                    { 

                    	motors.motor2_off(); 

                        lprint(0xC0, (char*)" Fire Detected                     "); 

                        toggle_pin(GPIOA, BUZZER_PIN); 

                        GPIOC->ODR &= ~(1 << LED1_PIN) ; 

                        motors.motor1_on(); 

                        toggle_pin(GPIOA,LED_PIN); 

                    } 

                    else if (temperature > 27.0 && !(GPIOA->IDR & (1 << LDR_PIN))) 

                    { 

                    	motors.motor2_on(); 

                    	toggle_pin(GPIOA, BUZZER_PIN); 

            			GPIOC->ODR |= (1 << LED1_PIN) ; 

            			motors.motor1_on(); 

            			toggle_pin(GPIOA,LED_PIN); 

                        lprint(0xC0, (char*)"Fire & Smoke          "); 

                    } 

                    else 

                    { 

                    	motors.motor2_off(); 

            			lprint(0xC0, (char*)"                      "); 

            			GPIOA->ODR &= ~(1 << BUZZER_PIN); 

            			GPIOC->ODR &= ~(1 << LED1_PIN) ; 

            			motors.motor1_off(); 

            			GPIOA->ODR &= ~(1 << LED_PIN); 

                    } 

                    if (gas_percent > 20) { 

                            std::sprintf(buffer, "Gas: %3d%%       ", gas_percent); 

                            lprint(0xC0, buffer); 

                        } 

 

                        if (temperature < 26.0 && (GPIOA->IDR & (1 << LDR_PIN)) && gas_percent < 20) { 

                            GPIOC->ODR |= (1 << LED2_PIN); 

                            lprint(0xC0, (char*)"safe                       "); 

                        } else { 

                            GPIOC->ODR &= ~(1 << LED2_PIN); 

                        } 

 

                        leds.display(adc2_val); 

                        vTaskDelay(pdMS_TO_TICKS(100)); 

 

        } 

    } 

}; 

 

MainController controller; 

void PasskeyTask(void* params) { 

    controller.security.init();           // <-- Add this 

    controller.security.wait_for_passkey(); 

    vTaskDelete(NULL); 

} 

 

void ControllerTask(void* params) { 

    xSemaphoreTake(passkey_semaphore, portMAX_DELAY); 

    controller.run(); 

} 

 

 

 

/* Private includes ----------------------------------------------------------*/ 

/* USER CODE BEGIN Includes */ 

 

/* USER CODE END Includes */ 

 

/* Private typedef -----------------------------------------------------------*/ 

/* USER CODE BEGIN PTD */ 

 

/* USER CODE END PTD */ 

 

/* Private define ------------------------------------------------------------*/ 

/* USER CODE BEGIN PD */ 

/* USER CODE END PD */ 

 

/* Private macro -------------------------------------------------------------*/ 

/* USER CODE BEGIN PM */ 

 

/* USER CODE END PM */ 

 

/* Private variables ---------------------------------------------------------*/ 

 

/* USER CODE BEGIN PV */ 

 

/* USER CODE END PV */ 

 

/* Private function prototypes -----------------------------------------------*/ 

void SystemClock_Config(void); 

/* USER CODE BEGIN PFP */ 

 

/* USER CODE END PFP */ 

 

/* Private user code ---------------------------------------------------------*/ 

/* USER CODE BEGIN 0 */ 

 

/* USER CODE END 0 */ 

 

/** 

  * @brief  The application entry point. 

  * @retval int 

  */ 

int main(void) 

{ 

  /* USER CODE BEGIN 1 */ 

 

  /* USER CODE END 1 */ 

 

  /* MCU Configuration--------------------------------------------------------*/ 

 

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */ 

  HAL_Init(); 

 

  /* USER CODE BEGIN Init */ 

 

  /* USER CODE END Init */ 

 

  /* Configure the system clock */ 

  SystemClock_Config(); 

 

  /* USER CODE BEGIN SysInit */ 

 

  /* USER CODE END SysInit */ 

 

  /* Initialize all configured peripherals */ 

  MX_GPIO_Init(); 

  /* USER CODE BEGIN 2 */ 

  passkey_semaphore = xSemaphoreCreateBinary(); 

      LcdInit(); 

      xTaskCreate(PasskeyTask, "Passkey", 512, NULL, 2, NULL); 

      xTaskCreate(ControllerTask, "Controller", 1024, NULL, 1, NULL); 

      vTaskStartScheduler(); 

  /* USER CODE END 2 */ 

 

  /* Infinite loop */ 

  /* USER CODE BEGIN WHILE */ 

  while (1) 

  { 

    /* USER CODE END WHILE */ 

 

    /* USER CODE BEGIN 3 */ 

  } 

  /* USER CODE END 3 */ 

} 

 

/** 

  * @brief System Clock Configuration 

  * @retval None 

  */ 

void SystemClock_Config(void) 

{ 

  RCC_OscInitTypeDef RCC_OscInitStruct = {0}; 

  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0}; 

 

  /** Configure the main internal regulator output voltage 

  */ 

  __HAL_RCC_PWR_CLK_ENABLE(); 

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1); 

 

  /** Initializes the RCC Oscillators according to the specified parameters 

  * in the RCC_OscInitTypeDef structure. 

  */ 

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI; 

  RCC_OscInitStruct.HSIState = RCC_HSI_ON; 

  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT; 

  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON; 

  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI; 

  RCC_OscInitStruct.PLL.PLLM = 16; 

  RCC_OscInitStruct.PLL.PLLN = 192; 

  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2; 

  RCC_OscInitStruct.PLL.PLLQ = 4; 

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) 

  { 

    Error_Handler(); 

  } 

 

  /** Initializes the CPU, AHB and APB buses clocks 

  */ 

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK 

                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2; 

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK; 

  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1; 

  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4; 

  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2; 

 

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK) 

  { 

    Error_Handler(); 

  } 

} 

 

/* USER CODE BEGIN 4 */ 

 

/* USER CODE END 4 */ 

 

/** 

  * @brief  Period elapsed callback in non blocking mode 

  * @note   This function is called  when TIM3 interrupt took place, inside 

  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment 

  * a global variable "uwTick" used as application time base. 

  * @param  htim : TIM handle 

  * @retval None 

  */ 

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) 

{ 

  /* USER CODE BEGIN Callback 0 */ 

 

  /* USER CODE END Callback 0 */ 

  if (htim->Instance == TIM3) { 

    HAL_IncTick(); 

  } 

  /* USER CODE BEGIN Callback 1 */ 

 

  /* USER CODE END Callback 1 */ 

} 

 

/** 

  * @brief  This function is executed in case of error occurrence. 

  * @retval None 

  */ 

void Error_Handler(void) 

{ 

  /* USER CODE BEGIN Error_Handler_Debug */ 

  /* User can add his own implementation to report the HAL error return state */ 

  __disable_irq(); 

  while (1) 

  { 

  } 

  /* USER CODE END Error_Handler_Debug */ 

} 

 

#ifdef  USE_FULL_ASSERT 

/** 

  * @brief  Reports the name of the source file and the source line number 

  *         where the assert_param error has occurred. 

  * @param  file: pointer to the source file name 

  * @param  line: assert_param error line source number 

  * @retval None 

  */ 

void assert_failed(uint8_t *file, uint32_t line) 

{ 

  /* USER CODE BEGIN 6 */ 

  /* User can add his own implementation to report the file name and line number, 

     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */ 

  /* USER CODE END 6 */ 

} 

#endif /* USE_FULL_ASSERT */ 

 