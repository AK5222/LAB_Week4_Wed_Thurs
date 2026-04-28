# Lab Week 04 Wed-Thurs

This lab, you'll be implementing Christmas light controller on the Pico. This time we aren't breaking it into exercises. We're just giving a high level description of the system.

You'll be exploring:
- Device-to-device communication
- Configuring SPI Communication on the RPPico
- Implementing SPI (slave) in Verilog

### Required components
1. FPGA (either ICESuger or ICESuger-pro)
1. 1x DIP switch (in total 1 switch will be used)
1. 2x Raspberry Pi Picos (2nd pico is for extra credit points)
1. 12x LEDs (with 220Ohm resistors)
1. Breadboard + enough wires to connect everything

### Wiring

- Connect four LEDs to each of the two picos, and the remaining four LEDs to the FPGA.
- Designate one of the two picos as the SPI Master, and hook up the slaves to the master.
  - This means allocating wires for CS, CLK, and MOSI (we don't need to worry about MISO, because the slaves don't reply)

## System Description

You are creating Christmas Lights! There will be two animated LED patterns that can be shown on 8 LEDs(12 for the extra credit). You will control these LEDs across 2 devices--with each device controlling four LEDs.

The devices will speak with each other using the SPI communication protocol. The devices you have at hand: 1x picos, 1x FPGA.

One of the picos will serve as the SPI master, the other two devices as SPI slaves.

The master pico generates a 12-bit pattern with each bit controlling one of the twelve LEDs.
- 4 bits are used by the SPI Master to control its 4 LEDs
- 4 bits are to be sent to the FPGA slave which will update its LEDs.
- (EC) The last 4 bits are sent over SPI to the pico slave. The pico slave will consume the data, and display the value on the LEDs

A single switch toggles between two animations that can be displayed on the LEDs.

A potentiometer controls how fast the pattern proceeds through its animation. The potentiometer should increase (or decrease) the animation speed by 100ms. The slowest speed should be 1s / frame. The highest speed should be 100ms / frame. "Frame" in the sense it's one step in the animation.

## [Demo Video](https://youtu.be/AtTU1ofXxoA?si=HjFPRcikXpi0CPEA)
The Demo Video linked shows the 2 patterns you will need to implement

## Pico Hardware SPI
### Dip Switch 
You might need to specify pullup or pulldown for your DIP switch input. After initializing it, you can use either `gpio_pull_up (uint gpio)` or `gpio_pull_down (uint gpio)` to set an input as pullup or pulldown.

### SPI Functions
The raspberry Pi Pico has 2 internal SPI controllers that you can use for this lab. Take a look at the Pico's pinout to find which pins are connect to the spi controllers. In order to use it, you wil need to add `hardware_spi` to your target_link_libraries in your CMakeLists.txt and then include hardware/spi.h in your code.

Here are some functions that you will need to use in order to complete this lab:

1. `spi_init (spi_inst_t * spi, uint baudrate)` 
    - Initializes one of the 2 spi controllers and sets the clock speed for SPI
2. `gpio_set_function (uint gpio, gpio_function_t fn)`
    - sets the funtionality of a specific gpio pin
    - you will need to call this for every pin you will use for SPI **(EXCEPT THE CS PINS ON THE MASTER PICO)** and you will need to set the the fn parameter to be `GPIO_FUNC_SPI`
3. `spi_write_blocking (spi_inst_t * spi, const uint8_t * src, size_t len)`
    - used to actually transmit data
    - `spi` parameter is used to pick which spi controller you are using
    - `src` is the data being sent. It should be an array of 8 bit values
    - `len` is the number of 8 bit values you want to send from `src`
4. `spi_set_slave (spi_inst_t * spi, bool slave)`
    - used to set a pico as a slave instead of master
5. `spi_is_readable (const spi_inst_t * spi)`
    - Checks if anything has been sent over SPI
6. `spi_read_blocking (spi_inst_t * spi, uint8_t repeated_tx_data, uint8_t * dst, size_t len)`
    - reads data that was sent over SPI
    - parameters are similar to the write version
    - `repeated_tx_data` is data to be sent the other way. Since we aren't sending anything back, this can just be 0
7. `spi_set_format (spi_inst_t * spi, uint data_bits, spi_cpol_t cpol, spi_cpha_t cpha, __unused spi_order_t order)`
    - This is optional depending on your CS pin ussage on the master pico and your CS pin set up on the slave
    - When the SSPCLKOUT phase is set to mode 0(which it is by default), the slave pico expects to see the CS pin actually toggle from low to high after a read and then back to low for a new read. If it doesn't see this behavior, it just ignores any new data.
    - In order to make reading possible on the slave pico while in SSPCLKOUT phase mode 0, you will need to make sure you are toggling the CS chip correctly on the master and you are using the default CS pin on the slave(GPIO pin 17 for spi0) and used the `gpio_set_function()` function to set it as an SPI pin. If you want to use a different pin as the CS pin on the slave, you will need to set SSPCLKOUT phase to mode 1 and then manually check the CS pin before every read.
    - here is an example for how to set it SSPCLKOUT phase to mode 1: `spi_set_format(spi_default, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);`


If there are any functions that you would like more details on, you can search for them in the [raspberry pi pico sdk documentation](https://www.raspberrypi.com/documentation/pico-sdk/) page.

## Extra Credit
The original version of this lab required 2 Raspberry Pi picos, however, the class parts list only listed one pico as required. Because of this, we have removed the need for the 2nd Pico and made it extra credit. Now, the current requirements to receive full credit on this lab is to implement the 1st pico and the FPGA meaning a total of 8 LEDs. If you implement the 2nd pico and display the pattern on all 12 LEDs, you will recieve 15 points of extra credit. The raspberry Pi pico ahs 2 seperate SPI controllers that you can use. In order to get the extra credit, you must use the same SPI controller to send data to both the FPGA and the 2nd Pico. If you do not have a 2nd pico but want to complete the extra credit assignment, you can borrow one from Allan.
