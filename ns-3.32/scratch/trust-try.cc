#include <iostream>
#include <cmath>
#include <fstream>

#include "ns3/aodv-module.h"
#include "ns3/netanim-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h" 
#include "ns3/v4ping-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/csma-helper.h"
#include "ns3/trust-aodv-module.h"
#include "ns3/trust-manager-helper.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/tap-bridge-module.h"


using namespace ns3;
using namespace aodv;

NS_LOG_COMPONENT_DEFINE ("TapWifiVirtualMachineExample");

int 
main (int argc, char *argv[])
{
  CommandLine cmd (__FILE__);
  cmd.Parse (argc, argv);

  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

//Tap Nodes  
  NodeContainer nodes;
  nodes.Create (2); 

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());

  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211a);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate54Mbps"));

  WifiMacHelper wifiMac;
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, nodes);
  
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (5.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);

  AodvHelper aodv;
  InternetStackHelper stack;
  stack.SetRoutingHelper (aodv);
  stack.Install (nodes);
  
  Ipv4AddressHelper address;
  address.SetBase ("10.1.0.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  TapBridgeHelper tapBridge1 (interfaces.GetAddress (0));
  tapBridge1.SetAttribute ("Mode", StringValue ("UseLocal"));
  tapBridge1.SetAttribute ("DeviceName", StringValue ("tap-left"));
  tapBridge1.Install (nodes.Get (0), devices.Get (0));

  TapBridgeHelper tapBridge2 (interfaces.GetAddress (1));
  tapBridge2.SetAttribute ("Mode", StringValue ("UseLocal"));
  tapBridge2.SetAttribute ("DeviceName", StringValue ("tap-right"));
  tapBridge2.Install (nodes.Get (1), devices.Get (1));

//Simulated Nodes
  NodeContainer nodesSim;
  nodesSim.Create (5); 

  YansWifiPhyHelper wifiPhySim = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannelSim = YansWifiChannelHelper::Default ();
  wifiPhySim.SetChannel (wifiChannelSim.Create ());

  WifiHelper wifiSim;
  wifiSim.SetStandard (WIFI_STANDARD_80211a);
  wifiSim.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate54Mbps"));

  WifiMacHelper wifiMacSim;
  wifiMacSim.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer devicesSim = wifiSim.Install (wifiPhySim, wifiMacSim, nodesSim);
  
  MobilityHelper mobilitySim;
  Ptr<ListPositionAllocator> positionAllocSim = CreateObject<ListPositionAllocator> ();
  positionAllocSim->Add (Vector (30.0, 0.0, 0.0));
  positionAllocSim->Add (Vector (55.0, 0.0, 0.0));
  positionAllocSim->Add (Vector (80.0, 0.0, 0.0));
  positionAllocSim->Add (Vector (105.0, 0.0, 0.0));
  positionAllocSim->Add (Vector (130.0, 0.0, 0.0));
  mobilitySim.SetPositionAllocator (positionAllocSim);
  mobilitySim.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilitySim.Install (nodesSim);

  TrustAodvHelper aodvSim;
  InternetStackHelper stackSim;
  stackSim.SetRoutingHelper (aodvSim);
  stackSim.Install (nodesSim);
  
  Ipv4AddressHelper addressSim;
  addressSim.SetBase ("10.3.0.0", "255.255.255.0");
  Ipv4InterfaceContainer interfacesSim = addressSim.Assign (devicesSim);
 
// //p2p

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("512kbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("10ms"));

  NodeContainer nodesp2p = NodeContainer (nodes.Get (1), nodesSim.Get (0));
  NetDeviceContainer devicesp2p = p2p.Install (nodesp2p);

  Ipv4AddressHelper ipv4p2p;
  ipv4p2p.SetBase ("10.2.0.0", "255.255.255.192");
  Ipv4InterfaceContainer interfacesp2p = ipv4p2p.Assign (devicesp2p);

// Simulate Traffic

uint16_t port = 9;   // Discard port (RFC 863)
  OnOffHelper onoff ("ns3::UdpSocketFactory", InetSocketAddress (interfacesp2p.GetAddress (1), port));
  onoff.SetConstantRate (DataRate ("500kb/s"));

  ApplicationContainer apps = onoff.Install (nodes.Get (1));
  apps.Start (Seconds (1.0));

  // Create a packet sink to receive these packets
  PacketSinkHelper sink ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));

  apps = sink.Install (nodesSim.Get (0));
  apps.Start (Seconds (1.0));


//
  if (1)
    {
      TrustManagerHelper trustManagerHelper;
      Ptr<OutputStreamWrapper> trustRoutingStream = Create<OutputStreamWrapper> ("trust-aodv_trust_table_at_55.routes", std::ios::out);
      trustManagerHelper.PrintTrustTableAllAt (Seconds (55), trustRoutingStream);

      TrustManagerHelper trustManagerHelper3;
      Ptr<OutputStreamWrapper> trustRoutingStream3 = Create<OutputStreamWrapper> ("trust-aodv_trust_table_at_75.routes", std::ios::out);
      trustManagerHelper3.PrintTrustTableAllAt (Seconds (75), trustRoutingStream3);
    }

//   for (uint32_t i = 0; i < 7; i++)
//     {

//       for (uint32_t j = 0; j < 7; j++)
//         {
//           if (i == ( (7 - 1) - j))
//             {
//               continue;
//             }
//           V4PingHelper ping (interfaces.GetAddress ( (7 - 1) - j));
//           ping.SetAttribute ("Verbose",
//                              BooleanValue (false));

//           ApplicationContainer p = ping.Install (nodes.Get (i));
//           p.Start (Seconds (0 + (i * 10)));
//           p.Stop (Seconds (50) - Seconds (0.001));
//         }

//     }
  
  Simulator::Stop (Seconds (600.));
  Simulator::Run ();
  Simulator::Destroy ();
}

