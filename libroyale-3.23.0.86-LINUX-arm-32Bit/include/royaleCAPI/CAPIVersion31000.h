/****************************************************************************\
* Copyright (C) 2017 Infineon Technologies
*
* THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
* PARTICULAR PURPOSE.
*
\****************************************************************************/

/**
* \addtogroup royaleCAPI
* @{
*/

#pragma once

#ifndef ROYALE_C_API_VERSION
#define ROYALE_C_API_VERSION 31000
#endif

#if ROYALE_C_API_VERSION != 31000
#error Cannot mix different C API versions within one file!
#endif

/* C API Version 31000 defines for royale_camera_device */

#define royale_camera_device_destroy                           royale_camera_device_destroy_v210
#define royale_camera_device_initialize                        royale_camera_device_initialize_v210
#define royale_camera_device_get_id                            royale_camera_device_get_id_v220
#define royale_camera_device_get_camera_name                   royale_camera_device_get_camera_name_v220
#define royale_camera_device_get_camera_info                   royale_camera_device_get_camera_info_v220
#define royale_camera_device_set_use_case                      royale_camera_device_set_use_case_v210
#define royale_camera_device_get_use_cases                     royale_camera_device_get_use_cases_v220
#define royale_camera_device_get_current_use_case              royale_camera_device_get_current_use_case_v220
#define royale_camera_device_set_exposure_time                 royale_camera_device_set_exposure_time_v300
#define royale_camera_device_set_exposure_mode                 royale_camera_device_set_exposure_mode_v300
#define royale_camera_device_get_exposure_mode                 royale_camera_device_get_exposure_mode_v300
#define royale_camera_device_get_exposure_limits               royale_camera_device_get_exposure_limits_v300
#define royale_camera_device_start_capture                     royale_camera_device_start_capture_v210
#define royale_camera_device_stop_capture                      royale_camera_device_stop_capture_v210
#define royale_camera_device_get_max_sensor_width              royale_camera_device_get_max_sensor_width_v220
#define royale_camera_device_get_max_sensor_height             royale_camera_device_get_max_sensor_height_v220
#define royale_camera_device_get_lens_parameters               royale_camera_device_get_lens_parameters_v210
#define royale_camera_device_is_connected                      royale_camera_device_is_connected_v220
#define royale_camera_device_is_calibrated                     royale_camera_device_is_calibrated_v220
#define royale_camera_device_is_capturing                      royale_camera_device_is_capturing_v220
#define royale_camera_device_get_access_level                  royale_camera_device_get_access_level_v220
#define royale_camera_device_start_recording                   royale_camera_device_start_recording_v210
#define royale_camera_device_stop_recording                    royale_camera_device_stop_recording_v210
#define royale_camera_device_register_record_stop_listener     royale_camera_device_register_record_stop_listener_v210
#define royale_camera_device_register_exposure_listener_stream royale_camera_device_register_exposure_listener_stream_v300
#define royale_camera_device_register_data_listener            royale_camera_device_register_data_listener_v210
#define royale_camera_device_register_depth_image_listener     royale_camera_device_register_depth_image_listener_v210
#define royale_camera_device_register_ir_image_listener        royale_camera_device_register_ir_image_listener_v210
#define royale_camera_device_register_spc_listener             royale_camera_device_register_spc_listener_v210
#define royale_camera_device_register_event_listener           royale_camera_device_register_event_listener_v210
#define royale_camera_device_unregister_record_stop_listener   royale_camera_device_unregister_record_stop_listener_v210
#define royale_camera_device_unregister_exposure_listener      royale_camera_device_unregister_exposure_listener_v210
#define royale_camera_device_unregister_data_listener          royale_camera_device_unregister_data_listener_v210
#define royale_camera_device_unregister_depth_image_listener   royale_camera_device_unregister_depth_image_listener_v210
#define royale_camera_device_unregister_ir_image_listener      royale_camera_device_unregister_ir_image_listener_v210
#define royale_camera_device_unregister_spc_listener           royale_camera_device_unregister_spc_listener_v210
#define royale_camera_device_unregister_event_listener         royale_camera_device_unregister_event_listener_v210
#define royale_camera_device_set_frame_rate                    royale_camera_device_set_frame_rate_v210
#define royale_camera_device_get_frame_rate                    royale_camera_device_get_frame_rate_v220
#define royale_camera_device_get_max_frame_rate                royale_camera_device_get_max_frame_rate_v220
#define royale_camera_device_get_streams                       royale_camera_device_get_streams_v300
#define royale_camera_device_get_number_of_streams             royale_camera_device_get_number_of_streams_v330
#define royale_camera_device_set_external_trigger              royale_camera_device_set_external_trigger_v330
#define royale_camera_device_set_processing_parameters         royale_camera_device_set_processing_parameters_v300
#define royale_camera_device_get_processing_parameters         royale_camera_device_get_processing_parameters_v300
#define royale_camera_device_set_exposure_times                royale_camera_device_set_exposure_times_v300
#define royale_camera_device_set_exposure_for_groups           royale_camera_device_set_exposure_for_groups_v300
#define royale_camera_device_set_group_exposure_time           royale_camera_device_set_group_exposure_time_v300
#define royale_camera_device_get_group_exposure_limits         royale_camera_device_get_group_exposure_limits_v300
#define royale_camera_device_get_exposure_groups               royale_camera_device_get_exposure_groups_v300
#define royale_camera_device_register_extended_data_listener   royale_camera_device_register_extended_data_listener_v210
#define royale_camera_device_unregister_extended_data_listener royale_camera_device_unregister_extended_data_listener_v210
#define royale_camera_device_set_callback_data                 royale_camera_device_set_callback_data_v210
#define royale_camera_device_set_callback_dataU16              royale_camera_device_set_callback_dataU16_v210
#define royale_camera_device_set_calibration_data              royale_camera_device_set_calibration_data_v210
#define royale_camera_device_get_calibration_data              royale_camera_device_get_calibration_data_v210
#define royale_camera_device_write_calibration_to_flash        royale_camera_device_write_calibration_to_flash_v210
#define royale_camera_device_set_duty_cycle                    royale_camera_device_set_duty_cycle_v210
#define royale_camera_device_write_registers                   royale_camera_device_write_registers_v210
#define royale_camera_device_read_registers                    royale_camera_device_read_registers_v210
#define royale_camera_device_shift_lens_center                 royale_camera_device_shift_lens_center_v320
#define royale_camera_device_get_lens_center                   royale_camera_device_get_lens_center_v320
#define royale_camera_device_write_data_to_flash               royale_camera_device_write_data_to_flash_v31000
#define royale_camera_device_write_data_to_flash_file          royale_camera_device_write_data_to_flash_file_v31000
#define royale_camera_device_initialize_with_use_case          royale_camera_device_initialize_with_use_case_v210


/* C API Version 31000 defines for royale (StatusCAPI.h) */

#define royale_get_version                                     royale_get_version_v220
#define royale_get_version_with_build                          royale_get_version_with_build_v220
#define royale_get_version_with_build_and_scm_revision         royale_get_version_with_build_and_scm_revision_v320

/* C API Version 31000 defines for structures / typedefs */

#define ROYALE_EXPOSURE_CALLBACK register_exposure_listener has changed its callback type
#define ROYALE_EXPOSURE_STREAM_CALLBACK ROYALE_EXPOSURE_CALLBACK_v300

/** @}*/
