#!/bin/bash

# Authors: Ratnesh Kumbhkar
#

# control_c()
# {
#   echo "exiting"
#   exit $?
# }
# 
# trap control_c SIGINT
# 
# if test ! -f ../../../../waf ; then
#     echo "please run this program from within the directory `dirname $0`, like this:"
#     echo "cd `dirname $0`"
#     echo "./`basename $0`"
#     exit 1
# fi
# 
#outputDir=`pwd`/results
outputDir=~/results$(date +%Y%m%d%H%M%S)
if [ -d $outputDir ] && [ "$1" != "-a" ]; then
    echo "$outputDir directory exists; exiting!"
    echo ""
    echo "Pass the '-a' option if you want to append; else move results/ out of the way"
    echo ""
    exit 1
else
    mkdir -p "${outputDir}"
fi
# Random number generator seed
RngRun=21

# Which node number in the scenario to enable logging on
# logNodeId=6

#######################################################################

# Transport protocol (Ftp, Tcp, or Udp).  Udp corresponds to full buffer.
transport=Udp
udpRate=200000Kbps # Kbps

# TXOP duration (ms) for LAA
lbtTxop=2

# Choose the topology; 0 (single), 1(7 UE along side)
topology=3

# FTP lambda, higher value = faster arrival
ftpLambda=2.5

# Simulation duration (seconds); scaled below
base_duration=10
duration=$(echo "$base_duration/$ftpLambda" | bc)

# Rule to update the contention window; (all, any, nacks10, nacks80)
cwUpdateRule=any

cwFactor=8

# Sensing mode to use ('Ene', Enects, Pre, Prects)
ChannelSenseMode=Ene

# chooose which WiFi standard to use (St80211n,St80211a,St80211nFull)
wifiStandard=St80211n

#######################################################################

# Enable voice instead of FTP on two UEs
#voiceEnabled=1

# Enlarge wifi queue size to accommodate FTP UDP file bursts (packets)
wifiQueueMaxSize=2000

# Set to value '0' for LAA SISO, '2' for LAA MIMO
laaTxMode=2

# Log 
logTxops=1

# Log backoff changes
logBackoffChanges=1

logBeaconArrivals=1

# Log contention window changes
logCwChanges=1

logDatatx=1

# Log Phy arrivals
logPhyArrivals=1

# Log Phy transmissions
logTxPhyArrivals=1

# Log HARQ feedback each subframe
logHarqFeedback=true

# Generate radio environment map, does not run the simulation
generateRem=false

asciiEnabled=true

serverStartTimeSeconds=2
clientStartTimeSeconds=2


#cp `pwd`/wifi-lte-laa-test.sh "$outputDir"
# need this as otherwise waf won't find the executables
cd ../../../../


#./waf --run laa-wifi-outdoor --command="%s --cellConfigA=Lte --cellConfigB=Wifi --logPhyArrivals=1 --transport=Udp  --duration=3 --logTxops=1 --logCwChanges=1 --logBackoffChanges=1 --RngRun=10 --outputDir=${outputDir} --generateRem=true"

#for location in 100; do
for location in 1 20 40 60 80 100 120; do
#for location in 1 20 40 60; do
#for location in 80 100 120; do
    #for RngRun in 13; do
    for RngRun in 11 13 21 45 95 65 25 33 89 57; do
        for laaEdThreshold in -62; do
            #for cell in Laa; do
            #for cell in Laa Wifi; do
            for cell in Wifi; do
                ./waf --run laa-wifi-indoor --command="%s  --cellConfigB=Wifi --cellConfigA=${cell} --ChannelSenseMode=${ChannelSenseMode} --topology=${topology} --cwFactor=${cwFactor} --location=${location} --lbtTxop=${lbtTxop} --logPhyArrivals=${logPhyArrivals} --logTxPhyArrivals=${logTxPhyArrivals} --transport=${transport} --udpPacketSize=1028 --udpRate=${udpRate}  --duration=${duration} --logTxops=${logTxops} --logCwChanges=${logCwChanges} --logBackoffChanges=${logBackoffChanges} --logHarqFeedback=${logHarqFeedback} --RngRun=${RngRun} --outputDir=${outputDir}
                --generateRem=${generateRem} --asciiEnabled=${asciiEnabled}  --ftpLambda=${ftpLambda} --cwUpdateRule=${cwUpdateRule} --laaEdThreshold=${laaEdThreshold} --serverStartTimeSeconds=${serverStartTimeSeconds} --wifiQueueMaxSize=${wifiQueueMaxSize} --ns3::LteEnbRrc::DefaultTransmissionMode=${laaTxMode} --logWifiRetries=1 --logWifiFailRetries=1 --logDataTx=1 --logBeaconArrivals=1" 
            done
        done
    done
done


