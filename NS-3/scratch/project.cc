#include <iostream>
#include <cmath>
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

using namespace ns3;
using namespace aodv;



/**
 * \ingroup trust-aodv-examples
 * \ingroup examples
 * \brief Test script.
 * 
 * This is non-linear simulation of 7 nodes with static positions
 * which uses a malicious node and trust based routing protocol
 */
class AodvExample 
{
public:
  AodvExample ();
  /**
   * \brief Configure script parameters
   * \param argc is the command line argument count
   * \param argv is the command line arguments
   * \return true on successful configuration
  */
  bool Configure (int argc, char **argv);
  /// Run simulation
  void Run ();
  /**
   * Report results
   * \param os the output stream
   */
  void Report (std::ostream & os);

public:

  // parameters
  /// Number of nodes
  uint32_t size;
  /// Simulation time, seconds
  double totalTime;
  /// Write per-device PCAP traces if true
  bool pcap;
  /// Print routes if true
  bool printRoutes;

  // network
  /// nodes used in the example
  NodeContainer nodes;
  /// devices used in the example
  NetDeviceContainer devices;
  /// interfaces used in the example
  Ipv4InterfaceContainer interfaces;

public:
  /// Create the nodes
  void CreateNodes ();
  /// Create the devices
  void CreateDevices ();
  /// Create the network
  void InstallInternetStack ();
  /// Create the simulation applications
  void InstallApplications ();
};

int main (int argc, char **argv)
{
  AodvExample test;
  if (!test.Configure (argc, argv))
    NS_FATAL_ERROR ("Configuration failed. Aborted.");

  test.Run ();
  test.Report (std::cout);
  return 0;
}

//-----------------------------------------------------------------------------
AodvExample::AodvExample () :
  size (13),
  totalTime (100),
  pcap (true),
  printRoutes (true)
{
}

bool
AodvExample::Configure (int argc, char **argv)
{

  bool verbose = false;

  SeedManager::SetSeed (12345);
  CommandLine cmd;

  cmd.AddValue ("pcap", "Write PCAP traces.", pcap);
  cmd.AddValue ("printRoutes", "Print routing table dumps.", printRoutes);
  cmd.AddValue ("time", "Simulation time, s.", totalTime);
  cmd.AddValue ("time", "Simulation time, s.", totalTime);
  cmd.AddValue ("verbose", "turn on the log", verbose);

  cmd.Parse (argc, argv);

  if (verbose)
    {
      LogComponentEnable ("SimpleAodvTrustManager", LOG_LEVEL_INFO);
    }

  return true;
}

void
AodvExample::Run ()
{
  CreateNodes ();
  CreateDevices ();
  InstallInternetStack ();
  InstallApplications ();

  Simulator::Stop (Seconds (totalTime));

  AnimationInterface anim ("aodv7n.xml");
  anim.EnablePacketMetadata(true);

  Simulator::Run ();
  Simulator::Destroy ();
}

void
AodvExample::Report (std::ostream &)
{ 
}

void
AodvExample::CreateNodes ()
{
  std::cout << "Creating " << (unsigned)size << " nodes.\n";
  nodes.Create (size);
  // Name nodes
  for (uint32_t i = 0; i < size; ++i)
    {
      std::ostringstream os;
      os << "node-" << i;
      Names::Add (os.str (), nodes.Get (i));
    }
  // Create static grid
  MobilityHelper mobility1;
  Ptr<ListPositionAllocator> positionAlloc1 = CreateObject <ListPositionAllocator>();
  positionAlloc1 ->Add(Vector(100, 100, 0)); // node1
  mobility1.SetPositionAllocator(positionAlloc1);
  mobility1.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility1.Install (nodes.Get(0));
  
  MobilityHelper mobility2;
  mobility2.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (60.0),
                                 "MinY", DoubleValue (80.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));


  mobility2.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  mobility2.Install (nodes.Get(1));
  Ptr<ConstantVelocityMobilityModel> mob = nodes.Get(1)->GetObject<ConstantVelocityMobilityModel>();
        	mob->SetVelocity(Vector(0.3, 0, 0));
  
  MobilityHelper mobility3;
  Ptr<ListPositionAllocator> positionAlloc3 = CreateObject <ListPositionAllocator>();
  positionAlloc3 ->Add(Vector(130, 70, 0)); // node3
  mobility3.SetPositionAllocator(positionAlloc3);
  mobility3.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility3.Install (nodes.Get(2));

  MobilityHelper mobility4;
  Ptr<ListPositionAllocator> positionAlloc4 = CreateObject <ListPositionAllocator>();
  positionAlloc4 ->Add(Vector(145, 50, 0)); // node4
  mobility4.SetPositionAllocator(positionAlloc4);
  mobility4.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility4.Install (nodes.Get(3));

  MobilityHelper mobility5;
  Ptr<ListPositionAllocator> positionAlloc5 = CreateObject <ListPositionAllocator>();
  positionAlloc5 ->Add(Vector(80, 145, 0)); // node5
  mobility5.SetPositionAllocator(positionAlloc5);
  mobility5.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility5.Install (nodes.Get(4));

  MobilityHelper mobility6;
  Ptr<ListPositionAllocator> positionAlloc6 = CreateObject <ListPositionAllocator>();
  positionAlloc6 ->Add(Vector(130, 130, 0)); // node6
  mobility6.SetPositionAllocator(positionAlloc6);
  mobility6.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility6.Install (nodes.Get(5));

  MobilityHelper mobility7;
  Ptr<ListPositionAllocator> positionAlloc7 = CreateObject <ListPositionAllocator>();
  positionAlloc7 ->Add(Vector(145, 110, 0)); // node7
  mobility7.SetPositionAllocator(positionAlloc7);
  mobility7.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility7.Install (nodes.Get(6));

  MobilityHelper mobility8;
  Ptr<ListPositionAllocator> positionAlloc8 = CreateObject <ListPositionAllocator>();
  positionAlloc8 ->Add(Vector(190, 130, 0)); // node8
  mobility8.SetPositionAllocator(positionAlloc8);
  mobility8.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility8.Install (nodes.Get(7));
  

  MobilityHelper mobility9;
  Ptr<ListPositionAllocator> positionAlloc9 = CreateObject <ListPositionAllocator>();
  positionAlloc9 ->Add(Vector(185, 160, 0)); // node10
  mobility9.SetPositionAllocator(positionAlloc9);
  mobility9.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility9.Install (nodes.Get(8));

  MobilityHelper mobilitya;
  Ptr<ListPositionAllocator> positionAlloca = CreateObject <ListPositionAllocator>();
  positionAlloca ->Add(Vector(175, 150, 0)); // node10
  mobilitya.SetPositionAllocator(positionAlloca);
  mobilitya.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilitya.Install (nodes.Get(9));

  MobilityHelper mobilityb;
  Ptr<ListPositionAllocator> positionAllocb = CreateObject <ListPositionAllocator>();
  positionAllocb ->Add(Vector(145, 185, 0)); // node11
  mobilityb.SetPositionAllocator(positionAllocb);
  mobilityb.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityb.Install (nodes.Get(10));

  MobilityHelper mobilityc;
  Ptr<ListPositionAllocator> positionAllocc = CreateObject <ListPositionAllocator>();
  positionAllocc ->Add(Vector(175, 200, 0)); // node12
  mobilityc.SetPositionAllocator(positionAllocc);
  mobilityc.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityc.Install (nodes.Get(11));

  MobilityHelper mobilityd;
  Ptr<ListPositionAllocator> positionAllocd = CreateObject <ListPositionAllocator>();
  positionAllocd ->Add(Vector(100, 175, 0)); // node13
  mobilityd.SetPositionAllocator(positionAllocd);
  mobilityd.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityd.Install (nodes.Get(12));
}

void
AodvExample::CreateDevices ()
{
  WifiMacHelper wifiMac;
  wifiMac.SetType ("ns3::AdhocWifiMac");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue (0));
  devices = wifi.Install (wifiPhy, wifiMac, nodes);
  //wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue (0));
  


  if (pcap)
    {
      wifiPhy.EnablePcapAll (std::string ("aodv"));
    }
}

void
AodvExample::InstallInternetStack ()
{
  NodeContainer normalNodes;
  normalNodes.Add (nodes.Get(0));
  normalNodes.Add (nodes.Get(1));
  normalNodes.Add (nodes.Get(2));
  normalNodes.Add (nodes.Get(3));
  normalNodes.Add (nodes.Get(4));
  normalNodes.Add (nodes.Get(5));
  normalNodes.Add (nodes.Get(6));
  normalNodes.Add (nodes.Get(7));
  normalNodes.Add (nodes.Get(8));
  normalNodes.Add (nodes.Get(9));
  normalNodes.Add (nodes.Get(10));
  normalNodes.Add (nodes.Get(11));
  normalNodes.Add (nodes.Get(12));

  //NodeContainer selfishNodes;
  //selfishNodes.Add (nodes.Get(6));

  TrustAodvHelper aodv;
  // you can configure AODV attributes here using aodv.Set(name, value)
  InternetStackHelper stack;
  stack.SetRoutingHelper (aodv); // has effect on the next Install ()
  stack.Install (normalNodes);

  //SelfishAodvHelper malAodv;
  //stack.SetRoutingHelper (malAodv); // has effect on the next Install ()
  //stack.Install (selfishNodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.0.0.0");
  interfaces = address.Assign (devices);

  if (printRoutes)
    {
      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("trust-aodv_routing_table_at_55.routes", std::ios::out);
      aodv.PrintRoutingTableAllAt (Seconds (15), routingStream);

      Ptr<OutputStreamWrapper> routingStream3 = Create<OutputStreamWrapper> ("trust-aodv_routing_table_at_75.routes", std::ios::out);
      aodv.PrintRoutingTableAllAt (Seconds (95), routingStream3);

      TrustManagerHelper trustManagerHelper0;
      Ptr<OutputStreamWrapper> trustRoutingStream0 = Create<OutputStreamWrapper> ("Trust_Table_9s.routes", std::ios::out);
      trustManagerHelper0.PrintTrustTableAllAt (Seconds (9), trustRoutingStream0);

      TrustManagerHelper trustManagerHelper1;
      Ptr<OutputStreamWrapper> trustRoutingStream1 = Create<OutputStreamWrapper> ("Trust_Table_18s.routes", std::ios::out);
      trustManagerHelper1.PrintTrustTableAllAt (Seconds (18), trustRoutingStream1);

      TrustManagerHelper trustManagerHelper2;
      Ptr<OutputStreamWrapper> trustRoutingStream2 = Create<OutputStreamWrapper> ("Trust_Table_25s.routes", std::ios::out);
      trustManagerHelper2.PrintTrustTableAllAt (Seconds (25), trustRoutingStream2);

      TrustManagerHelper trustManagerHelper3;
      Ptr<OutputStreamWrapper> trustRoutingStream3 = Create<OutputStreamWrapper> ("Trust_Table_68s.routes", std::ios::out);
      trustManagerHelper3.PrintTrustTableAllAt (Seconds (68), trustRoutingStream3);

      TrustManagerHelper trustManagerHelper5;
      Ptr<OutputStreamWrapper> trustRoutingStream5 = Create<OutputStreamWrapper> ("Trust_Table_95s.routes", std::ios::out);
      trustManagerHelper5.PrintTrustTableAllAt (Seconds (95), trustRoutingStream5);
    }
}

void
AodvExample::InstallApplications ()
{

  for (uint32_t i = 0; i < size; i++)
    {

      for (uint32_t j = 0; j < size; j++)
        {
          if (i == ( (size - 1) - j))
            {
              continue;
            }
          V4PingHelper ping (interfaces.GetAddress ( (size - 1) - j));
          ping.SetAttribute ("Verbose",
                             BooleanValue (false));

          ApplicationContainer p = ping.Install (nodes.Get (i));
          p.Start (Seconds (0 + (i * 10)));
          p.Stop (Seconds (70) - Seconds (0.001));
        }

    }

  // We want to check the route between 0 and 4.
  // Normally it should pass through node 1 and 2.
  V4PingHelper ping (interfaces.GetAddress (0));
  ping.SetAttribute ("Verbose",
                     BooleanValue (true));

  ApplicationContainer p = ping.Install (nodes.Get (6));
  p.Start (Seconds (80));
  p.Stop (Seconds (90) - Seconds (0.001));
}


