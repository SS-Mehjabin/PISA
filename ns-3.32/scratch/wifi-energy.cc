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

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WifiSimpleAdhocGrid");

static inline std::string
PrintReceivedPacket(Address &from)
{
  InetSocketAddress iaddr = InetSocketAddress::ConvertFrom(from);

  std::ostringstream oss;
  oss << "--\nReceived one packet! Socket: " << iaddr.GetIpv4()
      << " port: " << iaddr.GetPort()
      << " at time = " << Simulator::Now().GetSeconds()
      << "\n--";

  return oss.str();
}

void ReceivePacket(Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom(from)))
  {
    if (packet->GetSize() > 0)
    {
      NS_LOG_UNCOND(PrintReceivedPacket(from));
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

/// Trace function for remaining energy at node.
void RemainingEnergy(double oldValue, double remainingEnergy)
{
  NS_LOG_UNCOND(Simulator::Now().GetSeconds()
                << "s Current remaining energy = " << remainingEnergy << "J");
}

/// Trace function for total energy consumption at node.
void TotalEnergy(double oldValue, double totalEnergy)
{
  NS_LOG_UNCOND(Simulator::Now().GetSeconds()
                << "s Total energy consumed by radio = " << totalEnergy << "J");
}

int main (int argc, char *argv[])
{
  std::string phyMode ("DsssRate1Mbps");
  double distance = 500;  // m
  uint32_t packetSize = 1000; // bytes
  uint32_t numPackets = 10;
  uint32_t numNodes = 4;  // by default, 5x5
  uint32_t sinkNode = 3;
  uint32_t sourceNode = 0;
  double interval = 1.0; // seconds
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
  positionAlloc->Add (Vector (50.0, 50.0, 0.0));
  positionAlloc->Add (Vector (50.0, -50.0, 0.0));
  positionAlloc->Add (Vector (150.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (c);

    /** Energy Model **/
  /***************************************************************************/
  /* energy source */
  BasicEnergySourceHelper basicSourceHelper;
  // configure energy source
  basicSourceHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(0.1));
  // install source
  EnergySourceContainer sources = basicSourceHelper.Install(c);
  /* device energy model */
  WifiRadioEnergyModelHelper radioEnergyHelper;
  // configure radio energy model
  radioEnergyHelper.Set("TxCurrentA", DoubleValue(0.0174));
  // install device model
  DeviceEnergyModelContainer deviceModels = radioEnergyHelper.Install(devices, sources);
  /***************************************************************************/

  // Enable OLSR
  OlsrHelper olsr;
  Ipv4StaticRoutingHelper staticRouting;

  Ipv4ListRoutingHelper list;
  list.Add (staticRouting, 0);
  list.Add (olsr, 10);

  InternetStackHelper internet;
  internet.SetRoutingHelper (list); // has effect on the next Install ()
  internet.Install (c);

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink = Socket::CreateSocket (c.Get (3), tid);
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink->Bind (local);
  recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));

  ////
  Ptr<Socket> recvSink1 = Socket::CreateSocket (c.Get (0), tid);
  InetSocketAddress local1 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink1->Bind (local1);
  recvSink1->SetRecvCallback (MakeCallback (&ReceivePacket));
  
  Ptr<Socket> recvSink2 = Socket::CreateSocket (c.Get (1), tid);
  InetSocketAddress local2 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink2->Bind (local2);
  recvSink2->SetRecvCallback (MakeCallback (&ReceivePacket));
  
  Ptr<Socket> recvSink3 = Socket::CreateSocket (c.Get (2), tid);
  InetSocketAddress local3 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink3->Bind (local3);
  recvSink3->SetRecvCallback (MakeCallback (&ReceivePacket));


  ////

  Ptr<Socket> source = Socket::CreateSocket (c.Get (0), tid);
  InetSocketAddress remote = InetSocketAddress (i.GetAddress (3, 0), 80);
  source->Connect (remote);

  Ptr<Socket> source1 = Socket::CreateSocket (c.Get (0), tid);
  InetSocketAddress remote1 = InetSocketAddress (i.GetAddress (1, 0), 80);
  source1->Connect (remote1);

  Ptr<Socket> source2 = Socket::CreateSocket (c.Get (0), tid);
  InetSocketAddress remote2 = InetSocketAddress (i.GetAddress (2, 0), 80);
  source2->Connect (remote2);


  Ptr<Socket> source4 = Socket::CreateSocket (c.Get (1), tid);
  InetSocketAddress remote4 = InetSocketAddress (i.GetAddress (3, 0), 80);
  source4->Connect (remote4);

  Ptr<Socket> source5 = Socket::CreateSocket (c.Get (1), tid);
  InetSocketAddress remote5 = InetSocketAddress (i.GetAddress (0, 0), 80);
  source5->Connect (remote5);

  Ptr<Socket> source6 = Socket::CreateSocket (c.Get (1), tid);
  InetSocketAddress remote6 = InetSocketAddress (i.GetAddress (2, 0), 80);
  source6->Connect (remote6);

  Ptr<Socket> source8 = Socket::CreateSocket (c.Get (2), tid);
  InetSocketAddress remote8 = InetSocketAddress (i.GetAddress (3, 0), 80);
  source8->Connect (remote8);

  Ptr<Socket> source9 = Socket::CreateSocket (c.Get (2), tid);
  InetSocketAddress remote9 = InetSocketAddress (i.GetAddress (0, 0), 80);
  source9->Connect (remote9);

  Ptr<Socket> source10 = Socket::CreateSocket (c.Get (2), tid);
  InetSocketAddress remote10 = InetSocketAddress (i.GetAddress (1, 0), 80);
  source10->Connect (remote10);

  Ptr<Socket> source12 = Socket::CreateSocket (c.Get (3), tid);
  InetSocketAddress remote12 = InetSocketAddress (i.GetAddress (1, 0), 80);
  source12->Connect (remote12);

  Ptr<Socket> source13 = Socket::CreateSocket (c.Get (3), tid);
  InetSocketAddress remote13 = InetSocketAddress (i.GetAddress (0, 0), 80);
  source13->Connect (remote13);

  Ptr<Socket> source14 = Socket::CreateSocket (c.Get (3), tid);
  InetSocketAddress remote14 = InetSocketAddress (i.GetAddress (2, 0), 80);
  source14->Connect (remote14);

  Ptr<BasicEnergySource> basicSourcePtr = DynamicCast<BasicEnergySource>(sources.Get(2));
  basicSourcePtr->TraceConnectWithoutContext("RemainingEnergy", MakeCallback(&RemainingEnergy));

  // Ptr<DeviceEnergyModel> basicRadioModelPtr =
  //     basicSourcePtr->FindDeviceEnergyModels("ns3::WifiRadioEnergyModel").Get(0);
  // NS_ASSERT(basicRadioModelPtr != NULL);
  // basicRadioModelPtr->TraceConnectWithoutContext("TotalEnergyConsumption", MakeCallback(&TotalEnergy));

  ////

  if (tracing == true)
    {
      AsciiTraceHelper ascii;
      wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("wifi-simple-adhoc-grid.tr"));
      wifiPhy.EnablePcap ("wifi-simple-adhoc-grid", devices);
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
  Simulator::Schedule (Seconds (35.0), &GenerateTraffic,
                       source1, packetSize, numPackets, interPacketInterval);
  Simulator::Schedule (Seconds (40.0), &GenerateTraffic,
                       source2, packetSize, numPackets, interPacketInterval);
  // Simulator::Schedule (Seconds (30.0), &GenerateTraffic,
  //                      source3, packetSize, numPackets, interPacketInterval);
  Simulator::Schedule (Seconds (45.0), &GenerateTraffic,
                       source4, packetSize, numPackets, interPacketInterval);
  Simulator::Schedule (Seconds (50.0), &GenerateTraffic,
                       source5, packetSize, numPackets, interPacketInterval);
  Simulator::Schedule (Seconds (55.0), &GenerateTraffic,
                       source6, packetSize, numPackets, interPacketInterval);
  // Simulator::Schedule (Seconds (30.0), &GenerateTraffic,
  //                      source7, packetSize, numPackets, interPacketInterval);
  Simulator::Schedule (Seconds (60.0), &GenerateTraffic,
                       source8, packetSize, numPackets, interPacketInterval);
  Simulator::Schedule (Seconds (65.0), &GenerateTraffic,
                       source9, packetSize, numPackets, interPacketInterval);
  Simulator::Schedule (Seconds (70.0), &GenerateTraffic,
                       source10, packetSize, numPackets, interPacketInterval);
  // Simulator::Schedule (Seconds (30.0), &GenerateTraffic,
  //                      source11, packetSize, numPackets, interPacketInterval);
  Simulator::Schedule (Seconds (75.0), &GenerateTraffic,
                       source12, packetSize, numPackets, interPacketInterval);
  Simulator::Schedule (Seconds (80.0), &GenerateTraffic,
                       source13, packetSize, numPackets, interPacketInterval);
  Simulator::Schedule (Seconds (85.0), &GenerateTraffic,
                       source14, packetSize, numPackets, interPacketInterval);
  // Simulator::Schedule (Seconds (30.0), &GenerateTraffic,
  //                      source15, packetSize, numPackets, interPacketInterval);
  // Simulator::Schedule (Seconds (30.0), &GenerateTraffic,
  //                      source16, packetSize, numPackets, interPacketInterval);
  // Simulator::Schedule (Seconds (30.0), &GenerateTraffic,
  //                      source17, packetSize, numPackets, interPacketInterval);
  // Simulator::Schedule (Seconds (30.0), &GenerateTraffic,
  //                      source18, packetSize, numPackets, interPacketInterval);
  // Simulator::Schedule (Seconds (30.0), &GenerateTraffic,
  //                      source19, packetSize, numPackets, interPacketInterval);


  // Output what we are doing
  NS_LOG_UNCOND ("Testing from node " << sourceNode << " to " << sinkNode << " with grid distance " << distance);

  Simulator::Stop (Seconds (100.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

