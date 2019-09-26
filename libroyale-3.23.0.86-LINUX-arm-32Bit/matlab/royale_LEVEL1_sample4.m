function royale_LEVEL1_sample4()
%ROYALE_LEVEL1_SAMPLE4 - royale example #4: auto exposure

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

% activate autoexposure
fprintf('* Activating AutoExposure...\n');
cameraDevice.setExposureMode(royale.ExposureMode.AUTOMATIC);

% preview camera
fprintf('* Starting preview. Close figure to exit...\n');
% start capture mode
cameraDevice.startCapture();

% initialize preview figure
UseCase = cameraDevice.getCurrentUseCase();
hFig=figure('Name',...
    ['Preview: ',cameraDevice.getId(),' @ ', UseCase],...
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
    
    ExpSuffix=[' @ ',num2str(data.exposureTimes(1),'%d'),' µs'];
    subplot(2,3,1);
    my_image(data.x,['x',ExpSuffix]);
   
    subplot(2,3,2);
    my_image(data.y,['y',ExpSuffix]);
    
    subplot(2,3,3);
    my_image(data.z,['z',ExpSuffix]);

    subplot(2,3,4);
    my_image(data.grayValue,['grayValue',ExpSuffix]);
    
    subplot(2,3,5);
    my_image(data.noise,['noise',ExpSuffix]);
    
    subplot(2,3,6);
    my_image(data.depthConfidence,['depthConfidence',ExpSuffix]);
    
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
    my_titlehandle=title(Name);
    setappdata(gca,'my_titlehandle',my_titlehandle);
    setappdata(gca,'my_imagehandle',my_imagehandle);
else
    my_titlehandle = getappdata(gca,'my_titlehandle');
    my_imagehandle = getappdata(gca,'my_imagehandle');
    set(my_titlehandle,'String',Name);
    set(my_imagehandle,'CData',CData);
end
end
