function royale_LEVEL1_sample2()
%ROYALE_LEVEL1_SAMPLE2 - royale example #2: recording

% retrieve royale version information
royaleVersion = royale.getVersion();
fprintf('* royale version: %s\n',royaleVersion);

% the camera manager will query for a connected camera
manager = royale.CameraManager();
camlist = manager.getConnectedCameraList();

fprintf('* Cameras found: %d\n',numel(camlist));
cellfun(@(cameraId)...
    fprintf('    %s\n',cameraId),...
    camlist);

if (~isempty(camlist))
    % this represents the main camera device object
    cameraDevice = manager.createCamera(camlist{1});
else
    error(['Please make sure that a supported camera is plugged in, all drivers are ',...
        'installed, and you have proper USB permission']);
end

% the camera device is now available and CameraManager can be deallocated here
delete(manager);

% IMPORTANT: call the initialize method before working with the camera device
cameraDevice.initialize();

% display some information about the connected camera
fprintf('====================================\n');
fprintf('        Camera information\n');
fprintf('====================================\n');
fprintf('Id:              %s\n',cameraDevice.getId());
fprintf('Type:            %s\n',cameraDevice.getCameraName());
fprintf('Width:           %u\n',cameraDevice.getMaxSensorWidth());
fprintf('Height:          %u\n',cameraDevice.getMaxSensorHeight());

% retrieve valid use cases
UseCases=cameraDevice.getUseCases();
fprintf('Use cases: %d\n',numel(UseCases));
fprintf('    %s\n',UseCases{:});
fprintf('====================================\n');

if (numel(UseCases) == 0)
    error('No use case available');
end
    
% % set use case
% UseCase=UseCases{1};

% set use case interactively
UseCaseSelection=listdlg(...
    'Name','Operation Mode',...
    'PromptString','Choose operation mode:',...
    'ListString',UseCases,...
    'SelectionMode','single',...
    'ListSize',[200,200]);
if isempty(UseCaseSelection)
    return;
end
UseCase=UseCases{UseCaseSelection};

cameraDevice.setUseCase(UseCase);

% % change the exposure time (limited by the used operation mode [microseconds]
% fprintf('* Changing exposure time to 200 microseconds...\n');
% cameraDevice.setExposureTime(200);

% preview camera
fprintf('* Starting preview. Close figure to exit...\n');
% start capture mode
cameraDevice.startCapture();


% record 20 frames
fprintf('* Recording 20 frames...\n');
N_Frames = 20;
FileName = 'royale_LEVEL1_sample2.rrf';
cameraDevice.startRecording(FileName,N_Frames);
while (cameraDevice.isRecording())
    fprintf('Camera is recording...\n');
    pause(1);
end
fprintf('recording finished!\n');


% stop capture mode
fprintf('* Stopping capture mode...\n');
cameraDevice.stopCapture();

% close camera
delete(cameraDevice);

% open recorded file
manager = royale.CameraManager();
cameraDevice = manager.createCamera(FileName);
delete(manager);

cameraDevice.initialize();



% display some information about the connected camera
fprintf('====================================\n');
fprintf('          File information\n');
fprintf('====================================\n');
fprintf('Id:              %s\n',cameraDevice.getId());
fprintf('Type:            %s\n',cameraDevice.getCameraName());
fprintf('Width:           %u\n',cameraDevice.getMaxSensorWidth());
fprintf('Height:          %u\n',cameraDevice.getMaxSensorHeight());

% retrieve valid use cases
UseCases=cameraDevice.getUseCases();
fprintf('Use cases: %d\n',numel(UseCases));
fprintf('    %s\n',UseCases{:});
fprintf('====================================\n');

if (numel(UseCases) == 0)
    error('No use case available');
end

% start capture mode
cameraDevice.startCapture();

% initialize preview figure
hFig=figure('Name',...
    ['Preview: ',cameraDevice.getId(),' @ MODE_PLAYBACK'],...
    'IntegerHandle','off','NumberTitle','off');
colormap(jet(256));
TID = tic();
last_toc = toc(TID);
iFrame = 0;
while (ishandle(hFig))
    % retrieve data from camera
    data = cameraDevice.getData();
    
    iFrame = iFrame + 1;
    if (mod(iFrame,10) == 0)
        this_toc=toc(TID);
        fprintf('FPS = %.2f\n',10/(this_toc-last_toc));
        last_toc=this_toc;
    end
    
    %%% notice: figures are slow.
    %%% For higher FPS (e.g. 45), do not display every frame.
    %%% e.g. by doing here:
    % if (mod(iFrame,5) ~= 0);continue;end;
    
    % visualize data
    set(0,'CurrentFigure',hFig);
    
    subplot(2,3,1);
    my_image(data.x,'x');
   
    subplot(2,3,2);
    my_image(data.y,'y');
    
    subplot(2,3,3);
    my_image(data.z,'z');

    subplot(2,3,4);
    my_image(data.grayValue,'grayValue');
    
    subplot(2,3,5);
    my_image(data.noise,'noise');
    
    subplot(2,3,6);
    my_image(data.depthConfidence,'depthConfidence');
    
    drawnow;
end

% stop capture mode
fprintf('* Stopping capture mode...\n');
cameraDevice.stopCapture();

fprintf('* ...done!\n');
end

function my_image(CData,Name)
% convenience function for faster display refresh:
%  only update 'CData' on successive image calls
%  (does not update title or change resolution!)
if ~isappdata(gca,'my_imagehandle')
    my_imagehandle = imagesc(CData);
    axis image
    title(Name);
    setappdata(gca,'my_imagehandle',my_imagehandle);
else
    my_imagehandle = getappdata(gca,'my_imagehandle');
    set(my_imagehandle,'CData',CData);
end
end
