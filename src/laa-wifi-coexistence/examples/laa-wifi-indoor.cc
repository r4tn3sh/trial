/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 * Copyright (c) 2015 University of Washington
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Nicola Baldo <nbaldo@cttc.es> and Tom Henderson <tomh@tomh.org>
 */

//
//  From TR36.889-011 Study on LAA:
//  "Two operators deploy 4 small cells each in the single-floor building.
//  The small cells of each operator are equally spaced and centered along the
//  shorter dimension of the building. The distance between two closest nodes 
//  from two operators is random. The set of small cells for both operators 
//  is centered along the longer dimension of the building."
// 
//   +--------------------120 m---------------------+
//   |                                              |
//   |                                              |
//   |                                              |
//   50m     x o      x o          x o      x o     |
//   |                                              |
//   |                                              |
//   |                                              |
//   +----------------------------------------------+
// 
//  where 'x' and 'o' denote the small cell center points for the two operators.
// 
//  In Wi-Fi, the 'x' and 'o' correspond to access points.
// 
//  We model also N UEs (STAs) associated with each cell.  N defaults to 10.
//
//  The UEs (STAs) move around within the bounding box at a speed of 3 km/h.
//  In general, the program can be configured at run-time by passing

//  command-line arguments.  The command
//  ./waf --run "laa-wifi-indoor --help"
//  will display all of the available run-time help options, and in
//  particular, the command
//  ./waf --run "laa-wifi-indoor --PrintGlobals" should
//  display the following:
//
// Global values:
//     --ChecksumEnabled=[false]
//         A global switch to enable all checksums for all protocols
//     --RngRun=[1]
//         The run number used to modify the global seed
//     --RngSeed=[1]
//         The global seed of all rng streams
//     --SchedulerType=[ns3::MapScheduler]
//         The object class to use as the scheduler implementation
//     --SimulatorImplementationType=[ns3::DefaultSimulatorImpl]
//         The object class to use as the simulator implementation
//     --cellConfigA=[Wifi]
//         Lte or Wifi
//     --cellConfigB=[Wifi]
//         Lte or Wifi
//     --duration=[10]
//         Data transfer duration (seconds)
//     --pcapEnabled=[false]
//         Whether to enable pcap trace files for Wi-Fi
//     --transport=[Udp]
//         whether to use Udp or Tcp
//
//  The bottom five are specific to this example.  
//
//  In addition, some other variables may be modified at compile-time.
//  For simplification, an IEEE 802.11n configuration is used with the 
//  MinstrelWifi (with the 802.11a basic rates) is used.
//
//  The following sample output is provided.
//
//  When run with no arguments:
//
//  ./waf --run "laa-wifi-indoor"
//  Running simulation for 10 sec of data transfer; 50 sec overall
//  Number of cells per operator: 1; number of UEs per cell: 2
// Flow 1 (100.0.0.1 -> 100.0.0.2)
//   Tx Packets: 12500
//   Tx Bytes:   12850000
//   TxOffered:  10.28 Mbps
//   Rx Bytes:   12850000
//   Throughput: 10.0609 Mbps
//   Mean delay:  215.767 ms
//   Mean jitter:  0.277617 ms
//   Rx Packets: 12500
// Flow 2 (100.0.0.1 -> 100.0.0.3)
//   Tx Packets: 12500
//   Tx Bytes:   12850000
//   TxOffered:  10.28 Mbps
//   Rx Bytes:   6134076
//   Throughput: 4.80292 Mbps
//   Mean delay:  211.56 ms
//   Mean jitter:  0.140135 ms
//   Rx Packets: 5967
// 
// In addition, the program outputs various statistics files from the
// available LTE and Wi-Fi traces, including, for Wi-Fi, some statistics
// patterned after the 'athstats' tool, and for LTE, the stats
// described in the LTE Module documentation, which can be found at
// https://www.nsnam.org/docs/models/html/lte-user.html#simulation-output
//
// These files are named:
//  athstats-ap_002_000   
//  athstats-sta_003_000  
//  DlMacStats.txt
//  DlPdcpStats.txt
//  DlRlcStats.txt 
//  DlRsrpSinrStats.txt   
//  DlRxPhyStats.txt      
//  DlTxPhyStats.txt      
//  UlInterferenceStats.txt         
//  UlRxPhyStats.txt
//  UlSinrStats.txt
//  UlTxPhyStats.txt
//  etc.

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/mobility-module.h>
#include <ns3/scenario-helper.h>
#include <ns3/laa-wifi-coexistence-helper.h>
#include <ns3/lbt-access-manager.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("LaaWifiIndoor");

// Global Values are used in place of command line arguments so that these
// values may be managed in the ns-3 ConfigStore system.
static ns3::GlobalValue g_topology ("topology",
        "Topology choices",
        ns3::DoubleValue (0),
        ns3::MakeDoubleChecker<double> (0.0,10.0));

static ns3::GlobalValue g_cwFactor ("cwFactor",
        "Contenetion window update multiplier",
        ns3::UintegerValue (2),
        ns3::MakeUintegerChecker<uint32_t> (0,16));

static ns3::GlobalValue g_channelSenseMode ("ChannelSenseMode",
        "Ene, Enects, Pre, or Prects",
        ns3::EnumValue (LbtAccessManager::ENE),
        ns3::MakeEnumChecker (ns3::LbtAccessManager::ENE, "Ene",
            ns3::LbtAccessManager::ENECTS, "Enects",
            ns3::LbtAccessManager::PRE, "Pre",
            ns3::LbtAccessManager::PRECTS, "Prects"));
static ns3::GlobalValue g_location ("location",
        "Client location xaxis (meters)",
        ns3::DoubleValue (1),
        ns3::MakeDoubleChecker<double> (0.0, 120.0));
static ns3::GlobalValue g_duration ("duration",
        "Data transfer duration (seconds)",
        ns3::DoubleValue (2),
        ns3::MakeDoubleChecker<double> ());

static ns3::GlobalValue g_cellConfigA ("cellConfigA",
        "Lte, Wifi, or Laa",
        ns3::EnumValue (WIFI),
        ns3::MakeEnumChecker (WIFI, "Wifi",
            LTE, "Lte",
            LAA, "Laa"));
static ns3::GlobalValue g_cellConfigB ("cellConfigB",
        "Lte, Wifi, or Laa",
        ns3::EnumValue (WIFI),
        ns3::MakeEnumChecker (WIFI, "Wifi",
            LTE, "Lte",
            LAA, "Laa"));

static ns3::GlobalValue g_channelAccessManager ("ChannelAccessManager",
        "Default, DutyCycle, Lbt",
        ns3::EnumValue (Lbt),
        ns3::MakeEnumChecker (Default, "Default",
            DutyCycle, "DutyCycle",
            Lbt, "Lbt"));

static ns3::GlobalValue g_lbtTxop ("lbtTxop",
        "TxOp for LBT devices (ms)",
        ns3::DoubleValue (8.0),
        ns3::MakeDoubleChecker<double> (1.0, 20.0));

static ns3::GlobalValue g_useReservationSignal ("useReservationSignal",
        "Defines whether reservation signal will be used when used channel access manager at LTE eNb",
        ns3::BooleanValue (true),
        ns3::MakeBooleanChecker ());

static ns3::GlobalValue g_laaEdThreshold ("laaEdThreshold",
        "CCA-ED threshold for channel access manager (dBm)",
        ns3::DoubleValue (-72.0),
        ns3::MakeDoubleChecker<double> (-100.0, -50.0));

static ns3::GlobalValue g_pcap ("pcapEnabled",
        "Whether to enable pcap trace files for Wi-Fi",
        ns3::BooleanValue (false),
        ns3::MakeBooleanChecker ());

static ns3::GlobalValue g_ascii ("asciiEnabled",
        "Whether to enable ascii trace files for Wi-Fi",
        ns3::BooleanValue (false),
        ns3::MakeBooleanChecker ());

static ns3::GlobalValue g_transport ("transport",
        "whether to use 3GPP Ftp, Udp, or Tcp",
        ns3::EnumValue (UDP),
        ns3::MakeEnumChecker (FTP, "Ftp",
            UDP, "Udp",
            TCP, "Tcp"));

static ns3::GlobalValue g_lteDutyCycle ("lteDutyCycle",
        "Duty cycle value to be used for LTE",
        ns3::DoubleValue (1),
        ns3::MakeDoubleChecker<double> (0.0, 1.0));

static ns3::GlobalValue g_bsSpacing ("bsSpacing",
        "Spacing (in meters) between the closest two base stations of different operators",
        ns3::DoubleValue (5),
        ns3::MakeDoubleChecker<double> (1.0, 12.5));

static ns3::GlobalValue g_bsCornerPlacement ("bsCornerPlacement",
        "Rather than place base stations along axis according to TR36.889, place in corners instead",
        ns3::BooleanValue (false),
        ns3::MakeBooleanChecker ());


// Higher lambda means faster arrival rate; values [0.5, 1, 1.5, 2, 2.5]
// recommended
static ns3::GlobalValue g_ftpLambda ("ftpLambda",
        "Lambda value for FTP model 1 application",
        ns3::DoubleValue (0.5),
        ns3::MakeDoubleChecker<double> ());

static ns3::GlobalValue g_voiceEnabled ("voiceEnabled",
        "Whether to enable voice",
        ns3::BooleanValue (false),
        ns3::MakeBooleanChecker ());

static ns3::GlobalValue g_generateRem ("generateRem",
        "if true, will generate a REM and then abort the simulation;"
        "if false, will run the simulation normally (without generating any REM)",
        ns3::BooleanValue (false),
        ns3::MakeBooleanChecker ());

static ns3::GlobalValue g_simTag ("simTag",
        "tag to be appended to output filenames to distinguish simulation campaigns",
        ns3::StringValue ("default"),
        ns3::MakeStringChecker ());

static ns3::GlobalValue g_outputDir ("outputDir",
        "directory where to store simulation results",
        ns3::StringValue ("./"),
        ns3::MakeStringChecker ());

static ns3::GlobalValue g_cwUpdateRule ("cwUpdateRule",
        "Rule that will be used to update contention window of LAA node",
        ns3::EnumValue (LbtAccessManager::NACKS_80_PERCENT),
        ns3::MakeEnumChecker (ns3::LbtAccessManager::ALL_NACKS, "all",
            ns3::LbtAccessManager::ANY_NACK, "any",
            ns3::LbtAccessManager::NACKS_10_PERCENT, "nacks10",
            ns3::LbtAccessManager::NACKS_80_PERCENT, "nacks80"));

    int
main (int argc, char *argv[])
{
    // Effectively disable ARP cache entries from timing out
    Config::SetDefault ("ns3::ArpCache::AliveTimeout", TimeValue (Seconds (10000)));
    CommandLine cmd;
    cmd.Parse (argc, argv);

    // LogComponentEnable("LbtAccessManager", LOG_LEVEL_ALL);
    // LogComponentEnable("LbtAccessManager", LOG_LEVEL_INFO);
    // LogComponentEnable("ChannelAccessManager", LOG_LEVEL_ALL);
    // LogComponentEnable("LteHelper", LOG_LEVEL_ALL);
    // LogComponentEnable("MacLow", LOG_LEVEL_ALL);
    // LogComponentEnable("WifiRemoteStationManager", LOG_LEVEL_ALL);
    // LogComponentEnable("LteUePhy", LOG_LEVEL_ALL);
    // LogComponentEnable("LteHarqPhy", LOG_LEVEL_ALL);
    // LogComponentEnable("ScenarioHelper", LOG_LEVEL_ALL);
    // LogComponentEnable("SpectrumWifiPhy", LOG_LEVEL_ALL);
    // LogComponentEnable("LteEnbPhy", LOG_LEVEL_ALL);
    // LogComponentEnable("PfFfMacScheduler", LOG_LEVEL_ALL);
    // LogComponentEnable("LteEnbPhy", LOG_LEVEL_DEBUG);
    // LogComponentEnable("ApWifiMac", LOG_LEVEL_ALL);
    // LogComponentEnable("RegularWifiMac", LOG_LEVEL_FUNCTION);
    // LogComponentEnable("IdealWifiManager", LOG_LEVEL_FUNCTION);
    // LogComponentEnable("InterferenceHelper", LOG_LEVEL_ALL);
    // LogComponentEnable("Backoff", LOG_LEVEL_ALL);
    // LogComponentEnable("DcfManager", LOG_LEVEL_FUNCTION);
    // LogComponentEnable("DcaTxop", LOG_LEVEL_ALL);
    // LogComponentEnable("EdcaTxopN", LOG_LEVEL_ALL);

    // Some debugging settings not exposed as command line args are below

    // To disable application data flow (e.g. for visualizing mobility
    // in the visualizer), set below to true
    bool disableApps = false;

    // Extract global values into local variables.  
    EnumValue enumValue;
    DoubleValue doubleValue;
    BooleanValue booleanValue;
    StringValue stringValue;
    UintegerValue uintegerValue;

    GlobalValue::GetValueByName ("topology", doubleValue);
    double topology = doubleValue.Get ();
    GlobalValue::GetValueByName ("cwFactor", uintegerValue);
    uint32_t cwFactor = uintegerValue.Get ();
    GlobalValue::GetValueByName ("ChannelSenseMode", enumValue);
    enum LbtAccessManager::ChannelSenseMode_t ChannelSenseMode = (LbtAccessManager::ChannelSenseMode_t)enumValue.Get ();
    GlobalValue::GetValueByName ("location", doubleValue);
    double location = doubleValue.Get ();
    GlobalValue::GetValueByName ("ChannelAccessManager", enumValue);
    enum Config_ChannelAccessManager channelAccessManager = (Config_ChannelAccessManager) enumValue.Get ();
    GlobalValue::GetValueByName ("cellConfigA", enumValue);
    enum Config_e cellConfigA = (Config_e) enumValue.Get ();
    GlobalValue::GetValueByName ("cellConfigB", enumValue);
    enum Config_e cellConfigB = (Config_e) enumValue.Get ();
    GlobalValue::GetValueByName ("lbtTxop", doubleValue);
    double lbtTxop = doubleValue.Get ();
    GlobalValue::GetValueByName ("laaEdThreshold", doubleValue);
    double laaEdThreshold = doubleValue.Get ();
    GlobalValue::GetValueByName ("duration", doubleValue);
    double duration = doubleValue.Get ();
    Time durationTime = Seconds (duration);
    GlobalValue::GetValueByName ("transport", enumValue);
    enum Transport_e transport = (Transport_e) enumValue.Get ();
    GlobalValue::GetValueByName ("lteDutyCycle", doubleValue);
    double lteDutyCycle = doubleValue.Get ();
    GlobalValue::GetValueByName ("bsSpacing", doubleValue);
    double bsSpacing = doubleValue.Get ();
    GlobalValue::GetValueByName ("bsCornerPlacement", booleanValue);
    bool bsCornerPlacement = booleanValue.Get ();
    GlobalValue::GetValueByName ("generateRem", booleanValue);
    bool generateRem = booleanValue.Get ();
    GlobalValue::GetValueByName ("simTag", stringValue);
    std::string simTag = stringValue.Get ();
    GlobalValue::GetValueByName ("outputDir", stringValue);
    std::string outputDir = stringValue.Get ();
    GlobalValue::GetValueByName ("useReservationSignal", booleanValue);
    bool useReservationSignal = booleanValue.Get ();
    GlobalValue::GetValueByName ("cwUpdateRule", enumValue);
    enum  LbtAccessManager::CWUpdateRule_t cwUpdateRule = (LbtAccessManager::CWUpdateRule_t) enumValue.Get ();

    GlobalValue::GetValueByName ("asciiEnabled", booleanValue);
    if (booleanValue.Get () == true)
    {
        PacketMetadata::Enable ();
    }

    // This program has two operators, and nominally 4 cells per operator
    // and 5 UEs per cell.  These variables can be tuned below for
    // e.g. debugging on a smaller scale scenario
    // uint32_t numCells = 4;
    // uint32_t numUePerCell = 5;
        std::cout << "***************************" << cwFactor << std::endl;

    uint32_t numCellsA;
    uint32_t numUePerCellA;
    uint32_t numCellsB;
    uint32_t numUePerCellB;
    if (topology >= 1 && topology <=5)
    {
        numCellsA = 1;
        numUePerCellA = 7;
        numCellsB = 1;
        numUePerCellB = 1;
    }
    else if (topology == 6 || topology == 7)
    {
        numCellsA = 1;
        numUePerCellA = 7;
        numCellsB = 1;
        numUePerCellB = 7;
    }
    else if (topology == 8)
    {
        numCellsA = 2;
        numUePerCellA = 7;
        numCellsB = 1;
        numUePerCellB = 2;
    }
    else if (topology == 9)
    {
        numCellsA = 1;
        numUePerCellA = 10;
        numCellsB = 1;
        numUePerCellB = 2;
    }
    else if (topology == 0)
    {
        numCellsA = 1;
        numUePerCellA = 1;
        numCellsB = 1;
        numUePerCellB = 1;
    }



    Config::SetDefault ("ns3::ChannelAccessManager::EnergyDetectionThreshold", DoubleValue (laaEdThreshold));
    switch (channelAccessManager)
    {
        case Lbt:
            Config::SetDefault ("ns3::LaaWifiCoexistenceHelper::ChannelAccessManagerType", StringValue ("ns3::LbtAccessManager"));
            Config::SetDefault ("ns3::LbtAccessManager::Txop", TimeValue (Seconds (lbtTxop/1000.0)));
            Config::SetDefault ("ns3::LbtAccessManager::UseReservationSignal", BooleanValue(useReservationSignal));
            Config::SetDefault ("ns3::LbtAccessManager::CwUpdateRule", EnumValue(cwUpdateRule));
            Config::SetDefault ("ns3::LbtAccessManager::CwFactor", UintegerValue(cwFactor));
            Config::SetDefault ("ns3::LbtAccessManager::ChannelSenseMode", EnumValue(ChannelSenseMode));
            break;
        case DutyCycle:
            Config::SetDefault ("ns3::LaaWifiCoexistenceHelper::ChannelAccessManagerType", StringValue ("ns3::DutyCycleAccessManager"));
            Config::SetDefault ("ns3::DutyCycleAccessManager::OnDuration", TimeValue (MilliSeconds (60)));
            Config::SetDefault ("ns3::DutyCycleAccessManager::OnStartTime",TimeValue (MilliSeconds (0)));
            Config::SetDefault ("ns3::DutyCycleAccessManager::DutyCyclePeriod",TimeValue (MilliSeconds (80)));
            break;
        default:
            //default LTE channel access manager will be used, LTE always transmits
            break;
    } 

    UintegerValue rtslimit = 100000; // Setting it to such high value to disable all possible RTS/CTS
    Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", rtslimit);
    // When logging, use prefixes
    LogComponentEnableAll (LOG_PREFIX_TIME);
    LogComponentEnableAll (LOG_PREFIX_FUNC);
    LogComponentEnableAll (LOG_PREFIX_NODE);

    //
    // Topology setup phase
    //

    // Allocate 4 BS for nodes
    NodeContainer bsNodesA;
    bsNodesA.Create (numCellsA);
    NodeContainer bsNodesB;
    bsNodesB.Create (numCellsB);

    //
    // Create bounding box 120m x 50m

    // BS mobility helper
    MobilityHelper mobilityBs;
    if (bsCornerPlacement == false)
    {
        // // TOPOLOGY 1
        // // Place operator A's BS at coordinates (20,25), (45,25), (70,25), (95,25)
        // mobilityBs.SetPositionAllocator ("ns3::GridPositionAllocator",
        //                                 "MinX", DoubleValue (20),
        //                                 "MinY", DoubleValue (25),
        //                                 "DeltaX", DoubleValue (25),
        //                                 "LayoutType", StringValue ("RowFirst"));
        // mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
        // mobilityBs.Install (bsNodesA);
        // // The offset between the base stations of operators A and B is governed
        // // by the global value bsSpacing;  
        // // Place operator B's BS at coordinates (20 + bsSpacing ,25), (45,25), (70,25), (95,25)
        // mobilityBs.SetPositionAllocator ("ns3::GridPositionAllocator",
        //                                 "MinX", DoubleValue (20 + bsSpacing),
        //                                 "MinY", DoubleValue (25),
        //                                 "DeltaX", DoubleValue (25),
        //                                 "LayoutType", StringValue ("RowFirst"));
        // mobilityBs.Install (bsNodesB);

        //TODO: Genrealize the allocation - Ratnesh
        // // TOPOLOGY 2
        // // Place operator A's BS at coordinates (50,25)
        // mobilityBs.SetPositionAllocator ("ns3::GridPositionAllocator",
        //                                 "MinX", DoubleValue (50),
        //                                 "MinY", DoubleValue (25),
        //                                 "DeltaX", DoubleValue (25),
        //                                 "LayoutType", StringValue ("RowFirst"));
        // mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
        // mobilityBs.Install (bsNodesA);
        // // The offset between the base stations of operators A and B is governed
        // // by the global value bsSpacing;  
        // // Place BS at the corners (0,0), (120,0), (0,50), (120,50)
        // mobilityBs.SetPositionAllocator ("ns3::GridPositionAllocator",
        //                                 "MinX", DoubleValue (0),
        //                                 "MinY", DoubleValue (0),
        //                                 "GridWidth", UintegerValue (2),
        //                                 "DeltaX", DoubleValue (120),
        //                                 "DeltaY", DoubleValue (50),
        //                                 "LayoutType", StringValue ("RowFirst"));
        // mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
        // mobilityBs.Install (bsNodesB);
        // bsSpacing=bsSpacing; 

        // // TOPOLOGY 3
        // // Place operator A's BS at coordinates (50,25)
        // mobilityBs.SetPositionAllocator ("ns3::GridPositionAllocator",
        //                                 "MinX", DoubleValue (50),
        //                                 "MinY", DoubleValue (25),
        //                                 "DeltaX", DoubleValue (30),
        //                                 "LayoutType", StringValue ("RowFirst"));
        // mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
        // mobilityBs.Install (bsNodesA);
        // // The offset between the base stations of operators A and B is governed
        // // by the global value bsSpacing;  
        // // Place BS at the corners (0,0), (120,0), (0,50), (120,50)
        // mobilityBs.SetPositionAllocator ("ns3::GridPositionAllocator",
        //                                 "MinX", DoubleValue (35),
        //                                 "MinY", DoubleValue (25),
        //                                 "DeltaX", DoubleValue (30),
        //                                 "LayoutType", StringValue ("RowFirst"));
        // mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
        // mobilityBs.Install (bsNodesB);

        // // TOPOLOGY 4
        // // Place operator A's BS at coordinates (50,25)
        // mobilityBs.SetPositionAllocator ("ns3::GridPositionAllocator",
        //                                 "MinX", DoubleValue (0),
        //                                 "MinY", DoubleValue (0),
        //                                 "DeltaX", DoubleValue (30),
        //                                 "LayoutType", StringValue ("RowFirst"));
        // mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
        // mobilityBs.Install (bsNodesA);
        // // The offset between the base stations of operators A and B is governed
        // // by the global value bsSpacing;  
        // // Place BS at the corners (0,0), (120,0), (0,50), (120,50)
        // mobilityBs.SetPositionAllocator ("ns3::GridPositionAllocator",
        //                                 "MinY", DoubleValue (50),
        //                                 "MinX", DoubleValue (0),
        //                                 "DeltaX", DoubleValue (120),
        //                                 "LayoutType", StringValue ("RowFirst"));
        // mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
        // mobilityBs.Install (bsNodesB);


        // // TOPOLOGY 5
        // // Place operator A's BS at coordinates (50,25)
        // mobilityBs.SetPositionAllocator ("ns3::GridPositionAllocator",
        //                                 "MinX", DoubleValue (0),
        //                                 "MinY", DoubleValue (0),
        //                                 "DeltaX", DoubleValue (30),
        //                                 "LayoutType", StringValue ("RowFirst"));
        // mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
        // mobilityBs.Install (bsNodesA);
        // // The offset between the base stations of operators A and B is governed
        // // by the global value bsSpacing;  
        // // Place BS at the corners (0,0), (120,0), (0,50), (120,50)
        // mobilityBs.SetPositionAllocator ("ns3::GridPositionAllocator",
        //                                 "MinY", DoubleValue (50),
        //                                 "MinX", DoubleValue (120),
        //                                 "DeltaX", DoubleValue (120),
        //                                 "LayoutType", StringValue ("RowFirst"));
        // mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
        // mobilityBs.Install (bsNodesB);

        // TOPOLOGY 6
        if (topology == 0 || topology == 1)
        {
            mobilityBs.SetPositionAllocator ("ns3::GridPositionAllocator",
                    "MinX", DoubleValue (0),
                    "MinY", DoubleValue (0),
                    "DeltaX", DoubleValue (30),
                    "LayoutType", StringValue ("RowFirst"));
            mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
            mobilityBs.Install (bsNodesA);
            
            mobilityBs.SetPositionAllocator ("ns3::GridPositionAllocator",
                    "MinY", DoubleValue (50),
                    "MinX", DoubleValue (0),
                    "DeltaX", DoubleValue (120),
                    "LayoutType", StringValue ("RowFirst"));
            mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
            mobilityBs.Install (bsNodesB);


            bsSpacing=bsSpacing;
        }
        else if (topology == 2)
        {
            mobilityBs.SetPositionAllocator ("ns3::GridPositionAllocator",
                    "MinX", DoubleValue (50),
                    "MinY", DoubleValue (0),
                    "DeltaX", DoubleValue (30),
                    "LayoutType", StringValue ("RowFirst"));
            mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
            mobilityBs.Install (bsNodesA);
            
            mobilityBs.SetPositionAllocator ("ns3::GridPositionAllocator",
                    "MinY", DoubleValue (25),
                    "MinX", DoubleValue (0),
                    "DeltaX", DoubleValue (120),
                    "LayoutType", StringValue ("RowFirst"));
            mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
            mobilityBs.Install (bsNodesB);
        }
        else if (topology == 3)
        {
            mobilityBs.SetPositionAllocator ("ns3::GridPositionAllocator",
                    "MinX", DoubleValue (60),
                    "MinY", DoubleValue (0),
                    "DeltaX", DoubleValue (30),
                    "LayoutType", StringValue ("RowFirst"));
            mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
            mobilityBs.Install (bsNodesA);
            
            mobilityBs.SetPositionAllocator ("ns3::GridPositionAllocator",
                    "MinX", DoubleValue (60),
                    "MinY", DoubleValue (50),
                    "DeltaX", DoubleValue (120),
                    "LayoutType", StringValue ("RowFirst"));
            mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
            mobilityBs.Install (bsNodesB);
        }
        else if (topology == 4 || topology == 5 || topology == 6 || topology == 7)
        {
            mobilityBs.SetPositionAllocator ("ns3::GridPositionAllocator",
                    "MinX", DoubleValue (0),
                    "MinY", DoubleValue (25),
                    "DeltaX", DoubleValue (30),
                    "LayoutType", StringValue ("RowFirst"));
            mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
            mobilityBs.Install (bsNodesA);
            
            mobilityBs.SetPositionAllocator ("ns3::GridPositionAllocator",
                    "MinX", DoubleValue (120),
                    "MinY", DoubleValue (25),
                    "DeltaX", DoubleValue (120),
                    "LayoutType", StringValue ("RowFirst"));
            mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
            mobilityBs.Install (bsNodesB);
        }
        else if (topology == 8)
        {
            Ptr<ListPositionAllocator> positionBS1 = CreateObject<ListPositionAllocator> ();
            Ptr<ListPositionAllocator> positionBS2 = CreateObject<ListPositionAllocator> ();
            Ptr<ListPositionAllocator> positionAP1 = CreateObject<ListPositionAllocator> ();
            
            positionBS1->Add (Vector (50, 0, 0.0));	// set position of BS 1
            mobilityBs.SetPositionAllocator (positionBS1);
            mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
            mobilityBs.Install (bsNodesA.Get(0));

            positionBS2->Add (Vector (50, 50, 0.0));	// set position of BS 1
            mobilityBs.SetPositionAllocator (positionBS2);
            mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
            mobilityBs.Install (bsNodesA.Get(1));

            positionAP1->Add (Vector (0, 25, 0.0));	// set position of AP 1
            mobilityBs.SetPositionAllocator (positionAP1);
            mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
            mobilityBs.Install (bsNodesB.Get(0));
        }
        else if (topology == 9)
        {
            Ptr<ListPositionAllocator> positionBS1 = CreateObject<ListPositionAllocator> ();
            Ptr<ListPositionAllocator> positionAP1 = CreateObject<ListPositionAllocator> ();
            // Ptr<ListPositionAllocator> positionAP2 = CreateObject<ListPositionAllocator> ();

            positionBS1->Add (Vector (0, 50, 0.0));	// set position of BS 1
            mobilityBs.SetPositionAllocator (positionBS1);
            mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
            mobilityBs.Install (bsNodesA.Get(0));
            
            positionAP1->Add (Vector (120, 0, 0.0));	// set position of AP 1
            mobilityBs.SetPositionAllocator (positionAP1);
            mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
            mobilityBs.Install (bsNodesB.Get(0));

            // positionAP2->Add (Vector (120, 50, 0.0));	// set position of AP 2
            // mobilityBs.SetPositionAllocator (positionAP2);
            // mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
            // mobilityBs.Install (bsNodesB.Get(1));
        }
    }
    else
    {
        // Place BS at the corners (0,0), (120,0), (0,50), (120,50)
        mobilityBs.SetPositionAllocator ("ns3::GridPositionAllocator",
                "MinX", DoubleValue (0),
                "MinY", DoubleValue (0),
                "GridWidth", UintegerValue (2),
                "DeltaX", DoubleValue (120),
                "DeltaY", DoubleValue (50),
                "LayoutType", StringValue ("RowFirst"));
        mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
        mobilityBs.Install (bsNodesA);
        // place BS at the corners (1,1), (119,1), (1,49), (119,49)
        mobilityBs.SetPositionAllocator ("ns3::GridPositionAllocator",
                "MinX", DoubleValue (1),
                "MinY", DoubleValue (1),
                "GridWidth", UintegerValue (2),
                "DeltaX", DoubleValue (118),
                "DeltaY", DoubleValue (48),
                "LayoutType", StringValue ("RowFirst"));
        mobilityBs.Install (bsNodesB);
    }

    // Allocate UE for each cell
    NodeContainer ueNodesA;
    ueNodesA.Create (numUePerCellA * numCellsA);
    NodeContainer ueNodesB;
    ueNodesB.Create (numUePerCellB * numCellsB);

    // UE mobility helper
    MobilityHelper mobilityUe;
    if (topology==1)
    {
        Ptr<ListPositionAllocator> positionUE1 = CreateObject<ListPositionAllocator> ();
        Ptr<ListPositionAllocator> positionUE2 = CreateObject<ListPositionAllocator> ();
        Ptr<ListPositionAllocator> positionUE3 = CreateObject<ListPositionAllocator> ();
        Ptr<ListPositionAllocator> positionUE4 = CreateObject<ListPositionAllocator> ();
        Ptr<ListPositionAllocator> positionUE5 = CreateObject<ListPositionAllocator> ();
        Ptr<ListPositionAllocator> positionUE6 = CreateObject<ListPositionAllocator> ();
        Ptr<ListPositionAllocator> positionUE7 = CreateObject<ListPositionAllocator> ();
        Ptr<ListPositionAllocator> positionClient1 = CreateObject<ListPositionAllocator> ();
        positionUE1->Add (Vector (0,23, 0.0));	// set position of UE
	    mobilityUe.SetPositionAllocator (positionUE1);
	    mobilityUe.Install (ueNodesA.Get(0));

        positionUE2->Add (Vector (20,23, 0.0));	// set position of UE
	    mobilityUe.SetPositionAllocator (positionUE2);
	    mobilityUe.Install (ueNodesA.Get(1));

        positionUE3->Add (Vector (40,23, 0.0));	// set position of UE
	    mobilityUe.SetPositionAllocator (positionUE2);
	    mobilityUe.Install (ueNodesA.Get(2));

        positionUE4->Add (Vector (60,23, 0.0));	// set position of UE
	    mobilityUe.SetPositionAllocator (positionUE4);
	    mobilityUe.Install (ueNodesA.Get(3));

        positionUE5->Add (Vector (80,23, 0.0));	// set position of UE
	    mobilityUe.SetPositionAllocator (positionUE2);
	    mobilityUe.Install (ueNodesA.Get(4));

        positionUE6->Add (Vector (100,23, 0.0));	// set position of UE
	    mobilityUe.SetPositionAllocator (positionUE2);
	    mobilityUe.Install (ueNodesA.Get(5));

        positionUE7->Add (Vector (120,23, 0.0));	// set position of UE
	    mobilityUe.SetPositionAllocator (positionUE4);
	    mobilityUe.Install (ueNodesA.Get(6));

        
        positionClient1->Add (Vector (location,25, 0.0));	// set position of STA
	    mobilityUe.SetPositionAllocator (positionClient1);
	    mobilityUe.Install (ueNodesB.Get(0));
    }
    else if (topology == 2)
    {
        Ptr<ListPositionAllocator> positionClient1 = CreateObject<ListPositionAllocator> ();
        std::ostringstream xs;
        xs << "ns3::UniformRandomVariable[Min=0.0|Max=" << location << "]";
        mobilityUe.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
                                       "X", StringValue (xs.str()),
                                       "Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=50.0]"));
        mobilityUe.Install (ueNodesA);

        positionClient1->Add (Vector (20,20, 0.0));	// set position of STA
	    mobilityUe.SetPositionAllocator (positionClient1);
	    mobilityUe.Install (ueNodesB.Get(0));
        //mobilityUe.Install (ueNodesB);
    }
    else if (topology == 3 || topology == 4)
    {
        Ptr<ListPositionAllocator> positionClient1 = CreateObject<ListPositionAllocator> ();
        std::ostringstream xs;
        xs << "ns3::UniformRandomVariable[Min="<<60-location/2<<"|Max=" << 60+location/2 << "]";
        mobilityUe.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
                                       "X", StringValue (xs.str()),
                                       "Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=50.0]"));
        mobilityUe.Install (ueNodesA);

        positionClient1->Add (Vector (60,25, 0.0));	// set position of STA
	    mobilityUe.SetPositionAllocator (positionClient1);
	    mobilityUe.Install (ueNodesB.Get(0));
        //mobilityUe.Install (ueNodesB);
    }
    else if (topology == 5)
    {
        Ptr<ListPositionAllocator> positionClient1 = CreateObject<ListPositionAllocator> ();
        std::ostringstream xs;
        xs << "ns3::UniformRandomVariable[Min="<<17*location/20<<"|Max=" << 17+17*location/20 << "]"; // XXX: will cross 120 boundry
        mobilityUe.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
                                       "X", StringValue (xs.str()),
                                       "Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=50.0]"));
        mobilityUe.Install (ueNodesA);

        positionClient1->Add (Vector (60,25, 0.0));	// set position of STA
	    mobilityUe.SetPositionAllocator (positionClient1);
	    mobilityUe.Install (ueNodesB.Get(0));
        //mobilityUe.Install (ueNodesB);
    }
    else if (topology == 6)
    {
        Ptr<ListPositionAllocator> positionClient1 = CreateObject<ListPositionAllocator> ();
        std::ostringstream xs;
        xs << "ns3::UniformRandomVariable[Min="<<60-location/2<<"|Max=" << 60+location/2 << "]";
        mobilityUe.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
                                       "X", StringValue (xs.str()),
                                       "Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=50.0]"));
        mobilityUe.Install (ueNodesA);

        mobilityUe.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
                                       "X", StringValue ("ns3::UniformRandomVariable[Min=80.0|Max=90.0]"),
                                       "Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=50.0]"));
	    mobilityUe.Install (ueNodesB);
        //mobilityUe.Install (ueNodesB);
    }
    else if (topology==8)
    {
        Ptr<ListPositionAllocator> positionUE1 = CreateObject<ListPositionAllocator> ();
        Ptr<ListPositionAllocator> positionUE2 = CreateObject<ListPositionAllocator> ();
        Ptr<ListPositionAllocator> positionUE3 = CreateObject<ListPositionAllocator> ();
        Ptr<ListPositionAllocator> positionUE4 = CreateObject<ListPositionAllocator> ();
        Ptr<ListPositionAllocator> positionUE5 = CreateObject<ListPositionAllocator> ();
        Ptr<ListPositionAllocator> positionUE6 = CreateObject<ListPositionAllocator> ();
        Ptr<ListPositionAllocator> positionUE7 = CreateObject<ListPositionAllocator> ();
        Ptr<ListPositionAllocator> positionClient1 = CreateObject<ListPositionAllocator> ();
        Ptr<ListPositionAllocator> positionClient2 = CreateObject<ListPositionAllocator> ();

        positionUE1->Add (Vector (location, 14, 0.0));	// set position of UE
	    mobilityUe.SetPositionAllocator (positionUE1);
	    mobilityUe.Install (ueNodesA.Get(0));

        positionUE2->Add (Vector (location,32, 0.0));	// set position of UE
	    mobilityUe.SetPositionAllocator (positionUE2);
	    mobilityUe.Install (ueNodesA.Get(1));

        positionUE3->Add (Vector (50,25, 0.0));	// set position of UE
	    mobilityUe.SetPositionAllocator (positionUE3);
	    mobilityUe.Install (ueNodesA.Get(2));

        positionUE4->Add (Vector (100,25, 0.0));	// set position of UE
	    mobilityUe.SetPositionAllocator (positionUE4);
	    mobilityUe.Install (ueNodesA.Get(3));

        positionUE5->Add (Vector (100,40, 0.0));	// set position of UE
	    mobilityUe.SetPositionAllocator (positionUE5);
	    mobilityUe.Install (ueNodesA.Get(4));

        positionUE6->Add (Vector (110,0, 0.0));	// set position of UE
	    mobilityUe.SetPositionAllocator (positionUE6);
	    mobilityUe.Install (ueNodesA.Get(5));

        positionUE7->Add (Vector (120,23, 0.0));	// set position of UE
	    mobilityUe.SetPositionAllocator (positionUE7);
	    mobilityUe.Install (ueNodesA.Get(6));


        std::ostringstream xs;
        xs << "ns3::UniformRandomVariable[Min="<<17*location/20<<"|Max=" << 17+17*location/20 << "]"; // XXX: will cross 120 boundry
        mobilityUe.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
                                       "X", StringValue (xs.str()),
                                       "Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=50.0]"));
	    mobilityUe.Install (ueNodesA.Get(7));
	    mobilityUe.Install (ueNodesA.Get(8));
	    mobilityUe.Install (ueNodesA.Get(9));
	    mobilityUe.Install (ueNodesA.Get(10));
	    mobilityUe.Install (ueNodesA.Get(11));
	    mobilityUe.Install (ueNodesA.Get(12));
	    mobilityUe.Install (ueNodesA.Get(13));
        
        positionClient1->Add (Vector (40,25, 0.0));	// set position of STA
	    mobilityUe.SetPositionAllocator (positionClient1);
	    mobilityUe.Install (ueNodesB.Get(0));

        
        positionClient2->Add (Vector (30,25, 0.0));	// set position of STA
	    mobilityUe.SetPositionAllocator (positionClient2);
	    mobilityUe.Install (ueNodesB.Get(1));
    }
    else if (topology==9)
    {
        Ptr<ListPositionAllocator> positionUE1 = CreateObject<ListPositionAllocator> ();
        Ptr<ListPositionAllocator> positionUE2 = CreateObject<ListPositionAllocator> ();
        Ptr<ListPositionAllocator> positionClient1 = CreateObject<ListPositionAllocator> ();
        Ptr<ListPositionAllocator> positionClient2 = CreateObject<ListPositionAllocator> ();

        // positionUE1->Add (Vector (70, 36, 0.0));	// set position of UE
	    // mobilityUe.SetPositionAllocator (positionUE1);
	    // mobilityUe.Install (ueNodesA.Get(0));
        // positionUE2->Add (Vector (70, 44, 0.0));	// set position of UE
	    // mobilityUe.SetPositionAllocator (positionUE2);
	    // mobilityUe.Install (ueNodesA.Get(1));
        
        std::ostringstream xs;
        mobilityUe.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
                                       "X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=10.0]"),
                                       "Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=50.0]"));
	    for (uint32_t i = 0; i<numUePerCellA/2; i++)
            mobilityUe.Install (ueNodesA.Get(i));

        mobilityUe.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
                                       "X", StringValue ("ns3::UniformRandomVariable[Min=90.0|Max=100.0]"),
                                       "Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=50.0]"));
	    for (uint32_t i = numUePerCellA/2; i<numUePerCellA; i++)
            mobilityUe.Install (ueNodesA.Get(i));
	    // mobilityUe.Install (ueNodesA.Get(2));
	    // mobilityUe.Install (ueNodesA.Get(3));
	    // mobilityUe.Install (ueNodesA.Get(4));
	    // mobilityUe.Install (ueNodesA.Get(5));
	    // mobilityUe.Install (ueNodesA.Get(6));
	    // mobilityUe.Install (ueNodesA.Get(7));
	    // mobilityUe.Install (ueNodesA.Get(8));
	    // mobilityUe.Install (ueNodesA.Get(9));
	    // mobilityUe.Install (ueNodesA.Get(10));
	    // mobilityUe.Install (ueNodesA.Get(11));
	    // mobilityUe.Install (ueNodesA.Get(12));
	    // mobilityUe.Install (ueNodesA.Get(13));
	    // mobilityUe.Install (ueNodesA.Get(14));

        positionClient1->Add (Vector (120,28, 0.0));	// set position of STA
	    mobilityUe.SetPositionAllocator (positionClient1);
	    mobilityUe.Install (ueNodesB.Get(0));

        
        positionClient2->Add (Vector (120,15, 0.0));	// set position of STA
	    mobilityUe.SetPositionAllocator (positionClient2);
	    mobilityUe.Install (ueNodesB.Get(1));

        //positionClient1->Add (Vector (71,17, 0.0));	// set position of STA
	    //mobilityUe.SetPositionAllocator (positionClient1);
	    //mobilityUe.Install (ueNodesB.Get(2));

        //
        //positionClient2->Add (Vector (73,25, 0.0));	// set position of STA
	    //mobilityUe.SetPositionAllocator (positionClient2);
	    //mobilityUe.Install (ueNodesB.Get(3));
    }
    else if (topology == 0)
    {
        mobilityUe.SetPositionAllocator ("ns3::GridPositionAllocator",
                "MinX", DoubleValue (0),
                "MinY", DoubleValue (25),
                "DeltaX", DoubleValue (location),
                "LayoutType", StringValue ("RowFirst"));
        mobilityUe.Install (ueNodesA);
        mobilityUe.Install (ueNodesB);
    }
    mobilityUe.SetMobilityModel ("ns3::ConstantPositionMobilityModel");


    // REM settings tuned to get a nice figure for this specific scenario
    Config::SetDefault ("ns3::RadioEnvironmentMapHelper::OutputFile", StringValue ("laa-wifi-indoor.rem"));
    Config::SetDefault ("ns3::RadioEnvironmentMapHelper::XMin", DoubleValue (0));
    Config::SetDefault ("ns3::RadioEnvironmentMapHelper::XMax", DoubleValue (120));
    Config::SetDefault ("ns3::RadioEnvironmentMapHelper::YMin", DoubleValue (0));
    Config::SetDefault ("ns3::RadioEnvironmentMapHelper::YMax", DoubleValue (50));
    Config::SetDefault ("ns3::RadioEnvironmentMapHelper::XRes", UintegerValue (1200));
    Config::SetDefault ("ns3::RadioEnvironmentMapHelper::YRes", UintegerValue (500));
    Config::SetDefault ("ns3::RadioEnvironmentMapHelper::Z", DoubleValue (1.5));

    std::ostringstream simulationParams;
    simulationParams << "";

    // Specify some physical layer parameters that will be used in scenario helper
    PhyParams phyParams;

    if (topology<=3 || topology == 8)
    {
        phyParams.m_bsTxGain = 5; // dB antenna gain
        phyParams.m_bsRxGain = 0; // dB antenna gain
        phyParams.m_bsTxPower = 14; // dBm
        phyParams.m_bsNoiseFigure = 5; // dB
        phyParams.m_ueTxGain = 0; // dB antenna gain
        phyParams.m_ueRxGain = 0; // dB antenna gain
        phyParams.m_ueTxPower = 14; // dBm
        phyParams.m_ueNoiseFigure = 9; // dB
    }
    else if (topology == 9)
    {
        phyParams.m_bsTxGain = 5; // dB antenna gain
        phyParams.m_bsRxGain = 0; // dB antenna gain
        phyParams.m_bsTxPower = 14; // dBm
        phyParams.m_bsNoiseFigure = 5; // dB
        phyParams.m_ueTxGain = 0; // dB antenna gain
        phyParams.m_ueRxGain = 0; // dB antenna gain
        phyParams.m_ueTxPower = 14; // dBm
        phyParams.m_ueNoiseFigure = 9; // dB
    }
    else if(topology == 4 || topology == 5 || topology == 6)
    {
        phyParams.m_bsTxGain = 5; // dB antenna gain
        phyParams.m_bsRxGain = 0; // dB antenna gain
        phyParams.m_bsTxPower = 24; // dBm
        phyParams.m_bsNoiseFigure = 5; // dB
        phyParams.m_ueTxGain = 0; // dB antenna gain
        phyParams.m_ueRxGain = 0; // dB antenna gain
        phyParams.m_ueTxPower = 24; // dBm
        phyParams.m_ueNoiseFigure = 9; // dB
    }

    ConfigureAndRunScenario (cellConfigA, cellConfigB, bsNodesA, bsNodesB, ueNodesA, ueNodesB, phyParams, durationTime, transport, "ns3::Ieee80211axIndoorPropagationLossModel", disableApps, lteDutyCycle, generateRem, outputDir + "/laa_wifi_indoor_" + simTag, simulationParams.str ());

    return 0;
}
