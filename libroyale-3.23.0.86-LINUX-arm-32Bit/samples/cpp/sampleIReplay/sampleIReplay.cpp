/****************************************************************************\
 * Copyright (C) 2019 Infineon Technologies & pmdtechnologies ag
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 \****************************************************************************/

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include <royale.hpp>
#include <royale/IReplay.hpp>

namespace
{
    /**
    * This listener receives a signal "stop" from the IReplay interface.
    * This signal is send when:
    * 1. the ICameraDevice's stopCapture-method is called
    * 2. the IReplay interface is set to not loop and the capture reaches its last frame
    */
    class MyPlaybackStopListener : public royale::IPlaybackStopListener
    {
        void onPlaybackStopped() override
        {
            std::cout << "received a stop signal from the IReplay interface\n";
        }
    };

    /**
    * This listener receives depth data from an ICameraDevice.
    * If you want to know more about IDepthDataListener
    * look at the sampleRetrieveData.
    */
    class MyDepthDataListener : public royale::IDepthDataListener
    {
        /**
        * Logs the time_stamp of the received depth data.
        * The user can use the time_stamp to better understand the
        * order of the received depth data.
        */
        void onNewData (const royale::DepthData *data) override
        {
            std::cout << "received depth data with time_stamp: " << data->timeStamp.count() << '\n';
        };
    };
}

/**
* This sample shows how to properly load an RRF-File into royale and control its playback.
*/
int main (int argc, char *argv[])
{
    // We receive our RRF-File from the command line.
    //
    if (argc < 2)
    {
        std::cerr << "Wrong usage of this sample! Please pass an RRF-File as first parameter.\n";
        return -1;
    }
    auto rrf_file = argv[1];

    // This represents the main camera device object.
    //
    std::unique_ptr<royale::ICameraDevice> camera;
    {
        // The camera manager can be created locally.
        // It is only used to create a camera device object
        // and can be destroyed afterwards.
        //
        royale::CameraManager manager{};
        camera = manager.createCamera (rrf_file);
    }


    // We have to check if the camera device was created successfully.
    //
    if (camera == nullptr)
    {
        std::cerr << "Can not create the camera! This may be caused by passing a bad RRF-File.\n";
        return -2;
    }

    // Before the camera device is ready we have to invoke initialize on it.
    //
    if (camera->initialize() != royale::CameraStatus::SUCCESS)
    {
        std::cerr << "Camera can not be initialized!\n";
        return -3;
    }

    // Now that the camera is ready we create our IDepthDataListener
    // and register it to the camera device.
    //
    MyDepthDataListener depth_data_listener{};
    camera->registerDataListener (&depth_data_listener);

    // As we know that the camera device was created using a RRF-File
    // we can expect that the underlying object implements IReplay.
    // To use this interface we can dynamic_cast the object to an IReplay object.
    //
    auto replay = dynamic_cast<royale::IReplay *> (camera.get());
    if (replay == nullptr)
    {
        std::cerr << "Can not cast the camera into an IReplay object!\n";
        return -4;
    }

    // Now that we have access to the IReplay interface we can register
    // our IReplayStopListener.
    //
    MyPlaybackStopListener playback_stop_listener{};
    replay->registerStopListener (&playback_stop_listener);

    // In the next step we set the current
    // frame of the replay object to its last one.
    // We also set the replay not to loop its frames.
    // This means that only the last frame will be played when the camera starts capturing.
    //
    auto frame_count = replay->frameCount();
    replay->seek (frame_count - 1);
    replay->loop (false);

    // Here starts the first demonstrating part of this sample.
    // If the replay behaves as we defined, the camera
    // will only capture one depth data before it stops capturing.
    //
    std::cout << "start playback...\n";
    camera->startCapture();
    std::this_thread::sleep_for (std::chrono::seconds{ 6 });
    camera->stopCapture();
    std::cout << "stopped playback.\n";

    // In the next step we set the current
    // frame of the replay object to its last one.
    // We also set the replay to loop its frames.
    // This means that the play will start at the same
    // frame as before but this time the replay will loop its content.
    //
    replay->seek (frame_count - 1);
    replay->loop (true);

    // Here starts the second demonstrating part of this sample.
    // If the replay behaves as we defined, the camera
    // will capture multiple depth data.
    //
    // We also pause and resume the playback here after 2 and 4 seconds.
    // Therefor the camera should capture depth data in the first two seconds,
    // then stop capture data for two seconds and
    // then continue capture data for two more seconds.
    //
    std::cout << "start playback...\n";
    camera->startCapture();
    std::this_thread::sleep_for (std::chrono::seconds{ 2 });
    replay->pause();
    std::this_thread::sleep_for (std::chrono::seconds{ 2 });
    replay->resume();
    std::this_thread::sleep_for (std::chrono::seconds{ 2 });
    camera->stopCapture();
    std::cout << "stopped playback.\n";

    // In the next step we set the current frame of the
    // replay object to its first one.
    // We also set the replay not to use timestamps.
    // This has the effect that the Replay will not try to
    // send the frames with the original fps but as fast as possible.
    //
    replay->seek (0);
    replay->useTimestamps (false);

    // Here starts the third demonstrating part of this sample.
    // If the replay behaves as we defined, the camera
    // will capture multiple depth data much faster as in the demonstration before.
    //
    std::cout << "start playback...\n";
    camera->startCapture();
    std::this_thread::sleep_for (std::chrono::seconds{ 2 });
    camera->stopCapture();
    std::cout << "stopped playback.\n";

    return 0;
}
