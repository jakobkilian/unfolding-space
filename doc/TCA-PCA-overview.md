# Overview of register programming on the DRV2605l

To target the drv2605l ICs seperately (which are on the same adresse) we use a i2c multiplexer (TCA9584a) to route i2c signals to the targeted DRV. There may be other ways to solve this (enable/disable chips, using pwn, ...) but due to time restriction we stuck with this solution for the moment.To write and read we currently still use the *deprecated* WiringPi library functions (e.g. wiringPiI2CWrite).



## Address and Multiplexing

The adress of the PCA/TCA is set by pulling down pins. On the current PCB adresses are:

#### TCA 0

Hosting DRV 0-4

- **0x70** (7-bit) or **1110000**
- **0xE0** (8-bit ) or **11100000** for writing
- **0xE1** (8-bit ) or **11100001** for reading



#### TCA 1

Hosting DRV 5-8 & LSM (Gyro+Acc)

- **0x72** (7-bit) or **1110010**
- **0xE4** (8-bit ) or **11100100** for writing
- **0xE5** (8-bit ) or **11100101** for reading





## Registers / How to open lines

We access the TCA by just writing on the control register by using wiringPiI2CWrite(tca0, DATA) 

There is one bit for each of the 8 lines. You can open or close them by writing 0 or 1. Last bit is line 0, second last is line 1 and so on...

- 00000001 would close all lines exept from line 0 which is now open
- 11000000 would open line 7 and line 6 but close the others
- 00000000 would close all channels. equal to power-up/reset/default*



the function selectSingleMuxLine in i2c.cpp is written to be able to open only one line at a time: It uses bitshifting to shift a "1" by lineNo digits/positions



## Is this identical with the PCA9635? 

We have to check if we can use the PCA the exact same way... TCA was out of stock...
