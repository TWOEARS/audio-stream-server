clear a;
[data, ind, lost] = dedicatedSocketRead(4000, ind);
a = data.output;
pause(2);
for(i=1:5)
   [data, ind, lost] = dedicatedSocketRead(4000, ind);
   a = [a; data.output];
   pause(2);
end