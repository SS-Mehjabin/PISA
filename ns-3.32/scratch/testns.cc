/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

//
// This is an illustration of how one could use virtualization techniques to
// allow running applications on virtual machines talking over simulated
// networks.
//
// The actual steps required to configure the virtual machines can be rather
// involved, so we don't go into that here.  Please have a look at one of
// our HOWTOs on the nsnam wiki for more details about how to get the 
// system confgured.  For an example, have a look at "HOWTO Use Linux 
// Containers to set up virtual networks" which uses this code as an 
// example.
//
// The configuration you are after is explained in great detail in the 
// HOWTO, but looks like the following:
//
//  +----------+                           +----------+
//  | virtual  |                           | virtual  |
//  |  Linux   |                           |  Linux   |
//  |   Host   |                           |   Host   |
//  |          |                           |          |
//  |   eth0   |                           |   eth0   |
//  +----------+                           +----------+
//       |                                      |
//  +----------+                           +----------+
//  |  Linux   |                           |  Linux   |
//  |  Bridge  |                           |  Bridge  |
//  +----------+                           +----------+
//       |                                      |
//  +------------+                       +-------------+
//  | "tap-left" |                       | "tap-right" |
//  +------------+                       +-------------+
//       |           n0            n1           |
//       |       +--------+    +--------+       |
//       +-------|  tap   |    |  tap   |-------+
//               | bridge |    | bridge |
//               +--------+    +--------+
//               |  wifi  |    |  wifi  |
//               +--------+    +--------+
//                   |             |
//                 ((*))         ((*))
//
//                       Wifi LAN
//
//                        ((*))
//                          |
//                     +--------+
//                     |  wifi  |
//                     +--------+
//                     | access |
//                     |  point |
//                     +--------+
//
#include <iostream>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/tap-bridge-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TapWifiVirtualMachineExample");

int 
main (int argc, char *argv[])
{
  CommandLine cmd (__FILE__);
  cmd.Parse (argc, argv);

  //
  // We are interacting with the outside, real, world.  This means we have to 
  // interact in real-time and therefore means we have to use the real-time
  // simulator and take the time to calculate checksums.
  //
  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
  //
  // Create two ghost nodes.  The first will represent the virtual machine host
  // on the left side of the network; and the second will represent the VM on 
  // the right side.
  //
  NodeContainer nodesLeft;
  nodesLeft.Create (7);

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());

  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211a);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate54Mbps"));

  //
  // No reason for pesky access points, so we'll use an ad-hoc network.
  //
  WifiMacHelper wifiMac;
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer devicesLeft = wifi.Install (wifiPhy, wifiMac, nodesLeft);
  //
  // We need location information since we are talking about wifi, so add a
  // constant position to the ghost nodes.
  //
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (5.0, 0.0, 0.0));
  positionAlloc->Add (Vector (10.0, 0.0, 0.0));
  positionAlloc->Add (Vector (15.0, 0.0, 0.0));
  positionAlloc->Add (Vector (20.0, 0.0, 0.0));
  positionAlloc->Add (Vector (25.0, 0.0, 0.0));
  positionAlloc->Add (Vector (30.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodesLeft);

  //
  // Use the TapBridgeHelper to connect to the pre-configured tap devices for 
  // the left side.  We go with "UseLocal" mode since the wifi devices do not
  // support promiscuous mode (because of their natures0.  This is a special
  // case mode that allows us to extend a linux bridge into ns-3 IFF we will
  // only see traffic from one other device on that bridge.  That is the case
  // for this configuration.
  //

  InternetStackHelper internetLeft;
  internetLeft.Install (nodesLeft);

  Ipv4AddressHelper ipv4Left;
  ipv4Left.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfacesLeft = ipv4Left.Assign (devicesLeft);

  TapBridgeHelper tapBridge1 (interfacesLeft.GetAddress (0));
  tapBridge1.SetAttribute ("Mode", StringValue ("UseLocal"));
  tapBridge1.SetAttribute ("DeviceName", StringValue ("tap-left"));
  tapBridge1.Install (nodesLeft.Get (0), devicesLeft.Get (0));


  TapBridgeHelper tapBridge2 (interfacesLeft.GetAddress (1));
  tapBridge2.SetAttribute ("Mode", StringValue ("UseLocal"));
  tapBridge2.SetAttribute ("DeviceName", StringValue ("tap-right"));
  tapBridge2.Install (nodesLeft.Get (1), devicesLeft.Get (1));

  //
  // Run the simulation for ten minutes to give the user time to play around
  //
  Simulator::Stop (Seconds (600.));
  Simulator::Run ();
  Simulator::Destroy ();
}
