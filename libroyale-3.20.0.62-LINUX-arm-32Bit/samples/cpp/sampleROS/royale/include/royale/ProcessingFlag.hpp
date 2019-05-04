/****************************************************************************\
 * Copyright (C) 2015 pmdtechnologies ag
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 \****************************************************************************/

#pragma once

#include <royale/Variant.hpp>
#include <royale/Vector.hpp>
#include <royale/String.hpp>

namespace royale
{
    /*!
     *  This is a list of flags which can be set/altered in access LEVEL 2 in order
     *  to control the processing pipeline. The suffix type indicates the expected Variant type.
     *  For a more detailed description of the different parameters please refer to the documentation
     *  you will receive after getting LEVEL 2 access.
     *
     *  Keep in mind, that if this list is changed, the map with names has to be adapted!
     */
    enum class ProcessingFlag
    {
        ConsistencyTolerance_Float = 0,     ///< Consistency limit for asymmetry validation
        FlyingPixelsF0_Float,               ///< Scaling factor for lower depth value normalization
        FlyingPixelsF1_Float,               ///< Scaling factor for upper depth value normalization
        FlyingPixelsFarDist_Float,          ///< Upper normalized threshold value for flying pixel detection
        FlyingPixelsNearDist_Float,         ///< Lower normalized threshold value for flying pixel detection
        LowerSaturationThreshold_Int,       ///< Lower limit for valid raw data values
        UpperSaturationThreshold_Int,       ///< Upper limit for valid raw data values
        MPIAmpThreshold_Float,              ///< Threshold for MPI flags triggered by amplitude discrepancy
        MPIDistThreshold_Float,             ///< Threshold for MPI flags triggered by distance discrepancy
        MPINoiseDistance_Float,             ///< Threshold for MPI flags triggered by noise
        NoiseThreshold_Float,               ///< Upper threshold for final distance noise
        AdaptiveNoiseFilterType_Int,        ///< Kernel type of the adaptive noise filter
        AutoExposureRefAmplitude_Float,     ///< DEPRECATED : The reference amplitude for the new exposure estimate
        UseAdaptiveNoiseFilter_Bool,        ///< Activate spatial filter reducing the distance noise
        UseAutoExposure_Bool,               ///< DEPRECATED : Activate dynamic control of the exposure time
        UseRemoveFlyingPixel_Bool,          ///< Activate FlyingPixel flag
        UseMPIFlagAverage_Bool,             ///< Activate spatial averaging MPI value before thresholding
        UseMPIFlag_Amp_Bool,                ///< Activates MPI-amplitude flag
        UseMPIFlag_Dist_Bool,               ///< Activates MPI-distance flag
        UseValidateImage_Bool,              ///< Activates output image validation
        UseRemoveStrayLight_Bool,           ///< Activates the removal of stray light
        UseSparsePointCloud_Bool,           ///< Creates a sparse-point cloud in Spectre
        UseFilter2Freq_Bool,                ///< Activates 2 frequency filtering
        GlobalBinning_Int,                  ///< Sets the size of the global binning kernel
        UseAdaptiveBinning_Bool,            ///< Activates adaptive binning
        AutoExposureRefValue_Float,         ///< The reference value for the new exposure estimate
        NUM_FLAGS
    };

    /*!
    * For debugging, printable strings corresponding to the ProcessingFlag enumeration.  The
    * returned value is copy of the processing flag name. If the processing flag is not found
    * an empty string will be returned.
    *
    * These strings will not be localized.
    */
    ROYALE_API royale::String getProcessingFlagName (royale::ProcessingFlag mode);

    /*!
    *  This is a map combining a set of flags which can be set/altered in access LEVEL 2 and the set value as Variant type.
    *  The proposed minimum and maximum limits are recommendations for reasonable results. Values beyond these boundaries are
    *  permitted, but are currently neither evaluated nor verified.
    *

    The Corresponding unit types are applied, which are related to physical and technical basic units

    |   Unit    | Description                                                                                                       |
    |:---------:|:-----------------------------------------------------------------------------------------------------------------:|
    | Boolean   | Logical data type, which comprises two states true/on and false/off                                               |
    | Enum      | Enumeration used for operation specification                                                                      |
    | Distance  | Distance value related to a physical dimension in metre                                                           |
    | Amplitude | Amplitude value information, which is related to the bit-depth readout value of the imager currently set at 12Bit |


    FlyingPixel Flag
    ----------------

    The flying pixel flags operates on the final depth (Z) values. The absolute depth differences to the right and left neighbor
    (or to the top and bottom one) are compared with a certain threshold. If both differences (either left/right or top/bottom)
    exceed this limit the flag is set. The threshold depends on the actual depth-value and is linearly scaled to lie between
    flyingPixel_f_0 (for Z-values of flyingPixel_nearDist or closer) and flyingPixel_f_1 (for Z-values of flyingPixel_farDist or larger).
    The Flag works better and more reliable if the NoiseFilter is switched on.

    | ProcessingFlag                | Default | Minimum | Maximum |  Type  |   Unit    | Description                                                           |
    |-------------------------------|:-------:|:-------:|:-------:|:------:|:---------:|:---------------------------------------------------------------------:|
    | UseRemoveFlyingPixel_Bool     |    true |   false |    true |   bool | Boolean   | Activate FlyingPixel flag                                             |
    | FlyingPixelsF0_Float          |   0.018 |    0.01 |    0.04 |  float | Distance  | Scaling Factor for lower depth value normalization                    |
    | FlyingPixelsF1_Float          |    0.14 |     n.d.|     n.d.|  float | Distance  | Scaling Factor for upper depth value normalization                    |
    | FlyingPixelsFarDist_Float     |     4.5 |     n.d.|     n.d.|  float | Distance  | Upper Normalized threshold value for flying pixel detection           |
    | FlyingPixelsNearDist_Float    |     1.0 |     n.d.|     n.d.|  float | Distance  | Lower Normalized threshold value for flying pixel detection           |


    Asymmetry Flag
    --------------

    Asymmetry flag is sensitive to motion artifacts, flash-lights, distortions caused by other ToF cameras. The asymmetry is scaled
    by the corresponding amplitude and the result compared to a threshold. Higher threshold values make the flag less sensitive to asymmetries
    (e.g. caused by motion artifacts). In high noise / low amplitude conditions the flag is more likely to be triggered. Hence decreasing
    the threshold might give better and more sensitive results in high amplitude regimes but worse results in low noise regimes.

    | ProcessingFlag                | Default | Minimum | Maximum |  Type  |   Unit    | Description                                                           |
    |-------------------------------|:-------:|:-------:|:-------:|:------:|:---------:|:---------------------------------------------------------------------:|
    | ConsistencyTolerance_Float    |     1.2 |     0.2 |     1.5 |  float | Distance  | Threshold for asymmetry flag calculation                              |


    MPI Flag
    --------

    MPI or Multi-Path-Interference is a systematic depth error which is related to the physical principal of interference.
    Compensation methods can only be applied in reference to multiple recording frequency an additional intensity information.
    MPI can be specifically addressed to distance and/or amplitude information.
    MPI evaluation is limited to the operation modes:

    > - MODE_9_10FPS_1000;
    > - MODE_9_5FPS_2000;
    > - MODE_9_15FPS_700;
    > - MODE_9_25FPS_450;

    MPI is detected with respect to four parameters.

    - **Amplitude Flag**
    : MPI-Amp-Flags compares the amplitudes of the two frequency measurements, which should have a certain relation. The quality of the flags
    strongly depends on amplitude wiggling correction. Hence under certain conditions it might be useful to deactivate this flag but keep the
    distance MPI flag active.

    - **Distance Flag**
    : The MPI distance flag compares the two distances of the two modulation frequencies, which should have a certain relation.
    If this relation is violated the pixel is set invalid.

    - **Averaging**
    : Since both MPI flags evaluate  signal differences (distances or amplitudes) the SNR is usually quite low. To increase quality
    the data is averaged (3x3 median filter) to make the thresholds more reliable. If deactivated mpiNoiseDistance should be increased
    to account for higher noise levels.

    - **Noise**
    : In high noise regimes the MPI flags might be triggered only by noise. Therefore in addition to the above two thresholds a
    noise safety margin is kept. For the two flags the calculated noise of the signal above is multiplied with this scaling factor and used to
    modify the above thresholds (the maximum of the fixed threshold and the noise scaled
    limit is used).

    | ProcessingFlag                | Default | Minimum | Maximum |  Type  |   Unit    | Description                                                           |
    |-------------------------------|:-------:|:-------:|:-------:|:------:|:---------:|:---------------------------------------------------------------------:|
    | UseMPIFlagAverage_Bool        |    true |   false |    true |   bool | Boolean   | Activate spatial averaging MPI value before thresholding              |
    | UseMPIFlag_Amp_Bool           |    true |   false |    true |   bool | Boolean   | Activates MPI-amplitude flag                                          |
    | UseMPIFlag_Dist_Bool          |    true |   false |    true |   bool | Boolean   | Activates MPI-distance flag                                           |
    | MPIAmpThreshold_Float         |     0.3 |     0.1 |     0.5 |  float | Amplitude | Threshold for MPI flags triggered by amplitude discrepancy            |
    | MPIDistThreshold_Float        |     0.1 |    0.05 |     0.2 |  float | Distance  | Threshold for MPI flags triggered by distance discrepancy             |
    | MPINoiseDistance_Float        |     3.0 |     2.0 |     5.0 |  float | Distance  | Soft scaling factor to avoid noise triggered misinterpretation        |


    High Noise Flag
    ---------------

    This value defines the upper limit to generate a distance value out of noisy raw data. In case the 3D image has a strange behavior in
    the near range, this value can be used for optimization. The threshold is applied to the noise levels before filtering.

    | ProcessingFlag                | Default | Minimum | Maximum |  Type  |   Unit    | Description                                                           |
    |-------------------------------|:-------:|:-------:|:-------:|:------:|:---------:|:---------------------------------------------------------------------:|
    | NoiseThreshold_Float          |    0.07 |    0.03 |     0.2 |  float | Distance  | Upper threshold for final distance noise                              |


    Saturation Flag
    ---------------

    Determines the valid data for the consequent depth data evaluation within a specific raw data range. Increasing/Decreasing this limit
    lowers/raises the applicable amplitude value and hence the working range. However distortions caused by non-linearity might increase.

    | ProcessingFlag                | Default | Minimum | Maximum |  Type  |   Unit    | Description                                                           |
    |-------------------------------|:-------:|:-------:|:-------:|:------:|:---------:|:---------------------------------------------------------------------:|
    | LowerSaturationThreshold_Int  |     400 |       0 |     600 |    int | Amplitude | Lower limit for valid raw data values                                 |
    | UpperSaturationThreshold_Int  |    3750 |    3500 |    4095 |    int | Amplitude | Upper limit for valid raw data values                                 |


    Adaptive Noise Filter
    ---------------------

    Spatial filter that uses the calculated distance noise to adaptively reduce the distance noise. Larger kernel size means better averaging,
    but also higher computational load. Value 1 corresponds (Kernel size 3x3) and a noise reduction factor
    a noise-reduction-factor of ~2.5 and value 2 (Kernel size 5x5) to  a noise-reduction-factor of ~3.5

    | ProcessingFlag                | Default | Minimum | Maximum |  Type  |   Unit    | Description                                                           |
    |-------------------------------|:-------:|:-------:|:-------:|:------:|:---------:|:---------------------------------------------------------------------:|
    | UseAdaptiveNoiseFilter_Bool   |    true |   false |    true |   bool | Boolean   | Activate spatial filter reducing the distance noise                   |
    | AdaptiveNoiseFilterType_Int   |       2 |       1 |       2 |    int | Enum      | Kernel type of the adaptive noise filter                              |


    Validate Image
    --------------

    The calculated output data is validated. Invalid values are set to 0 and the corresponding flag map is merged to 1 for invalid pixel and 0 for valid.

    | ProcessingFlag                | Default | Minimum | Maximum |  Type  |   Unit    | Description                                                           |
    |-------------------------------|:-------:|:-------:|:-------:|:------:|:---------:|:---------------------------------------------------------------------:|
    | UseValidateImage_Bool         |    true |   false |    true |   bool | Boolean   | Activates output image validation                                     |


    Stray light Removal
    --------------

    Stray light that occurs in the scene will be removed. This will only work for modules with a special calibration (the pico flexx uses hard coded values).
    If the camera calibration does not support the stray light removal algorithm and UseRemoveStrayLight_Bool is set to true you will have to restart the
    camera afterwards.
    You can observe the effect of stray light if you point the camera at a flat wall and start to insert your hand into the field of view from the side.

    | ProcessingFlag                | Default | Minimum | Maximum |  Type  |   Unit    | Description                                                           |
    |-------------------------------|:-------:|:-------:|:-------:|:------:|:---------:|:---------------------------------------------------------------------:|
    | UseRemoveStrayLight_Bool      |   false |   false |    true |   bool | Boolean   | Activates the stray light removal algorithm                           |


    Sparse Point Cloud
    --------------

    If this flag is set, Spectre will only output valid depth points. This might lead to sparse point clouds.
    If it is not set non valid depth points will be marked with a depth of zero.

    | ProcessingFlag                | Default | Minimum | Maximum |  Type  |   Unit    | Description                                                           |
    |-------------------------------|:-------:|:-------:|:-------:|:------:|:---------:|:---------------------------------------------------------------------:|
    | UseSparsePointCloud_Bool      |   false |   false |    true |   bool | Boolean   | Activates the output of a sparse point cloud                          |


    Two frequency filtering
    --------------

    If enabled Spectre will try to correct pixels which have been assigned to the wrong unambiguity range during the two-frequency calculation.
    Disabling this means that the noise threshold should be lowered as well, otherwise a lot of wrong distances might be calculated.

    | ProcessingFlag                | Default | Minimum | Maximum |  Type  |   Unit    | Description                                                           |
    |-------------------------------|:-------:|:-------:|:-------:|:------:|:---------:|:---------------------------------------------------------------------:|
    | UseFilter2Freq_Bool           |   true  |   false |    true |   bool | Boolean   | Activates the two frequency filtering                                 |


    Global binning
    --------------

    Sets the size of the global binning kernel. Global binning will merge multiple pixels and replace them with the mean. In this case global means
    that Spectre will use one kernel size for the whole image.
    Changing this value will change the size of the output data you'll receive in the callback!

    | ProcessingFlag                | Default | Minimum | Maximum |  Type  |   Unit    | Description                                                           |
    |-------------------------------|:-------:|:-------:|:-------:|:------:|:---------:|:---------------------------------------------------------------------:|
    | GlobalBinning_Int             |   1     |   1     |    9    |    int |           | Chooses the size for the global binning algorithm                     |


    Adaptive Binning
    --------------

    This will enable/disable the adaptive binning. Adaptive binning will automatically choose the appropriate parameter for the global binning
    according to the last frame.
    Enabling this feature will change the size of the output data you'll receive in the callback!

    | ProcessingFlag                | Default | Minimum | Maximum |  Type  |   Unit    | Description                                                           |
    |-------------------------------|:-------:|:-------:|:-------:|:------:|:---------:|:---------------------------------------------------------------------:|
    | UseAdaptiveBinning_Bool       |   false |   false |    true |   bool | Boolean   | Activates the adaptive global binning                                 |

    *
    */
    typedef royale::Vector< royale::Pair<royale::ProcessingFlag, royale::Variant> > ProcessingParameterVector;
    typedef std::map< royale::ProcessingFlag, royale::Variant > ProcessingParameterMap;
    typedef std::pair< royale::ProcessingFlag, royale::Variant > ProcessingParameterPair;

#ifndef SWIG

    /**
     * Takes ProcessingParameterMaps a and b and returns a combination of both.
     * Keys that exist in both maps will take the value of map b.
     */
    ROYALE_API ProcessingParameterMap combineProcessingMaps (const ProcessingParameterMap &a,
            const ProcessingParameterMap  &b);

    namespace parameter
    {
        static const ProcessingParameterPair stdConsistencyTolerance ({ royale::ProcessingFlag::ConsistencyTolerance_Float, royale::Variant (1.2f, 0.2f, 1.5f) });
        static const ProcessingParameterPair stdFlyingPixelsF0 ({ royale::ProcessingFlag::FlyingPixelsF0_Float, royale::Variant (0.018f, 0.01f, 0.04f) });
        static const ProcessingParameterPair stdFlyingPixelsF1 ({ royale::ProcessingFlag::FlyingPixelsF1_Float, royale::Variant (0.14f, 0.01f, 0.5f) });
        static const ProcessingParameterPair stdFlyingPixelsFarDist ({ royale::ProcessingFlag::FlyingPixelsFarDist_Float, royale::Variant (4.5f, 0.01f, 1000.0f) });
        static const ProcessingParameterPair stdFlyingPixelsNearDist ({ royale::ProcessingFlag::FlyingPixelsNearDist_Float, royale::Variant (1.0f, 0.01f, 1000.0f) });
        static const ProcessingParameterPair stdLowerSaturationThreshold ({ royale::ProcessingFlag::LowerSaturationThreshold_Int, royale::Variant (400, 0, 600) });
        static const ProcessingParameterPair stdUpperSaturationThreshold ({ royale::ProcessingFlag::UpperSaturationThreshold_Int, royale::Variant (3750, 3500, 4095) });
        static const ProcessingParameterPair stdMPIAmpThreshold ({ royale::ProcessingFlag::MPIAmpThreshold_Float, royale::Variant (0.3f, 0.1f, 0.5f) });
        static const ProcessingParameterPair stdMPIDistThreshold ({ royale::ProcessingFlag::MPIDistThreshold_Float, royale::Variant (0.1f, 0.05f, 0.2f) });
        static const ProcessingParameterPair stdMPINoiseDistance ({ royale::ProcessingFlag::MPINoiseDistance_Float, royale::Variant (3.0f, 2.0f, 5.0f) });
        static const ProcessingParameterPair stdNoiseThreshold ({ royale::ProcessingFlag::NoiseThreshold_Float, royale::Variant (0.07f, 0.03f, 0.2f) });
        static const ProcessingParameterPair stdAdaptiveNoiseFilterType ({ royale::ProcessingFlag::AdaptiveNoiseFilterType_Int, royale::Variant (1, 1, 2) });
        static const ProcessingParameterPair stdAutoExposureRefValue ({ royale::ProcessingFlag::AutoExposureRefValue_Float, 1000.0f });
        static const ProcessingParameterPair stdUseRemoveStrayLight ({ royale::ProcessingFlag::UseRemoveStrayLight_Bool, false });
    }
#endif
}
