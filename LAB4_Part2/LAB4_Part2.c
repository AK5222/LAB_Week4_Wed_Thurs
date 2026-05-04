#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/adc.h"

// --- Pin Definitions ---
// 4 Local Pico LEDs
const uint PICO_LED_PINS[4] = {2, 3, 4, 5}; // GPIOs

// Inputs
const uint DIP_SWITCH_PIN = 6; 
const uint POT_ADC_INDEX = 0;   // ADC 0 is connected to GPIO 26 on the Pico

// SPI Pins
const uint SPI_SCK_PIN = 18; // SCK (Clock)
const uint SPI_TX_PIN = 19; // MOSI/Data. Won't use it here
const uint SPI_CS_PIN = 17; // We will handle CS manually


int main()
{
    stdio_init_all();

    // Initialize Local LEDs
    for (int i = 0; i < 4; i++) {
        gpio_init(PICO_LED_PINS[i]);
        gpio_set_dir(PICO_LED_PINS[i], GPIO_OUT);
    }

    // Initialize DIP Switch
    gpio_init(DIP_SWITCH_PIN);
    gpio_set_dir(DIP_SWITCH_PIN, GPIO_IN);
    gpio_pull_up(DIP_SWITCH_PIN); // Use internal pull-up (1 = off, 0 = on)

    // Initialize ADC for Potentiometer
    adc_init();
    adc_gpio_init(26);
    adc_select_input(POT_ADC_INDEX);


    // 4. Initialize SPI (Master)
    // We are using spi0. set communication speed to 1MHz (1000 * 1000).
    spi_init(spi0, 1000 * 1000);
    
    // Assign the SCK and TX pins to the SPI hardware
    gpio_set_function(SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_TX_PIN, GPIO_FUNC_SPI);
    
    // handle the CS pin manually on the Master Pico. So we treat it like a normal GPIO pin instead of an SPI pin
    gpio_init(SPI_CS_PIN);
    gpio_set_dir(SPI_CS_PIN, GPIO_OUT);
    
    // Set CS high (1) to start. In Mode 0 SPI, CS stays high when we arent talking,
    // pull it low (0) right before we start sending data.
    gpio_put(SPI_CS_PIN, 1);


    int frame_step = 0;              // Keeps track of where we are in the animation
    uint8_t current_pattern_val = 0; // The 8-bit pattern (4 bits for Pico, 4 bits for FPGA)

    // Keep the program running forever
    while (true) {
        // Read Speed from Potentiometer
        uint16_t adc_raw = adc_read(); // The ADC returns a # 0-4095.
        int speed_step = (adc_raw * 10) / 4096; // Maps to a step from 0-9
        int delay_ms = 100 + (speed_step * 100); // delay between 100ms and 1000ms.

        // Read Pattern Selection from DIP Switch
        // The pull-up makes the pin read 1 normally, and 0 when the switch is flipped on.
        bool switch_is_on = !gpio_get(DIP_SWITCH_PIN);

        // Compute the 8-bit LED Pattern
        if (switch_is_on) {
            // Pattern 1: Alternating (Odds, then Evens)
            if (frame_step % 2 == 0) {
                current_pattern_val = 0x55; // 01010101 in binary (Odds on)
            } else {
                current_pattern_val = 0xAA; // 10101010 in binary (Evens on)
            }
            
            frame_step++;
        } 
        else {
            // Pattern 0: Fill up
            if (frame_step == 0) {
                current_pattern_val = 0x01; // First LED on
            } else if (frame_step <= 7) {
                // Add the next LED to the line
                // Shift left and add a 1 (e.g., 001 -> 011 -> 111)
                current_pattern_val = (current_pattern_val << 1) | 0x01;
            } else {
                current_pattern_val = 0x00; // All LEDs off
            }

            frame_step++;
            if (frame_step > 8) { // Reset after all 8 turn on and then off
                frame_step = 0;
            }
        }

        // Update the Pico's Local 4 LEDs
        // We only care about the bottom 4 bits (bits 0, 1, 2, 3) for the Pico
        for (int i = 0; i < 4; i++) {
            // Read if the specific bit is a 1 or a 0
            bool is_led_on = (current_pattern_val & (1 << i)) != 0;
            gpio_put(PICO_LED_PINS[i], is_led_on);
        }

        
        // Transmit to FPGA over SPI
        // The Pico uses the bottom 4 bits. 
        // shift the pattern right by 4 to grab the top 4 bits for the FPGA.
        uint8_t fpga_data = current_pattern_val >> 4; 
        
        // Pull CS low to wake up the FPGA
        gpio_put(SPI_CS_PIN, 0);
        
        // Send the 1 byte data package out the MOSI pin
        spi_write_blocking(spi0, &fpga_data, 1);
        
        // Pull CS high to tell the FPGA we are done
        gpio_put(SPI_CS_PIN, 1);

        // Wait for the next frame
        sleep_ms(delay_ms);
    }

    return 0;
}