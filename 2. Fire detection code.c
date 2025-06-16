#include "stm32f405xx.h" 

 

#include "cmn.h"  // Assumes delayms() is defined here 

 

// Pin Definitions 

 

#define AIN1_PIN    12   // PA12 - Motor control 

 

#define AIN2_PIN    4   // PA4 - Motor control 

 

#define LDR_PIN     11   // PA11 - LDR input 

 

#define LED_PIN     6   // PA6 - LED output 

 

#define BUZZER_PIN  7   // PA7 - Buzzer output 

 

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

 

// Toggle GPIO pin 

 

void toggle_pin(uint8_t pin) { 

 

    GPIOA->ODR ^= (1 << pin);  // XOR toggles bit 

 

} 

 

int main(void) { 

 

    gpio_init();  // Initialize all GPIOs 

 

    while (1) { 

 

        if (GPIOA->IDR & (1 << LDR_PIN)) { 

 

            // Light detected → turn on motor 

 

            motor_on(); 

 

            // Toggle LED and Buzzer 

 

            toggle_pin(LED_PIN); 

 

            toggle_pin(BUZZER_PIN); 

 

        } else { 

 

            // No light → turn off motor, LED, buzzer 

 

            motor_off(); 

 

            GPIOA->ODR &= ~(1 << LED_PIN); 

 

            GPIOA->ODR &= ~(1 << BUZZER_PIN); 

 

        } 

 

        delayms(100);  // Poll every 200 ms 

 

    } 

 

} 