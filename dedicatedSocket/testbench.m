function [req, read, parse, times] = testbench(ip, reads)
    global dedicatedSocket; 

    %---Test Averages times for Request, Read and Parse---%
    %Open the connection
    dedicatedSocket.p = tcpip(ip,8081);     
    dedicatedSocket.p.InputBufferSize = 44100*4*2*4+4;
    fopen(dedicatedSocket.p);
    audio.index=0;
    periodSize = 2205;
    port=500;
    %Read something to have comunication before testing
    for(i=1:10)
        fprintf(dedicatedSocket.p, sprintf('Read Port: %d - Index %d',port, audio.index));
        %Wait for the first four bytes which contain the number of blocks to read.
        while(dedicatedSocket.p.BytesAvailable<4)
        end
        message=fread(dedicatedSocket.p, 4);
        dedicatedSocket.bytes = (message(1) + message(2)*256 + message(3)*65536 + message(4)*16777216)*2*4*periodSize+4;

        while(dedicatedSocket.p.BytesAvailable<dedicatedSocket.bytes)
        end
        message=fread(dedicatedSocket.p, dedicatedSocket.bytes);
        [audio.left, audio.right, audio.output, audio.index] = conversionBinaryMex(message);
        pause(0.5);
    end
    req = zeros(3,reads);
    read = zeros(3,reads);
    parse = zeros(3,reads);
    pause(5);
    %Test
    for(i=1:3)
        switch i
            case 2 %Port 500
                port = 500;
                time = 0.5;
            case 1 %Port 1000
                port = 1000;
                time = 1;
            case 3 %Port 4000
                port = 4000;
                time = 4;
        end
        for(j=1:reads)
            a = tic;
            fprintf(dedicatedSocket.p, sprintf('Read Port: %d - Index %d',port, audio.index));
            %Wait for the first four bytes which contain the number of blocks to read.
            while(dedicatedSocket.p.BytesAvailable<4)
            end
            message=fread(dedicatedSocket.p, 4);
            dedicatedSocket.bytes = (message(1) + message(2)*256 + message(3)*65536 + message(4)*16777216)*2*4*periodSize+4;

            while(dedicatedSocket.p.BytesAvailable<dedicatedSocket.bytes)
            end
            req(i,j) = toc(a);
            a = tic;
            message=fread(dedicatedSocket.p, dedicatedSocket.bytes);
            read(i,j) = toc(a);
            a = tic;
            [audio.left, audio.right, audio.output, audio.index] = conversionBinaryMex(message);
            parse(i,j) = toc(a);
            clear message;
            pause(time);
        end
        pause(5);
    end
    
    avg.req = mean(req,2);
    avg.read = mean(read,2);
    avg.parse = mean(parse,2);
    
    %Plot
    ports = [500, 1000, 4000];    
    figure(1);    
    plot(ports, avg.req*1000, ports, avg.read*1000, ports, avg.parse*1000, ports, avg.req*1000+avg.read*1000+avg.parse*1000)
    xlabel('Port Size');
    ylabel('Time [mS]');
    title('Port Size vs. Time');
    legend('Request', 'Read', 'Parse', 'Total');
    
    %Close the connection
    fclose(dedicatedSocket.p);  
    
    %---Test Times to read Each Port ---%
    times = zeros(3, reads);
    samples = zeros(1, reads);
    for(i=1:reads)
        samples(i) = i;
    end
    ind = audio.index;
    for(i=1:3)
        switch i
            case 1 %Port 500
                port = 500;
                time = 0,5;
            case 2 %Port 1000
                port = 1000;
                time = 1;
            case 3 %Port 4000
                port = 4000;
                time = 4;
        end
        dedicatedSocketOpen(ip);
        for(j=1:reads)
            a = tic;
            [data, ind, lost] = dedicatedSocketRead(port, ind);
            times(i, j) = toc(a); 
            pause(time);
        end
        dedicatedSocketClose();
    end
    times = times*1000;
    
    figure(2);
    plot(samples, times(1,:), samples, times(2,:), samples, times(3,:));
    xlabel('Read [N]');
    ylabel('Time [mS]');
    title('Read Number vs. Time');
    legend('Port500', 'Port1000', 'Port4000');
    
end