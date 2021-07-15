/* 
   LSM6DSM.h: Header file for LSM6DSM class

   Copyright (C) 2018 Simon D. Levy

   Adapted from https://github.com/kriswiner/LSM6DSM_LIS2MDL_LPS22HB

   This file is part of LSM6DSM.

   LSM6DSM is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   LSM6DSM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with LSM6DSM.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdint.h>

#include "CrossPlatformI2C_Core.h"

// One ifdef needed to support delay() cross-platform
#if defined(ARDUINO)
#include <Arduino.h>

#elif defined(__arm__) 
#if defined(STM32F303)  || defined(STM32F405xx)
extern "C" { void delay(uint32_t msec); }
}
#else
#include <wiringPi.h>
#endif

#else
void delay(uint32_t msec);
#endif

class LSM6DSM
{
    public:

        typedef enum {

            // Note order!
            AFS_2G,
            AFS_16G,
            AFS_4G,
            AFS_8G

        } Ascale_t;

        typedef enum {

            GFS_245DPS,
            GFS_500DPS,
            GFS_1000DPS,
            GFS_2000DPS

        } Gscale_t;

        typedef enum {

            ODR_12_5Hz,
            ODR_26Hz,
            ODR_52Hz,
            ODR_104Hz,
            ODR_208Hz,
            ODR_416Hz,
            ODR_833Hz,
            ODR_1660Hz,
            ODR_3330Hz,
            ODR_6660Hz

        } Rate_t;

        typedef enum {

            ERROR_NONE,
            ERROR_CONNECT,
            ERROR_ID,
            ERROR_SELFTEST

        } Error_t;

        LSM6DSM(Ascale_t ascale, Gscale_t gscale, Rate_t aodr, Rate_t godr, float accelBias[3], float gyroBias[3]);

        LSM6DSM(Ascale_t ascale, Gscale_t gscale, Rate_t aodr, Rate_t godr);

        Error_t begin(uint8_t bus=1);

        void calibrate(float * gyroBias, float * accelBias);

        void clearInterrupt(void);

        bool checkNewData(void);

        void readData(float & ax, float & ay, float & az, float & gx, float & gy, float & gz);


    private:

        static const uint8_t ADDRESS = 0x6A;

        /* LSM6DSM registers
http://www.st.com/content/ccc/resource/technical/document/datasheet/76/27/cf/88/c5/03/42/6b/DM00218116.pdf/files/DM00218116.pdf/jcr:content/translations/en.DM00218116.pdf
         */
        static const uint8_t FUNC_CFG_ACCESS           = 0x01;
        static const uint8_t SENSOR_SYNC_TIME_FRAME    = 0x04;
        static const uint8_t SENSOR_SYNC_RES_RATIO     = 0x05;
        static const uint8_t FIFO_CTRL1                = 0x06;
        static const uint8_t FIFO_CTRL2                = 0x07;
        static const uint8_t FIFO_CTRL3                = 0x08;
        static const uint8_t FIFO_CTRL4                = 0x09;
        static const uint8_t FIFO_CTRL5                = 0x0A;
        static const uint8_t DRDY_PULSE_CFG            = 0x0B;
        static const uint8_t INT1_CTRL                 = 0x0D;
        static const uint8_t INT2_CTRL                 = 0x0E;
        static const uint8_t WHO_AM_I                  = 0x0F; 
        static const uint8_t CTRL1_XL                  = 0x10;
        static const uint8_t CTRL2_G                   = 0x11;
        static const uint8_t CTRL3_C                   = 0x12;
        static const uint8_t CTRL4_C                   = 0x13;
        static const uint8_t CTRL5_C                   = 0x14;
        static const uint8_t CTRL6_C                   = 0x15;
        static const uint8_t CTRL7_G                   = 0x16;
        static const uint8_t CTRL8_XL                  = 0x17;
        static const uint8_t CTRL9_XL                  = 0x18;
        static const uint8_t CTRL10_C                  = 0x19;
        static const uint8_t MASTER_CONFIG             = 0x1A;
        static const uint8_t WAKE_UP_SRC               = 0x1B;
        static const uint8_t TAP_SRC                   = 0x1C;
        static const uint8_t D6D_SRC                   = 0x1D;
        static const uint8_t STATUS_REG                = 0x1E;
        static const uint8_t OUT_TEMP_L                = 0x20;
        static const uint8_t OUT_TEMP_H                = 0x21;
        static const uint8_t OUTX_L_G                  = 0x22;
        static const uint8_t OUTX_H_G                  = 0x23;
        static const uint8_t OUTY_L_G                  = 0x24;
        static const uint8_t OUTY_H_G                  = 0x25;
        static const uint8_t OUTZ_L_G                  = 0x26;
        static const uint8_t OUTZ_H_G                  = 0x27;
        static const uint8_t OUTX_L_XL                 = 0x28;
        static const uint8_t OUTX_H_XL                 = 0x29;
        static const uint8_t OUTY_L_XL                 = 0x2A;
        static const uint8_t OUTY_H_XL                 = 0x2B;
        static const uint8_t OUTZ_L_XL                 = 0x2C;
        static const uint8_t OUTZ_H_XL                 = 0x2D;
        static const uint8_t SENSORHUB1_REG            = 0x2E;
        static const uint8_t SENSORHUB2_REG            = 0x2F;
        static const uint8_t SENSORHUB3_REG            = 0x30;
        static const uint8_t SENSORHUB4_REG            = 0x31;
        static const uint8_t SENSORHUB5_REG            = 0x32;
        static const uint8_t SENSORHUB6_REG            = 0x33;
        static const uint8_t SENSORHUB7_REG            = 0x34;
        static const uint8_t SENSORHUB8_REG            = 0x35;
        static const uint8_t SENSORHUB9_REG            = 0x36;
        static const uint8_t SENSORHUB10_REG           = 0x37;
        static const uint8_t SENSORHUB11_REG           = 0x38;
        static const uint8_t SENSORHUB12_REG           = 0x39;
        static const uint8_t FIFO_STATUS1              = 0x3A;
        static const uint8_t FIFO_STATUS2              = 0x3B;
        static const uint8_t FIFO_STATUS3              = 0x3C;
        static const uint8_t FIFO_STATUS4              = 0x3D;
        static const uint8_t FIFO_DATA_OUT_L           = 0x3E;
        static const uint8_t FIFO_DATA_OUT_H           = 0x3F;
        static const uint8_t TIMESTAMP0_REG            = 0x40;
        static const uint8_t TIMESTAMP1_REG            = 0x41;
        static const uint8_t TIMESTAMP2_REG            = 0x42;
        static const uint8_t STEP_TIMESTAMP_L          = 0x49;
        static const uint8_t STEP_TIMESTAMP_H          = 0x4A;
        static const uint8_t STEP_COUNTER_L            = 0x4B;
        static const uint8_t STEP_COUNTER_H            = 0x4C;
        static const uint8_t SENSORHUB13_REG           = 0x4D;
        static const uint8_t SENSORHUB14_REG           = 0x4E;
        static const uint8_t SENSORHUB15_REG           = 0x4F;
        static const uint8_t SENSORHUB16_REG           = 0x50;
        static const uint8_t SENSORHUB17_REG           = 0x51;
        static const uint8_t SENSORHUB18_REG           = 0x52;
        static const uint8_t FUNC_SRC1                 = 0x53;
        static const uint8_t FUNC_SRC2                 = 0x54;
        static const uint8_t WRIST_TILT_IA             = 0x55;
        static const uint8_t TAP_CFG                   = 0x58;
        static const uint8_t TAP_THS_6D                = 0x59;
        static const uint8_t INT_DUR2                  = 0x5A;
        static const uint8_t WAKE_UP_THS               = 0x5B;
        static const uint8_t WAKE_UP_DUR               = 0x5C;
        static const uint8_t FREE_FALL                 = 0x5D;
        static const uint8_t MD1_CFG                   = 0x5E;
        static const uint8_t MD2_CFG                   = 0x5F;
        static const uint8_t MASTER_MODE_CODE          = 0x60;
        static const uint8_t SENS_SYNC_SPI_ERROR_CODE  = 0x61;
        static const uint8_t OUT_MAG_RAW_X_L           = 0x66;
        static const uint8_t OUT_MAG_RAW_X_H           = 0x67;
        static const uint8_t OUT_MAG_RAW_Y_L           = 0x68;
        static const uint8_t OUT_MAG_RAW_Y_H           = 0x69;
        static const uint8_t OUT_MAG_RAW_Z_L           = 0x6A;
        static const uint8_t OUT_MAG_RAW_Z_H           = 0x6B;
        static const uint8_t INT_OIS                   = 0x6F;
        static const uint8_t CTRL1_OIS                 = 0x70;
        static const uint8_t CTRL2_OIS                 = 0x71;
        static const uint8_t CTRL3_OIS                 = 0x72;
        static const uint8_t X_OFS_USR                 = 0x73;
        static const uint8_t Y_OFS_USR                 = 0x74;
        static const uint8_t Z_OFS_USR                 = 0x75;

        // Self-test bounds
        static constexpr float ACCEL_MIN = .09;
        static constexpr float ACCEL_MAX = 1.7;
        static constexpr float GYRO_MIN  = 20;
        static constexpr float GYRO_MAX  = 80;

        // Computed by calibrate()
        float _accelBias[3];
        float _gyroBias[3];

        // Stored by constructor
        Ascale_t _ascale;
        Gscale_t _gscale;
        Rate_t   _aodr;
        Rate_t   _godr;

        // Computed by constructor
        float _ares;
        float _gres;

        bool selfTest(void);

        void readData(int16_t data[7]);

        // Self-test helpers
        bool inBounds(int16_t ptest[3], int16_t ntest[3], int16_t nom[3], float res, float minval, float maxval);
        bool outOfBounds(float val, float minval, float maxval);

        // I^2C business

        uint8_t _i2c; // Support for wiringPi, I2CDEV

        void    writeRegister(uint8_t subAddress, uint8_t data);
        uint8_t readRegister(uint8_t subAddress);
        void    readRegisters(uint8_t subAddress, uint8_t count, uint8_t * dest);

};
