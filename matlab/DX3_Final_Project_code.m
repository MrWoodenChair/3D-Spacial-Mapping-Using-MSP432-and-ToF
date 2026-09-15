%% =========================================
%% Real-Time 3D Scan 
%% =========================================

clear;

%% SERIAL CONFIGURATION
port     = "COM4";       % change to your COM port
baudrate = 115200;

device = serialport(port, baudrate);
device.Timeout = 50;
configureTerminator(device, "CR/LF");
flush(device);

fprintf("Opened %s\n", port);

%% PARAMETERS
%Change if doing less scans
num_layers = 3;             % total scans/layers expected
points_per_rotation = 32;   % measurements per full rotation
max_points = num_layers * points_per_rotation;

% Preallocate arrays
X = nan(max_points,1);   % x_layer
Y = nan(max_points,1);   % y_radial
Z = nan(max_points,1);   % z_radial
Angles = nan(max_points,1);
Distances = nan(max_points,1);

point_idx = 0;

%% REAL-TIME PLOT SETUP
figure;
hold on; grid on;
xlabel('X (Layers)'); ylabel('Y'); zlabel('Z (Radial Distance)');
title('Real-Time Scan (Scatter + Wireframe)');
axis equal;
view(3);

hScatter = scatter3(0,0,0,10,'filled');
hLines = gobjects(0);

%% WAIT FOR START
disp("Waiting for START...");
while true
    if device.NumBytesAvailable > 0
        line = strtrim(readline(device));
        if line == "START"
            disp("Scan started!");
            break;
        end
    end
end

%% COLLECT DATA (REAL-TIME)
disp("Collecting points...");
while true
    if device.NumBytesAvailable == 0
        pause(0.001); % small pause to prevent CPU hogging
        continue;
    end

    line = strtrim(readline(device));
    if line == "END"
        disp("Scan complete!");
        break;
    end

    data = sscanf(line, "%f, %f, %f");
    if numel(data) ~= 3
        continue;
    end

    point_idx = point_idx + 1;

    virtual_z = data(1);
    angle     = data(2);
    distance  = data(3);

    % Convert polar to Cartesian
    x_layer = virtual_z;
    y_radial = distance * sind(angle);
    z_radial = distance * cosd(angle);

    % Store in preallocated arrays
    X(point_idx) = x_layer;
    Y(point_idx) = y_radial;
    Z(point_idx) = z_radial;
    Angles(point_idx) = angle;
    Distances(point_idx) = distance;

    % Update scatter
    set(hScatter, 'XData', X(1:point_idx), ...
                  'YData', Y(1:point_idx), ...
                  'ZData', Z(1:point_idx));
end

%% CLEANUP SERIAL
clear device;
disp("Serial port closed.");

%% =========================================
%% FINAL WIREFRAME (CLEAN)
%% =========================================
figure;
hold on; grid on;
scatter3(X(1:point_idx), Y(1:point_idx), Z(1:point_idx), 15, 'filled');

uniqueX = unique(X(1:point_idx));

%Connect points in a layer
for k = 1:length(uniqueX)
    idx = find(X(1:point_idx) == uniqueX(k));
    [~, order] = sort(Angles(idx));
    idx = idx(order);

    % Draw circle
    plot3(X(idx), Y(idx), Z(idx), 'k-');
    plot3([X(idx(end)), X(idx(1))], [Y(idx(end)), Y(idx(1))], [Z(idx(end)), Z(idx(1))], 'k-');
end

% Connect layers
for k = 2:length(uniqueX)
    idx1 = find(X(1:point_idx) == uniqueX(k-1));
    idx2 = find(X(1:point_idx) == uniqueX(k));
    [~, o1] = sort(Angles(idx1));
    [~, o2] = sort(Angles(idx2));
    idx1 = idx1(o1); idx2 = idx2(o2);

    for i = 1:min(length(idx1), length(idx2))
        plot3([X(idx1(i)), X(idx2(i))], [Y(idx1(i)), Y(idx2(i))], [Z(idx1(i)), Z(idx2(i))], 'k-');
    end
end

title('Final Wireframe');
xlabel('X (Layers)'); ylabel('Y'); zlabel('Z (Radial Distance)');
axis equal; grid on; view(3);
