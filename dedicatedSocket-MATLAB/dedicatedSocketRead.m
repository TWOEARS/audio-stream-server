function [data, newInd, lost] = dedicatedSocketRead(oldInd)
%function [data] = dedicatedSocketRead(port)
global dedicatedSocket;

%fprintf(dedicatedSocket.p, strcat('Read Port:', {' '}, num2str(port), '-', ' ', 'Index', ' ', num2str(oldInd)));
fprintf(dedicatedSocket.p, sprintf('Read Port - Index %d', oldInd));

rate = dedicatedSocket.captureConfig.transfer_rate;
periodSize = (dedicatedSocket.captureConfig.transfer_rate*dedicatedSocket.captureConfig.chunk_time)/1000;
nbFrames = ((dedicatedSocket.captureConfig.transfer_rate*dedicatedSocket.captureConfig.chunk_time)/1000)*dedicatedSocket.captureConfig.Port_chunks;
nbBlocks = dedicatedSocket.captureConfig.Port_chunks;
        

%Wait for the first four bytes which contain the number of blocks to read.
while(dedicatedSocket.p.BytesAvailable<4)
end
message=fread(dedicatedSocket.p, 4);
dedicatedSocket.bytes = (message(1) + message(2)*256 + message(3)*65536 + message(4)*16777216)*2*4*periodSize+4;

while(dedicatedSocket.p.BytesAvailable<dedicatedSocket.bytes)
end
message=fread(dedicatedSocket.p, dedicatedSocket.bytes);
[audio.left, audio.right, audio.output, audio.index] = conversionBinaryMex(message);

newInd = audio.index;
blocksAvailable = newInd - oldInd;

if (blocksAvailable == 0)
    data = 0;
    lost = -1;
    return;
end

lost = 0;
if (blocksAvailable > nbBlocks)
    lost = blocksAvailable-nbBlocks;
    disp([num2str(lost), ' blocks have been lost']);
    blocksAvailable = nbBlocks;
end

%framesAvailable = blocksAvailable*periodSize;
framesAvailable=length(audio.left);
data.left = zeros(framesAvailable, 1);
data.right = zeros(framesAvailable, 1);
data.output = zeros(framesAvailable, 2);

%data.left = audio.left((end-framesAvailable+1):end);
%data.right = audio.right((end-framesAvailable+1):end);
%data.output(:, 1) = audio.output((end-framesAvailable+1):end,1);
%data.output(:, 2) = audio.output((end-framesAvailable+1):end,2);
data.left = audio.left;
data.right = audio.right;
data.output(:, 1) = audio.output(:, 1);
data.output(:, 2) = audio.output(:, 2);
end