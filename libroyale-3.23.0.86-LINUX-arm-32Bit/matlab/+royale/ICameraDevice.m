classdef ICameraDevice < handle
    properties (Hidden = true, SetAccess = private)
        h
    end
    methods
        function obj=ICameraDevice(handle)
            obj.h=handle;
        end
        function delete(obj)
            royale.royale_matlab(obj,'delete');
        end

        %% LEVEL 1
        function initialize(obj,settings)
            if (nargin == 1)
                royale.royale_matlab(obj,'initialize');
            else
                royale.royale_matlab(obj,'initialize',settings);
            end
        end
        function cameraId = getId(obj)
            cameraId = royale.royale_matlab(obj,'getId');
        end
        function setUseCase(obj, mode)
            royale.royale_matlab(obj,'setUseCase',mode);
        end
        function mode = getCurrentUseCase(obj)
            mode = royale.royale_matlab(obj,'getCurrentUseCase');
        end
        function modes = getUseCases(obj)
            modes = royale.royale_matlab(obj,'getUseCases');
        end
        function streams = getStreams(obj)
            streams = royale.royale_matlab(obj,'getStreams');
        end
        function numberOfStreams = getNumberOfStreams(obj, useCase)
            if (nargin == 1)
                numberOfStreams = royale.royale_matlab(obj,'getNumberOfStreams');
            else
                numberOfStreams = royale.royale_matlab(obj,'getNumberOfStreams', useCase);
            end
        end
        function CameraType = getCameraName(obj)
            CameraType = royale.royale_matlab(obj,'getCameraName');
        end
        function CameraInfo = getCameraInfo(obj)
            CameraInfo = royale.royale_matlab(obj,'getCameraInfo');
        end
        function setExposureTime(obj, varargin)
            %SETEXPOSURETIME set exposure time in microseconds (us)
            %   SETEXPOSURETIME(exposureTime) sets the exposure time
            %   SETEXPOSURETIME(exposureTime, streamId) sets the exposure time of the specified stream
            %   SETEXPOSURETIME(exposureGroup, exposureTime) sets the exposure time of the specified exposure group
            if (nargin == 2)
                royale.royale_matlab(obj,...
                    'setExposureTime', uint32(varargin{1}));
            elseif (nargin == 3)
                if isnumeric(varargin{1})
                    royale.royale_matlab(obj,...
                        'setExposureTime', uint32(varargin{1}), uint16(varargin{2}))
                elseif ischar(varargin{1})
                    royale.royale_matlab(obj,...
                        'setExposureTime', varargin{1}, uint32(varargin{2}))
                else
                    error('Unexpected parameters');
                end
            else
                error('Unexpected parameter count');
            end
        end
        function setExposureTimes(obj, exposureTimes)
            royale.royale_matlab(obj,...
                'setExposureTimes', uint32(exposureTimes));
        end
        function setExposureForGroups(obj, exposureTimes)
            royale.royale_matlab(obj,...
                'setExposureForGroups', uint32(exposureTimes));
        end
        function setExposureMode(obj, exposureMode, streamId)
            if ~isa(exposureMode, 'royale.ExposureMode')
                error('invalid argument');
            end
            if (nargin == 2)
                royale.royale_matlab(obj,...
                    'setExposureMode', uint16(exposureMode));
            else
                royale.royale_matlab(obj,...
                    'setExposureMode', uint16(exposureMode), uint16(streamId));
            end
        end
        function exposureMode = getExposureMode(obj, streamId)
            if (nargin == 1)
                exposureMode = royale.ExposureMode(...
                    royale.royale_matlab(obj,'getExposureMode'));
            else
                exposureMode = royale.ExposureMode(...
                    royale.royale_matlab(obj,'getExposureMode', uint16(streamId)));
            end
        end
        function startCapture(obj)
            royale.royale_matlab(obj,'startCapture');
        end
        function stopCapture(obj)
            royale.royale_matlab(obj,'stopCapture');
        end
        function startRecording(obj,fileName,numberOfFrames)
            if (nargin == 2)
                royale.royale_matlab(obj,'startRecording',fileName);
            elseif (nargin == 3)
                royale.royale_matlab(obj,'startRecording',...
                    fileName, uint32(numberOfFrames));
            elseif (nargin == 4)
                royale.royale_matlab(obj,'startRecording',...
                    fileName, uint32(numberOfFrames), uint32(frameSkip));
            elseif (nargin == 5)
                royale.royale_matlab(obj,'startRecording',...
                    fileName, uint32(numberOfFrames), uint32(frameSkip), uint32(msSkip));
            else
                error('Unexpected number of input variables')
            end
        end
        function truefalse = isRecording(obj)
            truefalse = royale.royale_matlab(obj,'isRecording');
        end
        function stopRecording(obj)
            royale.royale_matlab(obj,'stopRecording');
        end
        function width = getMaxSensorWidth(obj)
            width = royale.royale_matlab(obj,'getMaxSensorWidth');
        end
        function height = getMaxSensorHeight(obj)
            height = royale.royale_matlab(obj,'getMaxSensorHeight');
        end
        function LensParameters = getLensParameters(obj)
            LensParameters = royale.royale_matlab(obj,'getLensParameters');
        end
        function truefalse = isConnected(obj)
            truefalse = royale.royale_matlab(obj,'isConnected');
        end
        function truefalse = isCapturing(obj)
            truefalse = royale.royale_matlab(obj,'isCapturing');
        end
        function [DepthData, IntermediateData, RawData] = getData(obj)
            switch nargout
                case 0
                    royale.royale_matlab(obj,'getData');
                case 1
                    DepthData = royale.royale_matlab(obj,'getData');
                case 2
                    [DepthData, IntermediateData] = royale.royale_matlab(obj,'getData');
                case 3
                    [DepthData, IntermediateData, RawData] = royale.royale_matlab(obj,'getData');
                otherwise
                    error('Unexpected number of output variables')
            end
        end
        function CameraAccessLevel = getAccessLevel(obj)
            CameraAccessLevel = royale.royale_matlab(obj,'getAccessLevel');
        end
        function setExternalTrigger(obj, useExternalTrigger)
            royale.royale_matlab(obj,'setExternalTrigger',useExternalTrigger);
        end
        function setFrameRate(obj, framerate)
            royale.royale_matlab(obj,'setFrameRate',framerate);
        end

        function setFilterLevel(obj, filterLevel, streamId)
            if ~isa(filterLevel, 'royale.FilterLevel')
                error('invalid argument');
            end
            if (nargin == 2)
                royale.royale_matlab(obj,...
                    'setFilterLevel', uint16(filterLevel));
            else
                royale.royale_matlab(obj,...
                    'setFilterLevel', uint16(filterLevel), uint16(streamId));
            end
        end
        function filterLevel = getFilterLevel(obj, streamId)
            if (nargin == 1)
                filterLevel = royale.FilterLevel(...
                    royale.royale_matlab(obj,'getFilterLevel'));
            else
                filterLevel = royale.FilterLevel(...
                    royale.royale_matlab(obj,'getFilterLevel', uint16(streamId)));
            end
        end

        function expoLimits = getExposureLimits(obj, streamId)
            if (nargin == 1)
                expoLimits = royale.royale_matlab(obj,'getExposureLimits');
            else
                expoLimits = royale.royale_matlab(obj,'getExposureLimits', uint16(streamId));
            end
        end
        
        
        %% Playback functionality
        function count = frameCount(obj)
            count = royale.royale_matlab(obj, 'frameCount');
        end
        function seek(obj,frameNumber)
            royale.royale_matlab(obj, 'seek', uint32(frameNumber));
        end
        function loop(obj,onoff)
            royale.royale_matlab(obj, 'loop', onoff);
        end
        function useTimestamps(obj, truefalse)
            royale.royale_matlab(obj, 'useTimestamps', truefalse);
        end
        function truefalse = freerun(obj, truefalse)
            if (nargin == 1)
                truefalse = royale.royale_matlab(obj, 'freerun');
            else
                royale.royale_matlab(obj, 'freerun', truefalse);
            end
        end

        %% Matlab only functionality
        function setMatlabTimeout(obj, tx)
            royale.royale_matlab(obj, 'setMatlabTimeout', uint32(tx));
        end

        %% LEVEL 2
        function exposureGroups = getExposureGroups(obj)
            exposureGroups = royale.royale_matlab(obj,'getExposureGroups');
        end
        function setProcessingParameters(obj, parameters, streamId)
            if (nargin == 2)
                royale.royale_matlab(obj,'setProcessingParameters',parameters);
            else
                royale.royale_matlab(obj,'setProcessingParameters',parameters, uint16(streamId));
            end
        end
        function parameters = getProcessingParameters(obj, streamId)
            if (nargin == 1)
                parameters = royale.royale_matlab(obj,'getProcessingParameters');
            else
                parameters = royale.royale_matlab(obj,'getProcessingParameters', uint16(streamId));
            end
        end
        function setCallbackData(obj, cbData)
            % possible values for cbData:
            %   royale.CallbackData.Raw   (no processing)
            %   royale.CallbackData.Depth (default)
            if ~isa(cbData, 'royale.CallbackData')
                error('invalid argument');
            end
            royale.royale_matlab(obj,'setCallbackData', uint16(cbData));
        end
        function setCalibrationData(obj, filename)
            royale.royale_matlab(obj,'setCalibrationData', filename);
        end
        function calibData = getCalibrationData(obj)
            calibData = royale.royale_matlab(obj,'getCalibrationData');
        end
        function writeCalibrationToFlash(obj)
            royale.royale_matlab(obj,'writeCalibrationToFlash');
        end

        %% LEVEL 3
        function writeRegister(obj, RegisterName, RegisterValue)
            royale.royale_matlab(obj,'writeRegister', RegisterName, RegisterValue);
        end
        function value = readRegister(obj, RegisterAddress)
            value = royale.royale_matlab(obj,'readRegister', RegisterAddress);
        end
        function setDutyCycle(obj, percent, iSeq)
            royale.royale_matlab(obj, 'setDutyCycle', percent, int16(iSeq));
        end

        function shiftLensCenter(obj, tx, ty)
            royale.royale_matlab(obj,'shiftLensCenter', int16(tx), int16(ty));
        end
        function [x, y] = getLensCenter(obj)
            [x, y] = royale.royale_matlab(obj,'getLensCenter');
        end

        function writeDataToFlash(obj, filename)
            royale.royale_matlab(obj,'writeDataToFlash', filename);
        end
    end
end