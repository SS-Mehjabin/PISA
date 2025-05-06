#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/aodv-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/internet-module.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/energy-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/wifi-radio-energy-model-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/v4ping-helper.h"
#include <stdio.h>
#include "myapp.h"

#include "ns3/trust-aodv-module.h"
#include "ns3/trust-manager-helper.h"
int blahh=0;
std::ofstream outfile ("malicious_10_energy_output.txt");
std::ofstream outfile2 ("malicious_10_data.csv");


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WifiSimpleAdhocGrid");

static inline std::string
PrintReceivedPacket (Address& from)
{
  InetSocketAddress iaddr = InetSocketAddress::ConvertFrom (from);

  std::ostringstream oss;
  oss << "--\nReceived one packet! Socket: " << iaddr.GetIpv4 ()
      << " port: " << iaddr.GetPort ()
      << " at time = " << Simulator::Now ().GetSeconds ()
      << "\n--";

  return oss.str ();
}

/**
 * \param socket Pointer to socket.
 *
 * Packet receiving sink.
 */
void
ReceivePacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () > 0)
        {
          NS_LOG_UNCOND (PrintReceivedPacket (from));
        }
    }
}

static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize,
                             uint32_t pktCount, Time pktInterval )
{
  if (pktCount > 0)
    {
      socket->Send (Create<Packet> (pktSize));
      Simulator::Schedule (pktInterval, &GenerateTraffic,
                           socket, pktSize,pktCount - 1, pktInterval);
    }
  else
    {
      socket->Close ();
    }
}

void
RemainingEnergy (double oldValue, double remainingEnergy)
{
   NS_LOG_UNCOND (Simulator::Now ().GetSeconds ()
                 << "s Current remaining energy = " << remainingEnergy << "J Node" << blahh);
  outfile << Simulator::Now ().GetSeconds ()<< "s Current remaining energy = " << remainingEnergy << "J Node" << blahh << std::endl;
  if (blahh>50-2) blahh=0;
  else blahh=blahh+1;
}

/// Trace function for total energy consumption at node.
void
TotalEnergy (double oldValue, double totalEnergy)
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds ()
                 << "s Total energy consumed by radio = " << totalEnergy << "J");
}


void throughput(Ptr<FlowMonitor> monitor,FlowMonitorHelper* flowmon) {
  monitor->CheckForLostPackets ();
  float AvgThroughput=0.0;
  int j=0;
  Time Delay;
  Time Jitter;
  int SentPackets = 0;
  int LostPackets = 0;
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon->GetClassifier ());
        std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      // Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
          AvgThroughput = AvgThroughput + (i->second.rxBytes * 8.0/(i->second.timeLastRxPacket.GetSeconds()-i->second.timeFirstTxPacket.GetSeconds())/1024);
          Delay = Delay + (i->second.delaySum);
          Jitter = Jitter + (i->second.jitterSum);

          SentPackets = SentPackets +(i->second.txPackets);
          LostPackets = LostPackets + (i->second.txPackets-i->second.rxPackets);
          j=j+1;
          // std::cout <<"Throughput?? = "<< AvgThroughput;
          // NS_LOG_UNCOND("Throughput =" <<i->second.rxBytes * 8.0/(i->second.timeLastRxPacket.GetSeconds()-i->second.timeFirstTxPacket.GetSeconds())/1024<<"Kbps");
    }
    outfile2 <<"Time,"<< Simulator::Now ().GetSeconds ()<<","<<"Throughput,"<< AvgThroughput/j<<","<<","<<"Delay,"<< Delay<<","<<"Jitter,"<< Jitter<<","<<"Loss,"<<((LostPackets*100)/SentPackets)<<","<<"\n";
// Reschedule the method after 1sec
Simulator::Schedule(Seconds(1.0), &throughput, monitor, flowmon);
}

int main (int argc, char *argv[])
{
  std::string phyMode ("DsssRate1Mbps");
  double distance = 500;  // m
  uint32_t packetSize = 256; // bytes
  uint32_t numPackets = 100;
  uint32_t numNodes = 50;  // by default, 5x5
  uint32_t sinkNode = 3;
  uint32_t sourceNode = 0;
  
  uint32_t SentPackets = 0;
  uint32_t ReceivedPackets = 0;
  uint32_t LostPackets = 0;
  
  double interval = 0.1; // seconds
  bool verbose = false;
  bool tracing = true;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("distance", "distance (m)", distance);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("numPackets", "number of packets generated", numPackets);
  cmd.AddValue ("interval", "interval (seconds) between packets", interval);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("tracing", "turn on ascii and pcap tracing", tracing);
  cmd.AddValue ("numNodes", "number of nodes", numNodes);
  cmd.AddValue ("sinkNode", "Receiver node number", sinkNode);
  cmd.AddValue ("sourceNode", "Sender node number", sourceNode);
  cmd.Parse (argc, argv);
  // Convert to time object
  Time interPacketInterval = Seconds (interval);

  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue (phyMode));

  NodeContainer c;
  c.Create (numNodes);
  NodeContainer not_malicious;  
  not_malicious.Add(c.Get(0));
  not_malicious.Add(c.Get(1));
  not_malicious.Add(c.Get(2));
  not_malicious.Add(c.Get(3));
  not_malicious.Add(c.Get(4));
  not_malicious.Add(c.Get(5));
  not_malicious.Add(c.Get(6));
  not_malicious.Add(c.Get(7));
  not_malicious.Add(c.Get(8));
  not_malicious.Add(c.Get(9));
  not_malicious.Add(c.Get(10));
  not_malicious.Add(c.Get(11));
  not_malicious.Add(c.Get(12));
  not_malicious.Add(c.Get(13));
  not_malicious.Add(c.Get(14));
  not_malicious.Add(c.Get(15));
  not_malicious.Add(c.Get(16));
  not_malicious.Add(c.Get(17));
  not_malicious.Add(c.Get(18));
  not_malicious.Add(c.Get(19));
  not_malicious.Add(c.Get(20));
  not_malicious.Add(c.Get(21));
  not_malicious.Add(c.Get(22));
  not_malicious.Add(c.Get(23));
  not_malicious.Add(c.Get(24));
  not_malicious.Add(c.Get(25));
  not_malicious.Add(c.Get(26));
  not_malicious.Add(c.Get(27));
  not_malicious.Add(c.Get(28));
  not_malicious.Add(c.Get(29));
  not_malicious.Add(c.Get(30));
  not_malicious.Add(c.Get(31));
  not_malicious.Add(c.Get(32));
  not_malicious.Add(c.Get(33));
  not_malicious.Add(c.Get(34));
  not_malicious.Add(c.Get(35));
  not_malicious.Add(c.Get(36));
  not_malicious.Add(c.Get(37));
  not_malicious.Add(c.Get(38));
  not_malicious.Add(c.Get(39));
  not_malicious.Add(c.Get(40));
  not_malicious.Add(c.Get(41));
  not_malicious.Add(c.Get(42));
  not_malicious.Add(c.Get(43));
  not_malicious.Add(c.Get(44));
  not_malicious.Add(c.Get(45));
  not_malicious.Add(c.Get(46));
  not_malicious.Add(c.Get(47));
  not_malicious.Add(c.Get(48));
  not_malicious.Add(c.Get(49));
//  NodeContainer malicious;
//  malicious.Add(c.Get(2));
WifiHelper wifi;
  if (verbose)
    {
      wifi.EnableLogComponents ();  // Turn on all Wifi logging
    }

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  // set it to zero; otherwise, gain will be added
  wifiPhy.Set ("RxGain", DoubleValue (-10) );
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add an upper mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetStandard (WIFI_STANDARD_80211b);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
  // Set it to adhoc mode
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, c);


  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();  
positionAlloc->Add (Vector (-44,-62, 0.0));
  positionAlloc->Add (Vector (8,52, 0.0));
  positionAlloc->Add (Vector (18,5, 0.0));
  positionAlloc->Add (Vector (-49,-41, 0.0));
  positionAlloc->Add (Vector (25,-28, 0.0));
  positionAlloc->Add (Vector (4,-54, 0.0));
  positionAlloc->Add (Vector (69,35, 0.0));
  positionAlloc->Add (Vector (86,-35, 0.0));
  positionAlloc->Add (Vector (59,0, 0.0));
  positionAlloc->Add (Vector (-64,3, 0.0));
  positionAlloc->Add (Vector (14,-16, 0.0));
  positionAlloc->Add (Vector (-26,9, 0.0));
  positionAlloc->Add (Vector (13,65, 0.0));
  positionAlloc->Add (Vector (17,38, 0.0));
  positionAlloc->Add (Vector (-22,-9, 0.0));
  positionAlloc->Add (Vector (18,-39, 0.0));
  positionAlloc->Add (Vector (-14,-19, 0.0));
  positionAlloc->Add (Vector (14,26, 0.0));
  positionAlloc->Add (Vector (-70,-25, 0.0));
  positionAlloc->Add (Vector (48,-18, 0.0));
  positionAlloc->Add (Vector (-28,59, 0.0));
  positionAlloc->Add (Vector (42,16, 0.0));
  positionAlloc->Add (Vector (-36,24, 0.0));
  positionAlloc->Add (Vector (-23,-30, 0.0));
  positionAlloc->Add (Vector (-4,-102, 0.0));
  positionAlloc->Add (Vector (-5,-66, 0.0));
  positionAlloc->Add (Vector (-74,17, 0.0));
  positionAlloc->Add (Vector (-44,39, 0.0));
  positionAlloc->Add (Vector (-3,-7, 0.0));
  positionAlloc->Add (Vector (1,-24, 0.0));
  positionAlloc->Add (Vector (-15,-48, 0.0));
  positionAlloc->Add (Vector (-31,97, 0.0));
  positionAlloc->Add (Vector (36,-76, 0.0));
  positionAlloc->Add (Vector (52,56, 0.0));
  positionAlloc->Add (Vector (-24,31, 0.0));
  positionAlloc->Add (Vector (-9,23, 0.0));
  positionAlloc->Add (Vector (76,7, 0.0));
  positionAlloc->Add (Vector (9,-4, 0.0));
  positionAlloc->Add (Vector (-36,-3, 0.0));
  positionAlloc->Add (Vector (-5,6, 0.0));
  positionAlloc->Add (Vector (-54,22, 0.0));
  positionAlloc->Add (Vector (27,18, 0.0));
  positionAlloc->Add (Vector (-49,-16, 0.0));
  positionAlloc->Add (Vector (5,22, 0.0));
  positionAlloc->Add (Vector (-18,44, 0.0));
  positionAlloc->Add (Vector (34,35, 0.0));
  positionAlloc->Add (Vector (43,-35, 0.0));
  positionAlloc->Add (Vector (36,-3, 0.0));
  positionAlloc->Add (Vector (-2,-41, 0.0));
  positionAlloc->Add (Vector (-6,56, 0.0));  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (c);
  
  /* energy source */
  BasicEnergySourceHelper basicSourceHelper;
  // configure energy source
  basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (500.1));
  // install source
  EnergySourceContainer sources = basicSourceHelper.Install (c);
  /* device energy model */
  WifiRadioEnergyModelHelper radioEnergyHelper;
  // configure radio energy model
  radioEnergyHelper.Set ("TxCurrentA", DoubleValue (0.0174));
  // install device model
  DeviceEnergyModelContainer deviceModels = radioEnergyHelper.Install (devices, sources);



  // Enable OLSR
  AodvHelper aodv;

  Ipv4StaticRoutingHelper staticRouting;

  // Ipv4ListRoutingHelper list;
  // list.Add (staticRouting, 2);
  // list.Add (aodv, 1);
  InternetStackHelper internet;
  internet.SetRoutingHelper (aodv); // has effect on the next Install ()
  internet.Install (not_malicious);

  // AodvHelper malaodv;
  // Ipv4StaticRoutingHelper staticRouting1;

  // Ipv4ListRoutingHelper list1;
  // list1.Add (staticRouting1, 1);
  // list1.Add (malaodv, 1);
  // malaodv.Set("IsMalicious",BooleanValue(true));
  // internet.SetRoutingHelper (malaodv); // has effect on the next Install ()
  // internet.Install (malicious);

//  internet.SetRoutingHelper (aodv); // has effect on the next Install ()
//  internet.Install (malicious);


  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();
  monitor->Start(Seconds(30.0));
  // monitor->Stop(Seconds(331.0));

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = ipv4.Assign (devices);

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink0 = Socket::CreateSocket (c.Get (0), tid);
  InetSocketAddress local0 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink0->Bind (local0);
  recvSink0->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink1 = Socket::CreateSocket (c.Get (1), tid);
  InetSocketAddress local1 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink1->Bind (local1);
  recvSink1->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink2 = Socket::CreateSocket (c.Get (2), tid);
  InetSocketAddress local2 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink2->Bind (local2);
  recvSink2->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink3 = Socket::CreateSocket (c.Get (3), tid);
  InetSocketAddress local3 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink3->Bind (local3);
  recvSink3->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink4 = Socket::CreateSocket (c.Get (4), tid);
  InetSocketAddress local4 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink4->Bind (local4);
  recvSink4->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink5 = Socket::CreateSocket (c.Get (5), tid);
  InetSocketAddress local5 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink5->Bind (local5);
  recvSink5->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink6 = Socket::CreateSocket (c.Get (6), tid);
  InetSocketAddress local6 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink6->Bind (local6);
  recvSink6->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink7 = Socket::CreateSocket (c.Get (7), tid);
  InetSocketAddress local7 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink7->Bind (local7);
  recvSink7->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink8 = Socket::CreateSocket (c.Get (8), tid);
  InetSocketAddress local8 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink8->Bind (local8);
  recvSink8->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink9 = Socket::CreateSocket (c.Get (9), tid);
  InetSocketAddress local9 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink9->Bind (local9);
  recvSink9->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink10 = Socket::CreateSocket (c.Get (10), tid);
  InetSocketAddress local10 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink10->Bind (local10);
  recvSink10->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink11 = Socket::CreateSocket (c.Get (11), tid);
  InetSocketAddress local11 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink11->Bind (local11);
  recvSink11->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink12 = Socket::CreateSocket (c.Get (12), tid);
  InetSocketAddress local12 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink12->Bind (local12);
  recvSink12->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink13 = Socket::CreateSocket (c.Get (13), tid);
  InetSocketAddress local13 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink13->Bind (local13);
  recvSink13->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink14 = Socket::CreateSocket (c.Get (14), tid);
  InetSocketAddress local14 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink14->Bind (local14);
  recvSink14->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink15 = Socket::CreateSocket (c.Get (15), tid);
  InetSocketAddress local15 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink15->Bind (local15);
  recvSink15->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink16 = Socket::CreateSocket (c.Get (16), tid);
  InetSocketAddress local16 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink16->Bind (local16);
  recvSink16->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink17 = Socket::CreateSocket (c.Get (17), tid);
  InetSocketAddress local17 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink17->Bind (local17);
  recvSink17->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink18 = Socket::CreateSocket (c.Get (18), tid);
  InetSocketAddress local18 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink18->Bind (local18);
  recvSink18->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink19 = Socket::CreateSocket (c.Get (19), tid);
  InetSocketAddress local19 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink19->Bind (local19);
  recvSink19->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink20 = Socket::CreateSocket (c.Get (20), tid);
  InetSocketAddress local20 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink20->Bind (local20);
  recvSink20->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink21 = Socket::CreateSocket (c.Get (21), tid);
  InetSocketAddress local21 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink21->Bind (local21);
  recvSink21->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink22 = Socket::CreateSocket (c.Get (22), tid);
  InetSocketAddress local22 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink22->Bind (local22);
  recvSink22->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink23 = Socket::CreateSocket (c.Get (23), tid);
  InetSocketAddress local23 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink23->Bind (local23);
  recvSink23->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink24 = Socket::CreateSocket (c.Get (24), tid);
  InetSocketAddress local24 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink24->Bind (local24);
  recvSink24->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink25 = Socket::CreateSocket (c.Get (25), tid);
  InetSocketAddress local25 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink25->Bind (local25);
  recvSink25->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink26 = Socket::CreateSocket (c.Get (26), tid);
  InetSocketAddress local26 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink26->Bind (local26);
  recvSink26->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink27 = Socket::CreateSocket (c.Get (27), tid);
  InetSocketAddress local27 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink27->Bind (local27);
  recvSink27->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink28 = Socket::CreateSocket (c.Get (28), tid);
  InetSocketAddress local28 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink28->Bind (local28);
  recvSink28->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink29 = Socket::CreateSocket (c.Get (29), tid);
  InetSocketAddress local29 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink29->Bind (local29);
  recvSink29->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink30 = Socket::CreateSocket (c.Get (30), tid);
  InetSocketAddress local30 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink30->Bind (local30);
  recvSink30->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink31 = Socket::CreateSocket (c.Get (31), tid);
  InetSocketAddress local31 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink31->Bind (local31);
  recvSink31->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink32 = Socket::CreateSocket (c.Get (32), tid);
  InetSocketAddress local32 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink32->Bind (local32);
  recvSink32->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink33 = Socket::CreateSocket (c.Get (33), tid);
  InetSocketAddress local33 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink33->Bind (local33);
  recvSink33->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink34 = Socket::CreateSocket (c.Get (34), tid);
  InetSocketAddress local34 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink34->Bind (local34);
  recvSink34->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink35 = Socket::CreateSocket (c.Get (35), tid);
  InetSocketAddress local35 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink35->Bind (local35);
  recvSink35->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink36 = Socket::CreateSocket (c.Get (36), tid);
  InetSocketAddress local36 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink36->Bind (local36);
  recvSink36->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink37 = Socket::CreateSocket (c.Get (37), tid);
  InetSocketAddress local37 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink37->Bind (local37);
  recvSink37->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink38 = Socket::CreateSocket (c.Get (38), tid);
  InetSocketAddress local38 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink38->Bind (local38);
  recvSink38->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink39 = Socket::CreateSocket (c.Get (39), tid);
  InetSocketAddress local39 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink39->Bind (local39);
  recvSink39->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink40 = Socket::CreateSocket (c.Get (40), tid);
  InetSocketAddress local40 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink40->Bind (local40);
  recvSink40->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink41 = Socket::CreateSocket (c.Get (41), tid);
  InetSocketAddress local41 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink41->Bind (local41);
  recvSink41->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink42 = Socket::CreateSocket (c.Get (42), tid);
  InetSocketAddress local42 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink42->Bind (local42);
  recvSink42->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink43 = Socket::CreateSocket (c.Get (43), tid);
  InetSocketAddress local43 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink43->Bind (local43);
  recvSink43->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink44 = Socket::CreateSocket (c.Get (44), tid);
  InetSocketAddress local44 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink44->Bind (local44);
  recvSink44->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink45 = Socket::CreateSocket (c.Get (45), tid);
  InetSocketAddress local45 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink45->Bind (local45);
  recvSink45->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink46 = Socket::CreateSocket (c.Get (46), tid);
  InetSocketAddress local46 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink46->Bind (local46);
  recvSink46->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink47 = Socket::CreateSocket (c.Get (47), tid);
  InetSocketAddress local47 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink47->Bind (local47);
  recvSink47->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink48 = Socket::CreateSocket (c.Get (48), tid);
  InetSocketAddress local48 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink48->Bind (local48);
  recvSink48->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> recvSink49 = Socket::CreateSocket (c.Get (49), tid);
  InetSocketAddress local49 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink49->Bind (local49);
  recvSink49->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> source0 = Socket::CreateSocket (c.Get (0), tid);
  InetSocketAddress remote0 = InetSocketAddress (interfaces.GetAddress (1, 0), 80);
  source0->Connect (remote0);

  Ptr<Socket> source1 = Socket::CreateSocket (c.Get (1), tid);
  InetSocketAddress remote1 = InetSocketAddress (interfaces.GetAddress (0, 0), 80);
  source1->Connect (remote1);

  Ptr<Socket> source2 = Socket::CreateSocket (c.Get (0), tid);
  InetSocketAddress remote2 = InetSocketAddress (interfaces.GetAddress (43, 0), 80);
  source2->Connect (remote2);

  Ptr<Socket> source3 = Socket::CreateSocket (c.Get (43), tid);
  InetSocketAddress remote3 = InetSocketAddress (interfaces.GetAddress (0, 0), 80);
  source3->Connect (remote3);

  Ptr<Socket> source4 = Socket::CreateSocket (c.Get (0), tid);
  InetSocketAddress remote4 = InetSocketAddress (interfaces.GetAddress (46, 0), 80);
  source4->Connect (remote4);

  Ptr<Socket> source5 = Socket::CreateSocket (c.Get (46), tid);
  InetSocketAddress remote5 = InetSocketAddress (interfaces.GetAddress (0, 0), 80);
  source5->Connect (remote5);

  Ptr<Socket> source6 = Socket::CreateSocket (c.Get (0), tid);
  InetSocketAddress remote6 = InetSocketAddress (interfaces.GetAddress (48, 0), 80);
  source6->Connect (remote6);

  Ptr<Socket> source7 = Socket::CreateSocket (c.Get (48), tid);
  InetSocketAddress remote7 = InetSocketAddress (interfaces.GetAddress (0, 0), 80);
  source7->Connect (remote7);

  Ptr<Socket> source8 = Socket::CreateSocket (c.Get (0), tid);
  InetSocketAddress remote8 = InetSocketAddress (interfaces.GetAddress (23, 0), 80);
  source8->Connect (remote8);

  Ptr<Socket> source9 = Socket::CreateSocket (c.Get (23), tid);
  InetSocketAddress remote9 = InetSocketAddress (interfaces.GetAddress (0, 0), 80);
  source9->Connect (remote9);

  Ptr<Socket> source10 = Socket::CreateSocket (c.Get (1), tid);
  InetSocketAddress remote10 = InetSocketAddress (interfaces.GetAddress (7, 0), 80);
  source10->Connect (remote10);

  Ptr<Socket> source11 = Socket::CreateSocket (c.Get (7), tid);
  InetSocketAddress remote11 = InetSocketAddress (interfaces.GetAddress (1, 0), 80);
  source11->Connect (remote11);

  Ptr<Socket> source12 = Socket::CreateSocket (c.Get (43), tid);
  InetSocketAddress remote12 = InetSocketAddress (interfaces.GetAddress (7, 0), 80);
  source12->Connect (remote12);

  Ptr<Socket> source13 = Socket::CreateSocket (c.Get (7), tid);
  InetSocketAddress remote13 = InetSocketAddress (interfaces.GetAddress (43, 0), 80);
  source13->Connect (remote13);

  Ptr<Socket> source14 = Socket::CreateSocket (c.Get (46), tid);
  InetSocketAddress remote14 = InetSocketAddress (interfaces.GetAddress (7, 0), 80);
  source14->Connect (remote14);

  Ptr<Socket> source15 = Socket::CreateSocket (c.Get (7), tid);
  InetSocketAddress remote15 = InetSocketAddress (interfaces.GetAddress (46, 0), 80);
  source15->Connect (remote15);

  Ptr<Socket> source16 = Socket::CreateSocket (c.Get (48), tid);
  InetSocketAddress remote16 = InetSocketAddress (interfaces.GetAddress (7, 0), 80);
  source16->Connect (remote16);

  Ptr<Socket> source17 = Socket::CreateSocket (c.Get (7), tid);
  InetSocketAddress remote17 = InetSocketAddress (interfaces.GetAddress (48, 0), 80);
  source17->Connect (remote17);

  Ptr<Socket> source18 = Socket::CreateSocket (c.Get (23), tid);
  InetSocketAddress remote18 = InetSocketAddress (interfaces.GetAddress (7, 0), 80);
  source18->Connect (remote18);

  Ptr<Socket> source19 = Socket::CreateSocket (c.Get (7), tid);
  InetSocketAddress remote19 = InetSocketAddress (interfaces.GetAddress (23, 0), 80);
  source19->Connect (remote19);

  Ptr<Socket> source20 = Socket::CreateSocket (c.Get (0), tid);
  InetSocketAddress remote20 = InetSocketAddress (interfaces.GetAddress (10, 0), 80);
  source20->Connect (remote20);

  Ptr<Socket> source21 = Socket::CreateSocket (c.Get (10), tid);
  InetSocketAddress remote21 = InetSocketAddress (interfaces.GetAddress (0, 0), 80);
  source21->Connect (remote21);

  Ptr<Socket> source22 = Socket::CreateSocket (c.Get (1), tid);
  InetSocketAddress remote22 = InetSocketAddress (interfaces.GetAddress (10, 0), 80);
  source22->Connect (remote22);

  Ptr<Socket> source23 = Socket::CreateSocket (c.Get (10), tid);
  InetSocketAddress remote23 = InetSocketAddress (interfaces.GetAddress (1, 0), 80);
  source23->Connect (remote23);

  Ptr<BasicEnergySource> basicSourcePtr0 = DynamicCast<BasicEnergySource> (sources.Get (0));
  basicSourcePtr0->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr1 = DynamicCast<BasicEnergySource> (sources.Get (1));
  basicSourcePtr1->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr2 = DynamicCast<BasicEnergySource> (sources.Get (2));
  basicSourcePtr2->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr3 = DynamicCast<BasicEnergySource> (sources.Get (3));
  basicSourcePtr3->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr4 = DynamicCast<BasicEnergySource> (sources.Get (4));
  basicSourcePtr4->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr5 = DynamicCast<BasicEnergySource> (sources.Get (5));
  basicSourcePtr5->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr6 = DynamicCast<BasicEnergySource> (sources.Get (6));
  basicSourcePtr6->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr7 = DynamicCast<BasicEnergySource> (sources.Get (7));
  basicSourcePtr7->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr8 = DynamicCast<BasicEnergySource> (sources.Get (8));
  basicSourcePtr8->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr9 = DynamicCast<BasicEnergySource> (sources.Get (9));
  basicSourcePtr9->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr10 = DynamicCast<BasicEnergySource> (sources.Get (10));
  basicSourcePtr10->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr11 = DynamicCast<BasicEnergySource> (sources.Get (11));
  basicSourcePtr11->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr12 = DynamicCast<BasicEnergySource> (sources.Get (12));
  basicSourcePtr12->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr13 = DynamicCast<BasicEnergySource> (sources.Get (13));
  basicSourcePtr13->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr14 = DynamicCast<BasicEnergySource> (sources.Get (14));
  basicSourcePtr14->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr15 = DynamicCast<BasicEnergySource> (sources.Get (15));
  basicSourcePtr15->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr16 = DynamicCast<BasicEnergySource> (sources.Get (16));
  basicSourcePtr16->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr17 = DynamicCast<BasicEnergySource> (sources.Get (17));
  basicSourcePtr17->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr18 = DynamicCast<BasicEnergySource> (sources.Get (18));
  basicSourcePtr18->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr19 = DynamicCast<BasicEnergySource> (sources.Get (19));
  basicSourcePtr19->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr20 = DynamicCast<BasicEnergySource> (sources.Get (20));
  basicSourcePtr20->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr21 = DynamicCast<BasicEnergySource> (sources.Get (21));
  basicSourcePtr21->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr22 = DynamicCast<BasicEnergySource> (sources.Get (22));
  basicSourcePtr22->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr23 = DynamicCast<BasicEnergySource> (sources.Get (23));
  basicSourcePtr23->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr24 = DynamicCast<BasicEnergySource> (sources.Get (24));
  basicSourcePtr24->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr25 = DynamicCast<BasicEnergySource> (sources.Get (25));
  basicSourcePtr25->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr26 = DynamicCast<BasicEnergySource> (sources.Get (26));
  basicSourcePtr26->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr27 = DynamicCast<BasicEnergySource> (sources.Get (27));
  basicSourcePtr27->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr28 = DynamicCast<BasicEnergySource> (sources.Get (28));
  basicSourcePtr28->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr29 = DynamicCast<BasicEnergySource> (sources.Get (29));
  basicSourcePtr29->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr30 = DynamicCast<BasicEnergySource> (sources.Get (30));
  basicSourcePtr30->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr31 = DynamicCast<BasicEnergySource> (sources.Get (31));
  basicSourcePtr31->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr32 = DynamicCast<BasicEnergySource> (sources.Get (32));
  basicSourcePtr32->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr33 = DynamicCast<BasicEnergySource> (sources.Get (33));
  basicSourcePtr33->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr34 = DynamicCast<BasicEnergySource> (sources.Get (34));
  basicSourcePtr34->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr35 = DynamicCast<BasicEnergySource> (sources.Get (35));
  basicSourcePtr35->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr36 = DynamicCast<BasicEnergySource> (sources.Get (36));
  basicSourcePtr36->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr37 = DynamicCast<BasicEnergySource> (sources.Get (37));
  basicSourcePtr37->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr38 = DynamicCast<BasicEnergySource> (sources.Get (38));
  basicSourcePtr38->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr39 = DynamicCast<BasicEnergySource> (sources.Get (39));
  basicSourcePtr39->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr40 = DynamicCast<BasicEnergySource> (sources.Get (40));
  basicSourcePtr40->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr41 = DynamicCast<BasicEnergySource> (sources.Get (41));
  basicSourcePtr41->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr42 = DynamicCast<BasicEnergySource> (sources.Get (42));
  basicSourcePtr42->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr43 = DynamicCast<BasicEnergySource> (sources.Get (43));
  basicSourcePtr43->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr44 = DynamicCast<BasicEnergySource> (sources.Get (44));
  basicSourcePtr44->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr45 = DynamicCast<BasicEnergySource> (sources.Get (45));
  basicSourcePtr45->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr46 = DynamicCast<BasicEnergySource> (sources.Get (46));
  basicSourcePtr46->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr47 = DynamicCast<BasicEnergySource> (sources.Get (47));
  basicSourcePtr47->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr48 = DynamicCast<BasicEnergySource> (sources.Get (48));
  basicSourcePtr48->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  Ptr<BasicEnergySource> basicSourcePtr49 = DynamicCast<BasicEnergySource> (sources.Get (49));
  basicSourcePtr49->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));

  // Ipv4StaticRoutingHelper staticRouting;
  // Ptr<Ipv4StaticRouting> staticRoute = staticRouting.GetStaticRouting(c.Get(5)->GetObject<Ipv4>());
  // staticRoute->AddHostRouteTo(interfaces.GetAddress(4), interfaces.GetAddress(3), 1, 0);

  // // Ipv4StaticRoutingHelper staticRouting5;
  // Ptr<Ipv4StaticRouting> staticRoute1 = staticRouting.GetStaticRouting(c.Get(5)->GetObject<Ipv4>());
  // staticRoute1->AddHostRouteTo(interfaces.GetAddress(0), interfaces.GetAddress(3), 1, 0);

  // // Ipv4StaticRoutingHelper staticRouting1;
  // Ptr<Ipv4StaticRouting> staticRoute2 = staticRouting.GetStaticRouting(c.Get(4)->GetObject<Ipv4>());
  // staticRoute2->AddHostRouteTo(interfaces.GetAddress(5), interfaces.GetAddress(3), 1, 0);

  // // Ipv4StaticRoutingHelper staticRouting2;
  // Ptr<Ipv4StaticRouting> staticRoute3 = staticRouting.GetStaticRouting(c.Get(0)->GetObject<Ipv4>());
  // staticRoute3->AddHostRouteTo(interfaces.GetAddress(5), interfaces.GetAddress(3), 1, 0);

  ////

  if (tracing == true)
    {
      AsciiTraceHelper ascii;
      wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("malicious_10_trace.tr"));
      wifiPhy.EnablePcap ("malicious_10", devices);
      // Trace routing tables
      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("malicious_10.routes", std::ios::out);
      aodv.PrintRoutingTableAllEvery (Seconds (2), routingStream);
      Ptr<OutputStreamWrapper> neighborStream = Create<OutputStreamWrapper> ("malicious_10.neighbors", std::ios::out);
      aodv.PrintNeighborCacheAllEvery (Seconds (2), neighborStream);

      // To do-- enable an IP-level trace that shows forwarding events only
    }  
	
  Simulator::Schedule (Seconds (0), &GenerateTraffic, source0, packetSize,200, Seconds(1.0)); 
  Simulator::Schedule (Seconds (0.2), &GenerateTraffic, source1, packetSize,200, Seconds(1.0)); 
  Simulator::Schedule (Seconds (0.4), &GenerateTraffic, source2, packetSize,200, Seconds(1.0)); 
  Simulator::Schedule (Seconds (0.6), &GenerateTraffic, source3, packetSize,200, Seconds(1.0)); 
  Simulator::Schedule (Seconds (0.8), &GenerateTraffic, source4, packetSize,200, Seconds(1.0)); 
  Simulator::Schedule (Seconds (1.0), &GenerateTraffic, source5, packetSize,200, Seconds(1.0)); 
  Simulator::Schedule (Seconds (1.2), &GenerateTraffic, source6, packetSize,200, Seconds(1.0)); 
  Simulator::Schedule (Seconds (1.4), &GenerateTraffic, source7, packetSize,200, Seconds(1.0)); 
  Simulator::Schedule (Seconds (1.6), &GenerateTraffic, source8, packetSize,200, Seconds(1.0)); 
  Simulator::Schedule (Seconds (1.8), &GenerateTraffic, source9, packetSize,200, Seconds(1.0)); 
  Simulator::Schedule (Seconds (2.0), &GenerateTraffic, source10, packetSize,200, Seconds(1.0)); 
  Simulator::Schedule (Seconds (2.2), &GenerateTraffic, source11, packetSize,200, Seconds(1.0)); 
  Simulator::Schedule (Seconds (2.4), &GenerateTraffic, source12, packetSize,200, Seconds(1.0)); 
  Simulator::Schedule (Seconds (2.6), &GenerateTraffic, source13, packetSize,200, Seconds(1.0)); 
  Simulator::Schedule (Seconds (2.8), &GenerateTraffic, source14, packetSize,200, Seconds(1.0)); 
  Simulator::Schedule (Seconds (3.0), &GenerateTraffic, source15, packetSize,200, Seconds(1.0)); 
  Simulator::Schedule (Seconds (3.2), &GenerateTraffic, source16, packetSize,200, Seconds(1.0)); 
  Simulator::Schedule (Seconds (3.4), &GenerateTraffic, source17, packetSize,200, Seconds(1.0)); 
  Simulator::Schedule (Seconds (3.6), &GenerateTraffic, source18, packetSize,200, Seconds(1.0)); 
  Simulator::Schedule (Seconds (3.8), &GenerateTraffic, source19, packetSize,200, Seconds(1.0)); 
  Simulator::Schedule (Seconds (4.0), &GenerateTraffic, source20, packetSize,200, Seconds(1.0)); 
  Simulator::Schedule (Seconds (4.2), &GenerateTraffic, source21, packetSize,200, Seconds(1.0)); 
  Simulator::Schedule (Seconds (4.4), &GenerateTraffic, source22, packetSize,200, Seconds(1.0)); 
  Simulator::Schedule (Seconds (4.6), &GenerateTraffic, source23, packetSize,200, Seconds(1.0)); 
  Simulator::Schedule (Seconds (10.0), &GenerateTraffic, source2, packetSize,10000, Seconds(0.001));
  Simulator::Schedule (Seconds (30.0), &GenerateTraffic, source10, packetSize,10000, Seconds(0.001));
  Simulator::Schedule (Seconds (50.0), &GenerateTraffic, source14, packetSize,10000, Seconds(0.001));
  Simulator::Schedule (Seconds (70.0), &GenerateTraffic, source23, packetSize,10000, Seconds(0.001));
  Simulator::Schedule (Seconds (90.0), &GenerateTraffic, source16, packetSize,10000, Seconds(0.001));
  Simulator::Schedule (Seconds (110.0), &GenerateTraffic, source5, packetSize,10000, Seconds(0.001));
  Simulator::Schedule (Seconds (130.0), &GenerateTraffic, source9, packetSize,10000, Seconds(0.001));
  Simulator::Schedule (Seconds (150.0), &GenerateTraffic, source18, packetSize,10000, Seconds(0.001));
  Simulator::Schedule (Seconds (170.0), &GenerateTraffic, source12, packetSize,10000, Seconds(0.001));
  Simulator::Schedule (Seconds (190.0), &GenerateTraffic, source19, packetSize,10000, Seconds(0.001));
  
  NS_LOG_UNCOND ("Testing from node " << sourceNode << " to " << sinkNode << " with grid distance " << distance);

  Simulator::Stop (Seconds (201.0));
  //Simulator::Schedule(Seconds(0), &throughput, monitor,&flowmon);
  Simulator::Run ();
  int j=0;
  float AvgThroughput = 0;
  Time Jitter;
  Time Delay;

  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter)
      {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);

  NS_LOG_UNCOND("----Flow ID:" <<iter->first);
  NS_LOG_UNCOND("Src Addr" <<t.sourceAddress << "Dst Addr "<< t.destinationAddress);
  NS_LOG_UNCOND("Sent Packets=" <<iter->second.txPackets);
  NS_LOG_UNCOND("Received Packets =" <<iter->second.rxPackets);
  NS_LOG_UNCOND("Lost Packets =" <<iter->second.txPackets-iter->second.rxPackets);
  NS_LOG_UNCOND("Packet delivery ratio =" <<iter->second.rxPackets*100/iter->second.txPackets << "%");
  NS_LOG_UNCOND("Packet loss ratio =" << (iter->second.txPackets-iter->second.rxPackets)*100/iter->second.txPackets << "%");
  NS_LOG_UNCOND("Delay =" <<iter->second.delaySum);
  NS_LOG_UNCOND("Jitter =" <<iter->second.jitterSum);
  NS_LOG_UNCOND("Throughput =" <<iter->second.rxBytes * 8.0/(iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds())/1024<<"Kbps");

  SentPackets = SentPackets +(iter->second.txPackets);
  ReceivedPackets = ReceivedPackets + (iter->second.rxPackets);
  LostPackets = LostPackets + (iter->second.txPackets-iter->second.rxPackets);
  AvgThroughput = AvgThroughput + (iter->second.rxBytes * 8.0/(iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds())/1024);
  if ((Delay + (iter->second.delaySum))==Delay){
    Delay = Delay*10000000000;
  }
  else {
    Delay = Delay + (iter->second.delaySum);
  }
  if (((Jitter + (iter->second.jitterSum))==Jitter)&&(iter->second.txPackets-iter->second.rxPackets)>10){
    Jitter = Jitter*10000000000;
  }
  else {
    Jitter = Jitter + (iter->second.jitterSum);
  }

  j = j + 1;

  }

  AvgThroughput = AvgThroughput/j;
  NS_LOG_UNCOND("--------Total Results of the simulation----------"<<std::endl);
  NS_LOG_UNCOND("Total sent packets  =" << SentPackets);
  NS_LOG_UNCOND("Total Received Packets =" << ReceivedPackets);
  NS_LOG_UNCOND("Total Lost Packets =" << LostPackets);
  NS_LOG_UNCOND("Packet Loss ratio =" << ((LostPackets*100)/SentPackets)<< "%");
  NS_LOG_UNCOND("Packet delivery ratio =" << ((ReceivedPackets*100)/SentPackets)<< "%");
  NS_LOG_UNCOND("Average Throughput =" << AvgThroughput<< "Kbps");
  NS_LOG_UNCOND("End to End Delay =" << Delay);
  NS_LOG_UNCOND("End to End Jitter delay =" << Jitter);
  NS_LOG_UNCOND("Total Flod id " << j);
  
  
  Simulator::Destroy ();
  outfile.close();

  return 0;
}
