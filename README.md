# lizard-living
ESP2866 and Arduino NeoPixel-based lizard habitat control
## Features
button and web based interface to switch between two pre-programmed RGB settings for the LEDs (day/night)
on boot, retrieves time of day from time.com to set initial RGB setting
failing web connect, defaults to night to prevent rude brightness
## TODO
implement web time polling for automatic day/night cycle
integrate RTC support to minimize web polling, and to persist timekeeping through wifi outages
implement web based RGB sliders to tinker with RGB settings for day/night

integrate temperature sensor and implement auto on/off for heating element based on temperature
add button and web based interface for adjusting desired temp
add LCD readout for temp
