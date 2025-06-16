#include "stm32f405xx.h" 

#include "cmn.h"  // Assumes delayms() is defined here 

#include <stdint.h> 

#include "lcd.h"  // Make sure this header exists and has lcd_init() and lcd_print() 

 

#include <stdio.h>  // for sprintf 

// === Pin Definitions === 

 

#define BIN1_PIN      3   // PA3 - Motor 2 

#define BIN2_PIN      2   // PA2 - Motor 2 

 

#define LDR_PIN       0   // PA0 - LDR input 

#define LED_PIN       6   // PA6 - LED output 

#define BUZZER_PIN    7   // PA7 - Buzzer output 

#define TEMP_ADC_CH   5   // PA5 = ADC1_IN5 

 

// === Global Variables === 

volatile uint16_t adc_value = 0; 

volatile float voltage = 0.0; 

volatile int temperature = 0.0; 

volatile float t1 = 0.0; 

#include "stm32f405xx.h" 

#include "cmn.h"  // Assumes delayms() is defined here 

#include <stdint.h> 

#include "lcd.h"  // Make sure this header exists and has lcd_init() and lcd_print() 

 

#include <stdio.h>  // for sprintf 

// === Pin Definitions === 

 

#define BIN1_PIN      3   // PA3 - Motor 2 

#define BIN2_PIN      2   // PA2 - Motor 2 

 

#define LDR_PIN       0   // PA0 - LDR input 

#define LED_PIN       6   // PA6 - LED output 

#define BUZZER_PIN    7   // PA7 - Buzzer output 

#define TEMP_ADC_CH   5   // PA5 = ADC1_IN5 

 

// === Global Variables === 

volatile uint16_t adc_value = 0; 

volatile float voltage = 0.0; 

volatile int temperature = 0.0; 

volatile float t1 = 0.0; 

 

void delay(void) { 

    for (volatile uint32_t i = 0; i < 100000; i++); 

} 

 

// === GPIO Initialization === 

void gpio_init(void) { 

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;  // Enable GPIOA clock 

 

 

 

    // --- Outputs: Motor2, LED, Buzzer --- 

    uint32_t pins = (1 << BIN1_PIN) | (1 << BIN2_PIN) | 

                    (1 << LED_PIN)  | (1 << BUZZER_PIN); 

    for (int i = 0; i < 8; i++) { 

        if (pins & (1 << i)) { 

            GPIOA->MODER &= ~(3 << (i * 2)); // Clear mode 

            GPIOA->MODER |=  (1 << (i * 2)); // Set as output 

        } 

    } 

} 

 

// === Motor 2 Control === 

void motor2_on(void) { 

    GPIOA->ODR |=  (1 << BIN1_PIN); 

    GPIOA->ODR &= ~(1 << BIN2_PIN); 

} 

void motor2_off(void) { 

    GPIOA->ODR &= ~(1 << BIN1_PIN); 

    GPIOA->ODR &= ~(1 << BIN2_PIN); 

} 

 

//// === Toggle Pin === 

//void toggle_pin(uint8_t pin) { 

//    GPIOA->ODR ^= (1 << pin); 

//} 

 

// === ADC Initialization === 

void adc_init(void) { 

    RCC->AHB1ENR |= (1 << 0);               // Enable GPIOA clock 

    GPIOA->MODER |= (3 << (5 * 2));         // PA5 -> Analog mode 

 

    RCC->APB2ENR |= (1 << 8);               // Enable ADC1 clock 

    ADC1->SQR3 = 5;                         // Channel 5 (PA5 = ADC1_IN5) 

 

    ADC1->CR1 |= (1 << 8);                  // Scan mode (not needed for 1 ch but ok) 

    ADC1->CR2 |= (1 << 1);                  // Continuous conversion 

    ADC1->CR2 |= (1 << 0);                  // Enable ADC 

} 

 

// === ADC Read === 

uint16_t adc_read(void) { 

    ADC1->CR2 |= (1 << 30);                 // Start conversion 

    while (!(ADC1->SR & (1 << 1)));         // Wait for end of conversion 

    return ADC1->DR; 

} 

 

// === Main === 

int main(void) { 

 

    gpio_init(); 

    adc_init(); 

 

    LcdInit(); 

 

    while (1) { 

        // --- Read temperature --- 

        adc_value = adc_read(); 

        voltage = (adc_value / 4095.0) * 3.3; 

        t1 = -50.0 * voltage; 

    temperature = t1 + 107.5; 

    char temp_str[17]; 

            sprintf(temp_str, "Temp: %d C", temperature); 

            lprint(0x80, temp_str);  // Display on first row 

 

 

 

        // --- Temperature-based control for Motor 2 with hysteresis --- 

        if (temperature > 27.0) { 

            motor2_on(); 

 

        } else 

        { 

            motor2_off(); 

 

        } 

 

        delayms(1000);  // Make sure you're using a real delay function here 

    } 

} 

 

void delay(void) { 

    for (volatile uint32_t i = 0; i < 100000; i++); 

} 

 

// === GPIO Initialization === 

void gpio_init(void) { 

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;  // Enable GPIOA clock 

 

 

 

    // --- Outputs: Motor2, LED, Buzzer --- 

    uint32_t pins = (1 << BIN1_PIN) | (1 << BIN2_PIN) | 

                    (1 << LED_PIN)  | (1 << BUZZER_PIN); 

    for (int i = 0; i < 8; i++) { 

        if (pins & (1 << i)) { 

            GPIOA->MODER &= ~(3 << (i * 2)); // Clear mode 

            GPIOA->MODER |=  (1 << (i * 2)); // Set as output 

        } 

    } 

} 

 

// === Motor 2 Control === 

void motor2_on(void) { 

    GPIOA->ODR |=  (1 << BIN1_PIN); 

    GPIOA->ODR &= ~(1 << BIN2_PIN); 

} 

void motor2_off(void) { 

    GPIOA->ODR &= ~(1 << BIN1_PIN); 

    GPIOA->ODR &= ~(1 << BIN2_PIN); 

} 

 

//// === Toggle Pin === 

//void toggle_pin(uint8_t pin) { 

//    GPIOA->ODR ^= (1 << pin); 

//} 

 

// === ADC Initialization === 

void adc_init(void) { 

    RCC->AHB1ENR |= (1 << 0);               // Enable GPIOA clock 

    GPIOA->MODER |= (3 << (5 * 2));         // PA5 -> Analog mode 

 

    RCC->APB2ENR |= (1 << 8);               // Enable ADC1 clock 

    ADC1->SQR3 = 5;                         // Channel 5 (PA5 = ADC1_IN5) 

 

    ADC1->CR1 |= (1 << 8);                  // Scan mode (not needed for 1 ch but ok) 

    ADC1->CR2 |= (1 << 1);                  // Continuous conversion 

    ADC1->CR2 |= (1 << 0);                  // Enable ADC 

} 

 

// === ADC Read === 

uint16_t adc_read(void) { 

    ADC1->CR2 |= (1 << 30);                 // Start conversion 

    while (!(ADC1->SR & (1 << 1)));         // Wait for end of conversion 

    return ADC1->DR; 

} 

 

// === Main === 

int main(void) { 

 

    gpio_init(); 

    adc_init(); 

 

    LcdInit(); 

 

    while (1) { 

        // --- Read temperature --- 

        adc_value = adc_read(); 

        voltage = (adc_value / 4095.0) * 3.3; 

        t1 = -50.0 * voltage; 

    temperature = t1 + 107.5; 

    char temp_str[17]; 

            sprintf(temp_str, "Temp: %d C", temperature); 

            lprint(0x80, temp_str);  // Display on first row 

 

 

 

        // --- Temperature-based control for Motor 2 with hysteresis --- 

        if (temperature > 27.0) { 

            motor2_on(); 

 

        } else 

        { 

            motor2_off(); 

 

        } 

 

        delayms(1000);  // Make sure you're using a real delay function here 

    } 

} 