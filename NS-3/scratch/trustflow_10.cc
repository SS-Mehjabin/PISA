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
#include "ns3/olsr-helper.h"
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
#include <stdio.h>
int blahh=0;
std::ofstream outfile ("energy_output_10.txt");


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

int main (int argc, char *argv[])
{
  std::string phyMode ("DsssRate1Mbps");
  double distance = 500;  // m
  uint32_t packetSize = 1024; // bytes
  uint32_t numPackets = 2000;
  uint32_t numNodes = 10;  // by default, 5x5
  uint32_t sinkNode = 3;
  uint32_t sourceNode = 0;
  
  uint32_t SentPackets = 0;
  uint32_t ReceivedPackets = 0;
  uint32_t LostPackets = 0;
  
  double interval = 0.25; // seconds
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
  positionAlloc->Add (Vector (20.0, 50.0, 0.0));
  positionAlloc->Add (Vector (20.0, 15.0, 0.0));
  positionAlloc->Add (Vector (20.0, -30.0, 0.0));
  positionAlloc->Add (Vector (40.0, -60.0, 0.0));
  positionAlloc->Add (Vector (40.0, 0.0, 0.0));
  positionAlloc->Add (Vector (50.0, 30.0, 0.0));
  positionAlloc->Add (Vector (60.0, -30.0, 0.0));
  positionAlloc->Add (Vector (70.0, 30.0, 0.0));
  positionAlloc->Add (Vector (80.0, 0.0, 0.0));
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
  OlsrHelper olsr;
//   Ipv4StaticRoutingHelper staticRouting;

//   Ipv4ListRoutingHelper list;
//   list.Add (staticRouting, 0);
//   list.Add (olsr, 10);

  InternetStackHelper internet;
  internet.SetRoutingHelper (olsr); // has effect on the next Install ()
  internet.Install (c);


  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

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



  ////

  Ptr<Socket> source = Socket::CreateSocket (c.Get (6), tid);
  InetSocketAddress remote = InetSocketAddress (i.GetAddress (5, 0), 80);
  source->Connect (remote);

  // Ptr<Socket> source1 = Socket::CreateSocket (c.Get (0), tid);
  // InetSocketAddress remote1 = InetSocketAddress (i.GetAddress (2, 0), 80);
  // source1->Connect (remote1);

  // Ptr<Socket> source2 = Socket::CreateSocket (c.Get (0), tid);
  // InetSocketAddress remote2 = InetSocketAddress (i.GetAddress (3, 0), 80);
  // source2->Connect (remote2);


  // Ptr<Socket> source3 = Socket::CreateSocket (c.Get (0), tid);
  // InetSocketAddress remote3 = InetSocketAddress (i.GetAddress (4, 0), 80);
  // source3->Connect (remote3);

  // Ptr<Socket> source4 = Socket::CreateSocket (c.Get (1), tid);
  // InetSocketAddress remote4 = InetSocketAddress (i.GetAddress (0, 0), 80);
  // source4->Connect (remote4);

  // Ptr<Socket> source5 = Socket::CreateSocket (c.Get (1), tid);
  // InetSocketAddress remote5 = InetSocketAddress (i.GetAddress (2, 0), 80);
  // source5->Connect (remote5);

  // Ptr<Socket> source6 = Socket::CreateSocket (c.Get (1), tid);
  // InetSocketAddress remote6 = InetSocketAddress (i.GetAddress (5, 0), 80);
  // source6->Connect (remote6);

  // Ptr<Socket> source7 = Socket::CreateSocket (c.Get (2), tid);
  // InetSocketAddress remote7 = InetSocketAddress (i.GetAddress (0, 0), 80);
  // source7->Connect (remote7);

  // Ptr<Socket> source8 = Socket::CreateSocket (c.Get (2), tid);
  // InetSocketAddress remote8 = InetSocketAddress (i.GetAddress (1, 0), 80);
  // source8->Connect (remote8);

  // Ptr<Socket> source9 = Socket::CreateSocket (c.Get (2), tid);
  // InetSocketAddress remote9 = InetSocketAddress (i.GetAddress (3, 0), 80);
  // source9->Connect (remote9);

  // Ptr<Socket> source10 = Socket::CreateSocket (c.Get (2), tid);
  // InetSocketAddress remote10 = InetSocketAddress (i.GetAddress (4, 0), 80);
  // source10->Connect (remote10);

  // Ptr<Socket> source11 = Socket::CreateSocket (c.Get (2), tid);
  // InetSocketAddress remote11 = InetSocketAddress (i.GetAddress (5, 0), 80);
  // source11->Connect (remote11);

  // Ptr<Socket> source12 = Socket::CreateSocket (c.Get (3), tid);
  // InetSocketAddress remote12 = InetSocketAddress (i.GetAddress (0, 0), 80);
  // source12->Connect (remote12);

  // Ptr<Socket> source13 = Socket::CreateSocket (c.Get (3), tid);
  // InetSocketAddress remote13 = InetSocketAddress (i.GetAddress (2, 0), 80);
  // source13->Connect (remote13);

  // Ptr<Socket> source14 = Socket::CreateSocket (c.Get (4), tid);
  // InetSocketAddress remote14 = InetSocketAddress (i.GetAddress (0, 0), 80);
  // source14->Connect (remote14);

  // Ptr<Socket> source15 = Socket::CreateSocket (c.Get (4), tid);
  // InetSocketAddress remote15 = InetSocketAddress (i.GetAddress (2, 0), 80);
  // source15->Connect (remote15);

  // Ptr<Socket> source16 = Socket::CreateSocket (c.Get (5), tid);
  // InetSocketAddress remote16 = InetSocketAddress (i.GetAddress (1, 0), 80);
  // source16->Connect (remote16);

  // Ptr<Socket> source17 = Socket::CreateSocket (c.Get (5), tid);
  // InetSocketAddress remote17 = InetSocketAddress (i.GetAddress (2, 0), 80);
  // source17->Connect (remote17);

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


  ////

  if (tracing == true)
    {
      AsciiTraceHelper ascii;
      wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("wifi-simple-adhoc-grid.tr"));
      wifiPhy.EnablePcap ("trustflow_10", devices);
      // Trace routing tables
      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("wifi-simple-adhoc-grid.routes", std::ios::out);
      olsr.PrintRoutingTableAllEvery (Seconds (2), routingStream);
      Ptr<OutputStreamWrapper> neighborStream = Create<OutputStreamWrapper> ("wifi-simple-adhoc-grid.neighbors", std::ios::out);
      olsr.PrintNeighborCacheAllEvery (Seconds (2), neighborStream);

      // To do-- enable an IP-level trace that shows forwarding events only
    }

  // Give OLSR time to converge-- 30 seconds perhaps
  Simulator::Schedule (Seconds (30.0), &GenerateTraffic,
                       source, packetSize, numPackets, interPacketInterval);


  // Output what we are doing
  NS_LOG_UNCOND ("Testing from node " << sourceNode << " to " << sinkNode << " with grid distance " << distance);

  Simulator::Stop (Seconds (540.0));
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
  Delay = Delay + (iter->second.delaySum);
  Jitter = Jitter + (iter->second.jitterSum);

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