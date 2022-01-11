Code in this folder is taken from https://github.com/simondlevy/CrossPlatformDataBus (but edited in some points)

CrossPlatformDataBus simplifies i2c communication on the Raspberry Pi. It uses the (now deprecated) wiringpi lib. There is a repository (using CrossPlatformDataBus) that offers all register writes and reads of the LSM6DSM so that I did not have to study the registers on my own...

Furthermore official wiringpi did not support the Raspi CM4 module. We use https://github.com/WiringPi/WiringPi which also supports the CM4.



```c++
/*
   WiringPiI2C.cpp: WiringPi implementation of cross-platform I2C routines

   This file is part of CrossPlatformDataBus.

   CrossPlatformDataBus is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   CrossPlatformDataBus is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with CrossPlatformDataBus.  If not, see <http://www.gnu.org/licenses/>.
*/
```

