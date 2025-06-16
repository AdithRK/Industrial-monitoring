#include "stm32f405xx.h" 

#include <stdint.h> 

 

volatile uint16_t adc_value = 0; 

volatile float voltage = 0.0; 

volatile int temperature = 0.0; 

volatile float t1 = 0.0; 

 

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

 

// === Simple Delay === 

void delay(void) { 

    for (volatile uint32_t i = 0; i < 100000; i++); 

} 

 

// === Main === 

int main(void) { 

    adc_init(); 

 

    while (1) { 

        adc_value = adc_read(); 

        voltage = (adc_value / 4095.0) * 3.3; 

        t1 = -50.0 * voltage; 

        temperature = t1 + 107.5; 

 

        delay();  // Allows time for live expression updates 

    } 

} 