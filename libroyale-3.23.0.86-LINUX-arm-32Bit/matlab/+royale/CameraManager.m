classdef CameraManager < handle
    properties (Hidden = true, SetAccess = private)
        h
    end
    methods
        function obj=CameraManager(activationCode)
            if (nargin ~= 0)
                obj.h=royale.royale_matlab(obj, 'new', activationCode);
            else
                obj.h=royale.royale_matlab(obj, 'new');
            end
        end
        function delete(obj)
            royale.royale_matlab(obj,'delete');
        end
        function ICameraDevice=createCamera(obj,cameraId,triggerMode)
            if (nargin == 2)
                ICameraDevice = royale.ICameraDevice(...
                    royale.royale_matlab(obj,'createCamera',cameraId));
            else
                if ~isa(triggerMode, 'royale.TriggerMode')
                    error('invalid argument');
                end
                ICameraDevice = royale.ICameraDevice(...
                    royale.royale_matlab(obj,'createCamera',cameraId, uint16(triggerMode)));
            end
        end
        function cameraIds=getConnectedCameraList(obj)
            cameraIds = royale.royale_matlab(obj,'getConnectedCameraList');
        end
    end
end