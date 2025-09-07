## Microprocessors and Peripherals @ ΕCE-AUTh

### Lab 1
Implemented these functions in ARM Assembly and used them in C.
- Hashing Function
- Modulo dehashing and digit count
- Fibonacci recursive function
All the above were printed on the pc's terminal using the built-in UART drivers of the MCU.

### Lab 2
Utilization of on-board LEDs and buttons using GPIO and interrupt drivers of the MCU.
The highest interrupt priority was assigned to the button press, and a lower priority was assigned to the UART interface.
Different timings required different clock frequencies, so the main clock was used for frequency division into multiple time intervals.

### Lab 3
In this lab, a driver for the DHT11 sensor was designed and used along with 2 other GPIO (1 analog and 1 digital) sensors for determining the conditions inside a greenhouse. Built-in ADC and DAC drivers were used for this design.

### Info
[STM32F411RE](https://www.st.com/en/microcontrollers-microprocessors/stm32f411re.html) and [ARM Keil](https://www.keil.com/) were used throughout all labs. [PuTTY](https://www.chiark.greenend.org.uk/~sgtatham/putty/latest.html) was used for reading the UART signal from the USB port.
