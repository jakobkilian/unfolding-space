function royale_LEVEL1_sample3()
%ROYALE_LEVEL1_SAMPLE3 - royale example #3:
% retrieve all frames from an .rrf file as fast as possible

% retrieve royale version information
royaleVersion = royale.getVersion();
fprintf('* royale version: %s\n',royaleVersion);

FileName = 'royale_LEVEL1_sample2.rrf';

% open recorded file
manager = royale.CameraManager();
cameraDevice = manager.createCamera(FileName);
delete(manager);

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

% configure playback
cameraDevice.loop(false);
cameraDevice.useTimestamps(false);

N_Frames=cameraDevice.frameCount();
fprintf('Retrieving %d frames...\n',N_Frames);

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
while (ishandle(hFig)) && (iFrame < N_Frames)
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
