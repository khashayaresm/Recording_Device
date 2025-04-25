% Define the filename (adjust path if necessary)
filename = 'putty1.log';

% Read all lines of the file into a string array
lines = readlines(filename);

% Preallocate an empty array for numeric data
data = [];

% Loop over each line and try to convert it to a numeric value.
for k = 1:length(lines)
    numVal = str2double(lines(k));
    if ~isnan(numVal)
        data(end+1) = numVal; %#ok<SAGROW>
    end
end

% Plot the data (using the index as the x-axis)
figure;
plot(data, 'b.-'); % blue line with markers
xlabel('Index');
ylabel('Value');
title(['Plot of Data from ', filename], 'Interpreter','none');
grid on;
