/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 Sri Lanka Institute of Information Technology
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
 * Author: Jude Niroshan <jude.niroshan11@gmail.com>
 */

#include <iostream>
#include <cmath>
#include <fstream>
#include <bits/stdc++.h> 


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
#include "ns3/trust-aodv-routing-protocol.h"


using namespace ns3;
using namespace aodv;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("TapWifiVirtualMachineExample");

int 
main (int argc, char *argv[])
{
  CommandLine cmd (__FILE__);
  cmd.Parse (argc, argv);

  // LogComponentEnable ("SimpleAodvTrustManager", LOG_LEVEL_INFO);

  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
  
  NodeContainer nodes;
  nodes.Create (7); 

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
  positionAlloc->Add (Vector (15.0, 0.0, 0.0));
  positionAlloc->Add (Vector (20.0, 0.0, 0.0));
  positionAlloc->Add (Vector (25.0, 0.0, 0.0));
  positionAlloc->Add (Vector (30.0, 0.0, 0.0));
  positionAlloc->Add (Vector (10.0, 0.0, 0.0));
  positionAlloc->Add (Vector (5.0, 0.0, 0.0));
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));

  // positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  // positionAlloc->Add (Vector (5.0, 0.0, 0.0));
  // positionAlloc->Add (Vector (25.0, 0.0, 0.0));
  // positionAlloc->Add (Vector (45.0, 0.0, 0.0));
  // positionAlloc->Add (Vector (45.0, 20.0, 0.0));
  // positionAlloc->Add (Vector (25.0, 20.0, 0.0));
  // positionAlloc->Add (Vector (5.0, 20.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);

  NodeContainer normalNodes;
  normalNodes.Add (nodes.Get(1));
  normalNodes.Add (nodes.Get(2));
  normalNodes.Add (nodes.Get(3));
  normalNodes.Add (nodes.Get(4));
  normalNodes.Add (nodes.Get(5));
  normalNodes.Add (nodes.Get(6));

  NodeContainer physicalNodes;
  physicalNodes.Add (nodes.Get(0));
  


  TrustAodvHelper aodv;
  InternetStackHelper stack;
  stack.SetRoutingHelper (aodv);
  // stack.Install (nodes);
  stack.Install (normalNodes);

// If we want to install trust manager to all nodes (Local host can not ping simulated nodes)
  // TrustAodvHelper aodv1;
  // InternetStackHelper stack1;
  // stack1.SetRoutingHelper (aodv1);
  // stack1.Install (physicalNodes);

// If we don't want to install trust manager physical nodes (Local host can ping simulated nodes, But RPi cannot.) 
// The ping from Rpi gets transmitted to the simulated node but reply is not received. Include ** ip route add 10.1.1.0/24 via 10.0.1.3 (local host ip) to Rpi
// Problem: No static route introducing 10.0.1.4 (Rpi) to simulated nodes.
  AodvHelper aodv1;
  InternetStackHelper stack1;
  stack1.SetRoutingHelper (aodv1);
  stack1.Install (physicalNodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  TapBridgeHelper tapBridge1 (interfaces.GetAddress (0));
  tapBridge1.SetAttribute ("Mode", StringValue ("UseLocal"));
  tapBridge1.SetAttribute ("DeviceName", StringValue ("tap-right"));
  tapBridge1.Install (nodes.Get (0), devices.Get (0));

  // TapBridgeHelper tapBridge2 (interfaces.GetAddress (1));
  // tapBridge2.SetAttribute ("Mode", StringValue ("UseLocal"));
  // tapBridge2.SetAttribute ("DeviceName", StringValue ("tap-left"));
  // tapBridge2.Install (nodes.Get (1), devices.Get (1));
  
  wifiPhy.EnablePcapAll (std::string ("aodv7"));

  for(int oo=0; oo<80; oo=oo+5)
    {
      TrustManagerHelper trustManagerHelper;
      string str1="ATrust", str3=".routes";
      stringstream ss;
      ss << oo;
      string str2 = ss.str();

      Ptr<OutputStreamWrapper> trustRoutingStream = Create<OutputStreamWrapper> (str1+str2+str3, std::ios::out);
      trustManagerHelper.PrintTrustTableAllAt (Seconds (oo), trustRoutingStream);
    }

  for (uint32_t i = 0; i < 7; i++)
    {

      for (uint32_t j = 0; j < 7; j++)
        {
          if (i == ( (7 - 1) - j))
            {
              continue;
            }
          V4PingHelper ping (interfaces.GetAddress ( (7 - 1) - j));
          ping.SetAttribute ("Verbose",
                             BooleanValue (false));

          ApplicationContainer p = ping.Install (nodes.Get (i));
          p.Start (Seconds (0 + (i * 10)));
          p.Stop (Seconds (75) - Seconds (0.001));
        }

    }


  Simulator::Stop (Seconds (80.)); // Code runs for 80 seconds.
  Simulator::Run ();
  Simulator::Destroy ();
}

