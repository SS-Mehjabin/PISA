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
  uint32_t packetSize = 1024*4; // bytes
  uint32_t numPackets = 10;
  uint32_t numNodes = 4;  // by default, 5x5
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
  positionAlloc->Add (Vector (0.0, 30.0, 0.0));
  positionAlloc->Add (Vector (0.0, 120.0, 0.0));
  positionAlloc->Add (Vector (0.0, 180.0, 0.0));
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


  Ipv4StaticRoutingHelper staticRouting;
  InternetStackHelper internet;
  internet.Install (not_malicious);

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();
  monitor->Start(Seconds(1.0));
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
  



  ////

  // Ptr<Socket> source = Socket::CreateSocket (c.Get (0), tid);
  // InetSocketAddress remote = InetSocketAddress (Ipv4Address("10.2.1.4"), 80);
  // source->Connect (remote);

  Ptr<Socket> source = Socket::CreateSocket (c.Get (0), tid);
  InetSocketAddress remote = InetSocketAddress (interfaces.GetAddress (3, 0), 80);
  source->Connect (remote);

  Ptr<Socket> source1 = Socket::CreateSocket (c.Get (3), tid);
  InetSocketAddress remote1 = InetSocketAddress (interfaces.GetAddress (0, 0), 80);
  source1->Connect (remote1);



  Ptr<Ipv4StaticRouting> staticRoute = staticRouting.GetStaticRouting(c.Get(3)->GetObject<Ipv4>());
  staticRoute->AddHostRouteTo(interfaces.GetAddress(0), interfaces.GetAddress(2), 1, 0);
  Ptr<Ipv4StaticRouting> staticRoute1 = staticRouting.GetStaticRouting(c.Get(2)->GetObject<Ipv4>());
  staticRoute1->AddHostRouteTo(interfaces.GetAddress(0), interfaces.GetAddress(1), 1, 0);
  

  Ptr<Ipv4StaticRouting> staticRoute2 = staticRouting.GetStaticRouting(c.Get(0)->GetObject<Ipv4>());
  staticRoute2->AddHostRouteTo(interfaces.GetAddress(3), interfaces.GetAddress(1), 1, 0);
  Ptr<Ipv4StaticRouting> staticRoute3 = staticRouting.GetStaticRouting(c.Get(1)->GetObject<Ipv4>());
  staticRoute3->AddHostRouteTo(interfaces.GetAddress(3), interfaces.GetAddress(2), 1, 0);

//   Ptr<Ipv4StaticRouting> staticRoute4 = staticRouting.GetStaticRouting(c.Get(0)->GetObject<Ipv4>());
//   staticRoute4->AddHostRouteTo(interfaces.GetAddress(1), interfaces.GetAddress(2), 1, 0);
//   Ptr<Ipv4StaticRouting> staticRoute5 = staticRouting.GetStaticRouting(c.Get(0)->GetObject<Ipv4>());
//   staticRoute5->AddHostRouteTo(interfaces.GetAddress(0), interfaces.GetAddress(1), 1, 0);

  // // Ipv4StaticRoutingHelper staticRouting1;
  // Ptr<Ipv4StaticRouting> staticRoute2 = staticRouting.GetStaticRouting(c.Get(4)->GetObject<Ipv4>());
  // staticRoute2->AddHostRouteTo(interfaces.GetAddress(5), interfaces.GetAddress(3), 1, 0);

  // // Ipv4StaticRoutingHelper staticRouting2;
  // Ptr<Ipv4StaticRouting> staticRoute3 = staticRouting.GetStaticRouting(c.Get(0)->GetObject<Ipv4>());
  // staticRoute3->AddHostRouteTo(interfaces.GetAddress(5), interfaces.GetAddress(3), 1, 0);

  ////

  if (tracing == true)
    {
      wifiPhy.EnablePcap ("selective", devices);
      // To do-- enable an IP-level trace that shows forwarding events only
    }


  // Simulator::Schedule (Seconds (30.0), &GenerateTraffic,
  //                      source, packetSize, numPackets, interPacketInterval);
  Simulator::Schedule (Seconds (2.0), &GenerateTraffic,
                       source, packetSize, numPackets, interPacketInterval);
  Simulator::Schedule (Seconds (12.0), &GenerateTraffic,
                       source1, packetSize, numPackets, interPacketInterval);
  // Output what we are doing
  NS_LOG_UNCOND ("Testing from node " << sourceNode << " to " << sinkNode << " with grid distance " << distance);

  Simulator::Stop (Seconds (50.0));

  Simulator::Schedule(Seconds(1.0), &throughput, monitor,&flowmon);
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
