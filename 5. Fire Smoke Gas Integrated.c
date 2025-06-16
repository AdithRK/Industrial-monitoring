#include "stm32f405xx.h" 

 

#include "cmn.h"  // Assumes delayms() is defined here 

#include <stdint.h> 

#include "lcd.h"  // Make sure this header exists and has lcd_init() and lcd_print() 

 

#include <stdio.h>  // for sprintf 

// Pin Definitions 

 

#define AIN1_PIN    12   // PA12 - Motor control 

 

#define AIN2_PIN    4   // PA4 - Motor control 

#define BIN1_PIN      3   // PA3 - Motor 2 

#define BIN2_PIN      2   // PA2 - Motor 2 

#define TEMP_ADC_CH   5   // PA5 = ADC1_IN5 

#define LDR_PIN     11   // PA11 - LDR input 

 

#define LED_PIN     6   // PA6 - LED output 

 

#define BUZZER_PIN  7   // PA7 - Buzzer output 

 

// === Global Variables === 

volatile uint16_t adc_value = 0; 

volatile float voltage = 0.0; 

volatile int temperature = 0.0; 

volatile float t1 = 0.0; 

uint16_t result=0; 

// Function prototypes 

 

extern void delayms(uint32_t ms); 

 

// GPIO Initialization 

 

void gpio_init(void) { 

 

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;  // Enable GPIOA clock 

 

    // PA11 as Input (LDR) 

 

    GPIOA->MODER &= ~(3 << (LDR_PIN * 2)); 

 

    GPIOA->PUPDR &= ~(3 << (LDR_PIN * 2));  // No pull 

 

    // PA4, PA5 as Output (Motor) 

 

    GPIOA->MODER &= ~((3 << (AIN1_PIN * 2)) | (3 << (AIN2_PIN * 2))); 

 

    GPIOA->MODER |= (1 << (AIN1_PIN * 2)) | (1 << (AIN2_PIN * 2)); 

 

    GPIOA->OTYPER &= ~((1 << AIN1_PIN) | (1 << AIN2_PIN)); 

 

    GPIOA->OSPEEDR |= (3 << (AIN1_PIN * 2)) | (3 << (AIN2_PIN * 2)); 

 

    // PA2, PA3 as Output (Motor) 

 

        GPIOA->MODER &= ~((3 << (BIN1_PIN * 2)) | (3 << (BIN2_PIN * 2))); 

 

        GPIOA->MODER |= (1 << (BIN1_PIN * 2)) | (1 << (BIN2_PIN * 2)); 

 

        GPIOA->OTYPER &= ~((1 << BIN1_PIN) | (1 << BIN2_PIN)); 

 

        GPIOA->OSPEEDR |= (3 << (BIN1_PIN * 2)) | (3 << (BIN2_PIN * 2)); 

 

 

    // PA6 (LED) and PA7 (Buzzer) as Output 

 

    GPIOA->MODER &= ~((3 << (LED_PIN * 2)) | (3 << (BUZZER_PIN * 2))); 

 

    GPIOA->MODER |= (1 << (LED_PIN * 2)) | (1 << (BUZZER_PIN * 2)); 

 

    GPIOA->OTYPER &= ~((1 << LED_PIN) | (1 << BUZZER_PIN)); 

 

    GPIOA->OSPEEDR |= (3 << (LED_PIN * 2)) | (3 << (BUZZER_PIN * 2)); 

 

 

    } 

 

 

 

// Motor Control 

 

void motor_on(void) { 

 

    GPIOA->ODR |= (1 << AIN1_PIN); 

 

    GPIOA->ODR &= ~(1 << AIN2_PIN); 

 

} 

 

void motor_off(void) { 

 

    GPIOA->ODR &= ~(1 << AIN1_PIN); 

 

    GPIOA->ODR &= ~(1 << AIN2_PIN); 

 

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

 

 

 

// Toggle GPIO pin 

 

void toggle_pin(uint8_t pin) { 

 

    GPIOA->ODR |= (1 << pin);  // XOR toggles bit 

 

} 

 

 

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

 

 

void adc2_init(void) 

{ 

    RCC->AHB1ENR |= (1 << 2);      // Enable GPIOC clock 

 

    // Set PC2 to analog mode 

    GPIOC->MODER |= (3 << (2 * 2));  // PC2 = analog mode (MODER2[1:0] = 11) 

 

    RCC->APB2ENR |= (1 << 9);      // Enable ADC2 clock 

 

    ADC2->SQR3 = 12;               // Select channel 12 (PC2) as first in regular sequence 

    ADC2->CR1 = 0;                 // Disable scan mode (only 1 channel needed) 

    ADC2->CR2 |= (1 << 1);         // Enable continuous conversion 

    ADC2->CR2 |= (1 << 0);         // Enable ADC2 

} 

 

// === ADC2 Read Function === 

uint16_t adc2_read(void) 

{ 

    ADC2->CR2 |= (1 << 30);                  // Start conversion 

    while (!(ADC2->SR & (1 << 1)));          // Wait for EOC 

    return ADC2->DR;                         // Return ADC result 

} 

 

// LED Initialization for PC6, PB13, PB14, PB15 

 

void led_init(void) 

 

{ 

 

    RCC->AHB1ENR |= (1 << 2);  // Enable GPIO Clock for PortC 

 

 

 

    // Set PC0–PC5 as output 

 

        for (int i = 0; i <= 5; i++) 

 

            GPIOC->MODER |= (1 << (i * 2)); 

 

 

} 

 

// Turn off all LEDs 

 

void led_control(uint16_t adc_value) 

{ 

    // Turn OFF all LEDs 

    GPIOC->ODR &= ~0x3F; // Clear PC0–PC5 (bits 0–5) 

 

 

 

 

    int index = adc_value / 410;  // Get range 0–9 

 

    switch (index) 

    { 

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

 

 

int main(void) { 

 

    gpio_init();  // Initialize all GPIOs 

    adc_init(); 

    adc2_init();  // Initialize ADC for potentiometer reading 

        char buffer[17]; 

        led_init();  // Initialize GPIO for LEDs 

 

        LcdInit(); 

 

    while (1) { 

    	result = adc2_read(); // Read ADC value from potentiometer 

    	        int gas_percent = (result * 100) / 4095; 

    	// --- Read temperature --- 

    	        adc_value = adc_read(); 

    	        voltage = (adc_value / 4095.0) * 3.3; 

    	        t1 = -50.0 * voltage; 

    	    temperature = t1 + 104.5; 

    	    char temp_str[17]; 

    	            sprintf(temp_str, "Temp: %d C", temperature); 

    	            lprint(0x80, temp_str);  // Display on first row 

 

 

 

    	        // --- Temperature-based control for Motor 2 with hysteresis --- 

    	        if (temperature > 27.0) { 

    	            motor2_on(); 

    	            lprint(0xc0, "                     "); 

    	            lprint(0xc0, "smoke detected"); 

 

    	        } else 

    	        { 

    	        	lprint(0xc0, "                     "); 

    	            motor2_off(); 

 

    	        } 

 

 

        if (GPIOA->IDR & (1 << LDR_PIN)) { 

 

            // Light detected → turn on motor 

 

            motor_on(); 

 

            // Toggle LED and Buzzer 

 

            toggle_pin(LED_PIN); 

 

            toggle_pin(BUZZER_PIN); 

            lprint(0xc0, "                     "); 

            lprint(0xc0, "fire detected"); 

 

        } else { 

 

            // No light → turn off motor, LED, buzzer 

 

            motor_off(); 

            lprint(0xc0, "                     "); 

 

            GPIOA->ODR &= ~(1 << LED_PIN); 

 

            GPIOA->ODR &= ~(1 << BUZZER_PIN); 

 

        } 

 

        if(gas_percent>20){ 

                sprintf(buffer, "Gas: %3d%%       ", gas_percent); 

                lprint(0xc0, buffer); 

 

                } 

                else 

                { 

                	lprint(0xc0, "                     "); 

                } 

                led_control(result); // Control LED based on ADC value 

 

                for (uint32_t j = 0; j < 20000; j++) {} // Small delay to make the change visible 

 

 

        delayms(100);  // Poll every 200 ms 

 

    } 

 

} 