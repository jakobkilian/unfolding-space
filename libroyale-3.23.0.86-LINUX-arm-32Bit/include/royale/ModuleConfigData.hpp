/****************************************************************************\
* Copyright (C) 2016 Infineon Technologies
*
* THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
* PARTICULAR PURPOSE.
*
\****************************************************************************/

#pragma once

#include <config/ModuleConfig.hpp>

namespace royale
{
    namespace config
    {
        /**
        * The module configurations are not exported directly from the library.
        * The full list is available via royale::config::getUsbProbeDataRoyale()
        */
        namespace moduleconfig
        {
            /**
            * Configuration for the PicoFlexx with Enclustra firmware.
            */
            extern const royale::config::ModuleConfig PicoFlexxU6;

            /**
            * This is the fallback configuration for the Animator CX3 bring-up board with both UVC
            * and Amundsen firmware.
            *
            * Users of the bring-up board are recommended to copy this config to a new file, assign
            * a product identifier to the device, and configure the ModuleConfigFactoryAnimatorBoard
            * to detect that new hardware by the product identifier and use the new file.  This is
            * not necessary, but is a good practice for handling multiple devices instead of
            * altering the AnimatorDefault for each one.
            *
            * This default is able to use the imager with any access level, unlike the other
            * xxxDefault configs which typically use an eye-safe-unless-level-three fallback.
            */
            extern const royale::config::ModuleConfig AnimatorDefault;

            /**
            * Configuration for pico monstar (first diffuser)
            */
            extern const royale::config::ModuleConfig PicoMonstar1;

            /**
            * Configuration for pico monstar (second diffuser)
            */
            extern const royale::config::ModuleConfig PicoMonstar2;

            /**
            * Configuration for pico monstar 850nm rev 4 with glass lens
            */
            extern const royale::config::ModuleConfig PicoMonstar850nmGlass;

            /**
            * Configuration for pico monstar 850nm rev 4 with glass lens (2019 batch)
            */
            extern const royale::config::ModuleConfig PicoMonstar850nmGlass2;

            /**
            * Configuration for pico monstar 940nm
            */
            extern const royale::config::ModuleConfig PicoMonstar940nm;

            /**
            * Default configuration for pico monstar with only one calibration
            * use case
            */
            extern const royale::config::ModuleConfig PicoMonstarDefault;

            /**
            * Configuration for the Salome camera modules with 940nm VCSel
            * K9 / 60x45 / 945nm / 202 / PO
            */
            extern const royale::config::ModuleConfig Salome940nm;

            /**
            * Configuration for the Salome Rev 2 camera modules with 940nm VCSel
            * K9 or Leica / 60x45 / 945nm / 202 / PO / ICM or ECM
            */
            extern const royale::config::ModuleConfig Salome2Rev940nm;

            /**
            * Default configuration for the Salome camera modules that only
            * offers a calibration use case
            */
            extern const royale::config::ModuleConfig SalomeDefault;

            /**
            * Configuration for X1 850nm 2W
            */
            extern const royale::config::ModuleConfig X18502W;

            /**
            * Configuration for the Selene camera modules with ICM
            */
            extern const royale::config::ModuleConfig SeleneIcm;

            /**
            * Default configuration for the Selene camera modules that only
            * offers a calibration use case
            */
            extern const royale::config::ModuleConfig SeleneDefault;

            /**
            * Configuration for the Orpheus camera modules
            */
            extern const royale::config::ModuleConfig Orpheus;

            /**
            * Default configuration for the Orpheus camera modules that only
            * offers a calibration use case
            */
            extern const royale::config::ModuleConfig OrpheusDefault;

            /**
            * Default configuration for the Orat45 camera module
            */
            extern const royale::config::ModuleConfig Orat45;

            /**
            * Default configuration for an unknown pmd module (M2453 versions)
            */
            extern const royale::config::ModuleConfig PmdModule238xDefault;

            /**
            * Default configuration for an unknown pmd module (M2455 versions)
            */
            extern const royale::config::ModuleConfig PmdModule277xDefault;

            /**
            * Default configuration for the MTT016 camera module
            */
            extern const royale::config::ModuleConfig MTT016;
        }
    }
}
