close all;
clear all;
total_ueA = 5; % per operator
total_ueB = 10; % per operator
Yaxislim = 150;
%############################################
fd = fopen('./results/laa_wifi_indoor_default_cw_log','r');
index = 1;
while true
    %# Get the current line
    ln = fgetl(fd);
    %# Stop if EOF
    if ln == -1
        break;
    endif
    elems = strsplit(ln, ' ');
    nums = str2double(elems);
    cwdata(index,:)=nums;
    index = index+1;
endwhile
fclose(fd);
%plot(cwdata(:,2),cwdata(:,3),'o')
%############################################
fd = fopen('./results/laa_wifi_indoor_default_backoff_log','r');
index = 1;
while true
    %# Get the current line
    ln = fgetl(fd);
    %# Stop if EOF
    if ln == -1
        break;
    endif
    elems = strsplit(ln, ' ');
    nums = str2double(elems);
    bodata(index,:)=nums;
    index = index+1;
endwhile
fclose(fd);
%figure;
%plot(bodata(:,2),bodata(:,3),'o')

%############################################
fd = fopen('./results/laa_wifi_indoor_default_operatorA','r');
index = 1;
while true
    %# Get the current line
    ln = fgetl(fd);
    %# Stop if EOF
    if ln == -1
        break;
    endif
    elems = strsplit(ln, ' ');
    nums = str2double(elems);
    A_tp(index,:)=nums;
    index = index+1;
endwhile
fclose(fd);
setsize = 4;
for i=1:floor(size(A_tp,1)/setsize)
    A_ue_tp = 0;
    for j=1:setsize
        A_ue_tp = A_ue_tp + A_tp(setsize*(i-1)+j,8);
    end
    A_ue_tp = A_ue_tp/setsize
end
% figure;
% bar(A_ue_tp, "facecolor", "c", "edgecolor", "b")
% set(gca, "ygrid", "on", "fontsize", 18)
% xlim([0 total_ueA+1]);
% ylim([0 Yaxislim]);
% title ("Throughput for operator A");
% xlabel ("UE/STA id");
% ylabel ("Throughput (Mbps)");
% print -dpng operatorA_tp.png;

%############################################
fd = fopen('./results/laa_wifi_indoor_default_operatorB','r');
index = 1;
while true
    %# Get the current line
    ln = fgetl(fd);
    %# Stop if EOF
    if ln == -1
        break;
    endif
    elems = strsplit(ln, ' ');
    nums = str2double(elems);
    B_tp(index,:)=nums;
    index = index+1;
endwhile
fclose(fd);
setsize = 4;
for i=1:floor(size(B_tp,1)/setsize)
    B_ue_tp = 0;
    for j=1:setsize
        B_ue_tp = B_ue_tp + B_tp(setsize*(i-1)+j,8);
    end
    B_ue_tp = B_ue_tp/setsize
end
% figure;
% bar(A_ue_tp, "facecolor", "c", "edgecolor", "b")
% set(gca, "ygrid", "on", "fontsize", 18)
% xlim([0 total_ueA+1]);
% ylim([0 Yaxislim]);
% title ("Throughput for operator A");
% xlabel ("UE/STA id");
% ylabel ("Throughput (Mbps)");
% print -dpng operatorA_tp.png;

%pause;

close all;
