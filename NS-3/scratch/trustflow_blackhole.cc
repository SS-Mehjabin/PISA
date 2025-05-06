/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 University of Washington
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
 */

//
// This program configures a grid (default 5x5) of nodes on an
// 802.11b physical layer, with
// 802.11b NICs in adhoc mode, and by default, sends one packet of 1000
// (application) bytes to node 1.
//
// The default layout is like this, on a 2-D grid.
//
// n20  n21  n22  n23  n24
// n15  n16  n17  n18  n19
// n10  n11  n12  n13  n14
// n5   n6   n7   n8   n9
// n0   n1   n2   n3   n4
//
// the layout is affected by the parameters given to GridPositionAllocator;
// by default, GridWidth is 5 and numNodes is 25..
//
// There are a number of command-line options available to control
// the default behavior.  The list of available command-line options
// can be listed with the following command:
// ./waf --run "wifi-simple-adhoc-grid --help"
//
// Note that all ns-3 attributes (not just the ones exposed in the below
// script) can be changed at command line; see the ns-3 documentation.
//
// For instance, for this configuration, the physical layer will
// stop successfully receiving packets when distance increases beyond
// the default of 500m.
// To see this effect, try running:
//
// ./waf --run "wifi-simple-adhoc-grid --distance=500"
// ./waf --run "wifi-simple-adhoc-grid --distance=1000"
// ./waf --run "wifi-simple-adhoc-grid --distance=1500"
//
// The source node and sink node can be changed like this:
//
// ./waf --run "wifi-simple-adhoc-grid --sourceNode=20 --sinkNode=10"
//
// This script can also be helpful to put the Wifi layer into verbose
// logging mode; this command will turn on all wifi logging:
//
// ./waf --run "wifi-simple-adhoc-grid --verbose=1"
//
// By default, trace file writing is off-- to enable it, try:
// ./waf --run "wifi-simple-adhoc-grid --tracing=1"
//
// When you are done tracing, you will notice many pcap trace files
// in your directory.  If you have tcpdump installed, you can try this:
//
// tcpdump -r wifi-simple-adhoc-grid-0-0.pcap -nn -tt
//

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
std::ofstream outfile ("aodv_energy_output_flood.txt");
std::ofstream outfile2 ("trustflow_blackhole.csv");


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
  if (blahh>4) blahh=0;
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
    outfile2 << Simulator::Now ().GetSeconds ()<<","<< SentPackets<<","<< LostPackets<<"\n";
// Reschedule the method after 1sec
Simulator::Schedule(Seconds(1.0), &throughput, monitor, flowmon);
}

int main (int argc, char *argv[])
{
  std::string phyMode ("DsssRate1Mbps");
  double distance = 500;  // m
  uint32_t packetSize = 1024; // bytes
  uint32_t numPackets = 100;
  uint32_t numNodes = 6;  // by default, 5x5
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
  not_malicious.Add(c.Get(1));
  not_malicious.Add(c.Get(2));
  not_malicious.Add(c.Get(0));
  not_malicious.Add(c.Get(4));
  not_malicious.Add(c.Get(5));
  NodeContainer malicious;
  malicious.Add(c.Get(3));

  // The below set of helpers will help us to put together the wifi NICs we want
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
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (50.0, -30.0, 0.0));
  positionAlloc->Add (Vector (90.0, 0.0, 0.0));
  positionAlloc->Add (Vector (50.0, 20.0, 0.0));
  positionAlloc->Add (Vector (100.0, 70.0, 0.0));
  positionAlloc->Add (Vector (110.0, -60.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
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

  AodvHelper malaodv;
  malaodv.Set("IsMalicious",BooleanValue(true));
  internet.SetRoutingHelper (malaodv); // has effect on the next Install ()
  internet.Install (malicious);

  // internet.SetRoutingHelper (aodv); // has effect on the next Install ()
  // internet.Install (malicious);


  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();
  monitor->Start(Seconds(30.0));
  // monitor->Stop(Seconds(331.0));

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = ipv4.Assign (devices);

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink = Socket::CreateSocket (c.Get (0), tid);
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink->Bind (local);
  recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));

  ////
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



  ////

  // Ptr<Socket> source = Socket::CreateSocket (c.Get (0), tid);
  // InetSocketAddress remote = InetSocketAddress (Ipv4Address("10.2.1.4"), 80);
  // source->Connect (remote);

  Ptr<Socket> source = Socket::CreateSocket (c.Get (0), tid);
  InetSocketAddress remote = InetSocketAddress (interfaces.GetAddress (1, 0), 80);
  source->Connect (remote);

  Ptr<Socket> source1 = Socket::CreateSocket (c.Get (0), tid);
  InetSocketAddress remote1 = InetSocketAddress (interfaces.GetAddress (2, 0), 80);
  source1->Connect (remote1);

  Ptr<Socket> source2 = Socket::CreateSocket (c.Get (0), tid);
  InetSocketAddress remote2 = InetSocketAddress (interfaces.GetAddress (3, 0), 80);
  source2->Connect (remote2);


  Ptr<Socket> source3 = Socket::CreateSocket (c.Get (0), tid);
  InetSocketAddress remote3 = InetSocketAddress (interfaces.GetAddress (4, 0), 80);
  source3->Connect (remote3);

  Ptr<Socket> source4 = Socket::CreateSocket (c.Get (1), tid);
  InetSocketAddress remote4 = InetSocketAddress (interfaces.GetAddress (0, 0), 80);
  source4->Connect (remote4);

  Ptr<Socket> source5 = Socket::CreateSocket (c.Get (1), tid);
  InetSocketAddress remote5 = InetSocketAddress (interfaces.GetAddress (2, 0), 80);
  source5->Connect (remote5);

  Ptr<Socket> source6 = Socket::CreateSocket (c.Get (1), tid);
  InetSocketAddress remote6 = InetSocketAddress (interfaces.GetAddress (5, 0), 80);
  source6->Connect (remote6);

  Ptr<Socket> source7 = Socket::CreateSocket (c.Get (2), tid);
  InetSocketAddress remote7 = InetSocketAddress (interfaces.GetAddress (0, 0), 80);
  source7->Connect (remote7);

  Ptr<Socket> source8 = Socket::CreateSocket (c.Get (2), tid);
  InetSocketAddress remote8 = InetSocketAddress (interfaces.GetAddress (1, 0), 80);
  source8->Connect (remote8);

  Ptr<Socket> source9 = Socket::CreateSocket (c.Get (2), tid);
  InetSocketAddress remote9 = InetSocketAddress (interfaces.GetAddress (3, 0), 80);
  source9->Connect (remote9);

  Ptr<Socket> source10 = Socket::CreateSocket (c.Get (2), tid);
  InetSocketAddress remote10 = InetSocketAddress (interfaces.GetAddress (4, 0), 80);
  source10->Connect (remote10);

  Ptr<Socket> source11 = Socket::CreateSocket (c.Get (2), tid);
  InetSocketAddress remote11 = InetSocketAddress (interfaces.GetAddress (5, 0), 80);
  source11->Connect (remote11);

  Ptr<Socket> source12 = Socket::CreateSocket (c.Get (3), tid);
  InetSocketAddress remote12 = InetSocketAddress (interfaces.GetAddress (0, 0), 80);
  source12->Connect (remote12);

  Ptr<Socket> source13 = Socket::CreateSocket (c.Get (3), tid);
  InetSocketAddress remote13 = InetSocketAddress (interfaces.GetAddress (2, 0), 80);
  source13->Connect (remote13);

  Ptr<Socket> source14 = Socket::CreateSocket (c.Get (4), tid);
  InetSocketAddress remote14 = InetSocketAddress (interfaces.GetAddress (0, 0), 80);
  source14->Connect (remote14);

  Ptr<Socket> source15 = Socket::CreateSocket (c.Get (4), tid);
  InetSocketAddress remote15 = InetSocketAddress (interfaces.GetAddress (2, 0), 80);
  source15->Connect (remote15);

  Ptr<Socket> source16 = Socket::CreateSocket (c.Get (5), tid);
  InetSocketAddress remote16 = InetSocketAddress (interfaces.GetAddress (1, 0), 80);
  source16->Connect (remote16);

  Ptr<Socket> source17 = Socket::CreateSocket (c.Get (5), tid);
  InetSocketAddress remote17 = InetSocketAddress (interfaces.GetAddress (2, 0), 80);
  source17->Connect (remote17);

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
      wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("aodv_trace.tr"));
      wifiPhy.EnablePcap ("selective", devices);
      // Trace routing tables
      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("wifi-simple-adhoc-grid.routes", std::ios::out);
      aodv.PrintRoutingTableAllEvery (Seconds (2), routingStream);
      Ptr<OutputStreamWrapper> neighborStream = Create<OutputStreamWrapper> ("wifi-simple-adhoc-grid.neighbors", std::ios::out);
      aodv.PrintNeighborCacheAllEvery (Seconds (2), neighborStream);

      // To do-- enable an IP-level trace that shows forwarding events only
    }


  for (uint32_t i = 0; i < numNodes; i++)
    {

      for (uint32_t j = 0; j < numNodes; j++)
        {
          if (i == ( (numNodes - 1) - j))
            {
              continue;
            }
          V4PingHelper ping (interfaces.GetAddress ( (numNodes - 1) - j));
          ping.SetAttribute ("Verbose",
                             BooleanValue (false));

          ApplicationContainer p = ping.Install (c.Get (i));
          p.Start (Seconds (0 + (i * 2)));
          p.Stop (Seconds (15) - Seconds (0.001));
        }

    }
  // Give OLSR time to converge-- 30 seconds perhaps
  // Simulator::Schedule (Seconds (30.0), &GenerateTraffic,
  //                      source, packetSize, numPackets, interPacketInterval);
  Simulator::Schedule (Seconds (30.0), &GenerateTraffic,
                       source, packetSize, numPackets, interPacketInterval);
  Simulator::Schedule (Seconds (37.0), &GenerateTraffic,
                       source4, packetSize, numPackets, interPacketInterval);
  Simulator::Schedule (Seconds (50.0), &GenerateTraffic,
                       source1, packetSize, numPackets, interPacketInterval);
  Simulator::Schedule (Seconds (57.0), &GenerateTraffic,
                       source7, packetSize, numPackets, interPacketInterval);
  Simulator::Schedule (Seconds (30.0), &GenerateTraffic,
                       source2, packetSize, 680, Seconds(0.25));
  Simulator::Schedule (Seconds (30.1), &GenerateTraffic,
                       source12, packetSize, 680, Seconds(0.25));
  Simulator::Schedule (Seconds (100.0), &GenerateTraffic,
                       source3, packetSize, 400, Seconds(0.25));
  Simulator::Schedule (Seconds (100.0), &GenerateTraffic,
                       source14, packetSize, 400, Seconds(0.25));
  Simulator::Schedule (Seconds (110.0), &GenerateTraffic,
                       source5, packetSize, numPackets, interPacketInterval);
  Simulator::Schedule (Seconds (117.0), &GenerateTraffic,
                       source8, packetSize, numPackets, interPacketInterval);
  Simulator::Schedule (Seconds (130.0), &GenerateTraffic,
                       source6, packetSize, numPackets, interPacketInterval);
  Simulator::Schedule (Seconds (137.0), &GenerateTraffic,
                       source16, packetSize, numPackets, interPacketInterval);
  Simulator::Schedule (Seconds (150.0), &GenerateTraffic,
                       source9, packetSize, numPackets, interPacketInterval);
  Simulator::Schedule (Seconds (157.0), &GenerateTraffic,
                       source13, packetSize, numPackets, interPacketInterval);
  Simulator::Schedule (Seconds (170.0), &GenerateTraffic,
                       source10, packetSize, numPackets, interPacketInterval);
  Simulator::Schedule (Seconds (177.0), &GenerateTraffic,
                       source15, packetSize, numPackets, interPacketInterval);
  Simulator::Schedule (Seconds (190.0), &GenerateTraffic,
                       source11, packetSize, numPackets, interPacketInterval);
  Simulator::Schedule (Seconds (197.0), &GenerateTraffic,
                       source17, packetSize, numPackets, interPacketInterval);
  // Simulator::Schedule (Seconds (100.0), &GenerateTraffic,
  //                      source12, packetSize, 50000, Seconds(0.001));

  // Output what we are doing
  NS_LOG_UNCOND ("Testing from node " << sourceNode << " to " << sinkNode << " with grid distance " << distance);

  Simulator::Stop (Seconds (207.0));

  Simulator::Schedule(Seconds(30.0), &throughput, monitor,&flowmon);
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
