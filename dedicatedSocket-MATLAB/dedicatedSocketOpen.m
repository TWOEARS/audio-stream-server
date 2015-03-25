function dedicatedSocketOpen(ip)
global dedicatedSocket;

%Get Capture Configuration:
dedicatedSocket.captureConfig = GMB_capture_GetCaptureConfig;

dedicatedSocket.p = tcpip(ip,8081);
%TODO: Don't allow multiple connections per client.     
dedicatedSocket.p.InputBufferSize = 44100*4*2*4+4;
fopen(dedicatedSocket.p); 
end