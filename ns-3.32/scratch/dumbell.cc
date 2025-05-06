#include <iostream>
#include <cmath>
#include <fstream>
#include <bits/stdc++.h>

#include "ns3/aodv-module.h"
#include "ns3/netanim-module.h"
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
#include "ns3/v4ping-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/csma-helper.h"
#include "ns3/trust-aodv-module.h"
#include "ns3/trust-manager-helper.h"
#include "ns3/trust-aodv-routing-protocol.h"

using namespace ns3;
using namespace aodv;
using namespace std;

NS_LOG_COMPONENT_DEFINE("TapDumbbellExample");

int main(int argc, char *argv[])
{
  //   std::string mode = "ConfigureLocal";
  //   std::string tapName = "thetap";

  CommandLine cmd(__FILE__);
  //   cmd.AddValue ("mode", "Mode setting of TapBridge", mode);
  //   cmd.AddValue ("tapName", "Name of the OS tap device", tapName);
  cmd.Parse(argc, argv);

  GlobalValue::Bind("SimulatorImplementationType", StringValue("ns3::RealtimeSimulatorImpl"));
  GlobalValue::Bind("ChecksumEnabled", BooleanValue(true));

  //
  // Now, create the right side.
  //
  NodeContainer nodesRight;
  nodesRight.Create(2);

  CsmaHelper csmaRight;
  csmaRight.SetChannelAttribute("DataRate", DataRateValue(5000000));
  csmaRight.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));

  NetDeviceContainer devicesRight = csmaRight.Install(nodesRight);

  InternetStackHelper internetRight;
  internetRight.Install(nodesRight);

  Ipv4AddressHelper ipv4Right;
  ipv4Right.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfacesRight = ipv4Right.Assign(devicesRight);

  TapBridgeHelper tapBridge(interfacesRight.GetAddress(0));
  tapBridge.SetAttribute("Mode", StringValue("UseLocal"));
  tapBridge.SetAttribute("DeviceName", StringValue("tap-right"));
  tapBridge.Install(nodesRight.Get(0), devicesRight.Get(0));

  //
  // The topology has a Wifi network of four nodes on the left side.  We'll make
  // node zero the AP and have the other three will be the STAs.
  //
  NodeContainer nodesLeft;
  nodesLeft.Create(7);

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
  wifiPhy.SetChannel(wifiChannel.Create());

  Ssid ssid = Ssid("left");
  WifiHelper wifi;
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager("ns3::ArfWifiManager");

  wifiMac.SetType("ns3::ApWifiMac",
                  "Ssid", SsidValue(ssid));
  NetDeviceContainer devicesLeft = wifi.Install(wifiPhy, wifiMac, nodesLeft.Get(0));

  wifiMac.SetType("ns3::StaWifiMac",
                  "Ssid", SsidValue(ssid),
                  "ActiveProbing", BooleanValue(false));
  devicesLeft.Add(wifi.Install(wifiPhy, wifiMac, NodeContainer(nodesLeft.Get(1), nodesLeft.Get(2), nodesLeft.Get(3))));
  devicesLeft.Add(wifi.Install(wifiPhy, wifiMac, NodeContainer(nodesLeft.Get(4), nodesLeft.Get(5), nodesLeft.Get(6))));

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
  positionAlloc->Add(Vector(0.0, 0.0, 0.0));
  positionAlloc->Add(Vector(5.0, 0.0, 0.0));
  positionAlloc->Add(Vector(35.0, 0.0, 0.0));
  positionAlloc->Add(Vector(65.0, 0.0, 0.0));
  positionAlloc->Add(Vector(65.0, 30.0, 0.0));
  positionAlloc->Add(Vector(35.0, 30.0, 0.0));
  positionAlloc->Add(Vector(5.0, 30.0, 0.0));
  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(nodesLeft);

  // TrustAodvHelper aodv;
  InternetStackHelper internetLeft;
  // internetLeft.SetRoutingHelper (aodv);
  internetLeft.Install(nodesLeft);

  Ipv4AddressHelper ipv4Left;
  ipv4Left.SetBase("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer interfacesLeft = ipv4Left.Assign(devicesLeft);

  //
  // Stick in the point-to-point line between the sides.
  //
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue("512kbps"));
  p2p.SetChannelAttribute("Delay", StringValue("10ms"));

  NodeContainer nodes = NodeContainer(nodesRight.Get(1), nodesLeft.Get(0));
  NetDeviceContainer devices = p2p.Install(nodes);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.1.2.0", "255.255.255.192");
  Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

  //
  // Simulate some CBR traffic over the point-to-point link
  //
  uint16_t port = 9; // Discard port (RFC 863)
  OnOffHelper onoff("ns3::UdpSocketFactory", InetSocketAddress(interfaces.GetAddress(1), port));
  onoff.SetConstantRate(DataRate("500kb/s"));

  ApplicationContainer apps = onoff.Install(nodesRight.Get(1));
  apps.Start(Seconds(1.0));

  // Create a packet sink to receive these packets
  PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));

  apps = sink.Install(nodesLeft.Get(0));
  apps.Start(Seconds(1.0));

  wifiPhy.EnablePcapAll("tap-wifi-dumbbell");

  csmaRight.EnablePcapAll("tap-wifi-dumbbell", false);
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  Ipv4StaticRoutingHelper staticRouting;
  Ptr<Ipv4StaticRouting> staticRoute = staticRouting.GetStaticRouting(nodesRight.Get(1)->GetObject<Ipv4>());
  staticRoute->AddHostRouteTo(Ipv4Address("10.0.1.4"), interfacesRight.GetAddress(0), 1, 0);

  Ipv4StaticRoutingHelper staticRouting1;
  Ptr<Ipv4StaticRouting> staticRoute1 = staticRouting1.GetStaticRouting(nodes.Get(0)->GetObject<Ipv4>());
  staticRoute1->AddHostRouteTo(Ipv4Address("10.0.1.4"), interfacesRight.GetAddress(1), 1, 0);

  Ipv4StaticRoutingHelper staticRouting2;
  Ptr<Ipv4StaticRouting> staticRoute2 = staticRouting2.GetStaticRouting(nodes.Get(1)->GetObject<Ipv4>());
  staticRoute2->AddHostRouteTo(Ipv4Address("10.0.1.4"), interfaces.GetAddress(0), 1, 0);

  Ipv4StaticRoutingHelper staticRouting3;
  Ptr<Ipv4StaticRouting> staticRoute3 = staticRouting3.GetStaticRouting(nodesLeft.Get(0)->GetObject<Ipv4>());
  staticRoute3->AddHostRouteTo(Ipv4Address("10.0.1.4"), interfaces.GetAddress(1), 1, 0);


  for (int oo = 0; oo < 75; oo = oo + 5)
  {
    TrustManagerHelper trustManagerHelper;
    string str1 = "TUTU", str3 = ".routes";
    stringstream ss;
    ss << oo;
    string str2 = ss.str();

    Ptr<OutputStreamWrapper> trustRoutingStream = Create<OutputStreamWrapper>(str1 + str2 + str3, std::ios::out);
    trustManagerHelper.PrintTrustTableAllAt(Seconds(oo), trustRoutingStream);
  }

  for (uint32_t i = 0; i < 7; i++)
  {

    for (uint32_t j = 0; j < 7; j++)
    {
      if (i == ((7 - 1) - j))
      {
        continue;
      }
      V4PingHelper ping(interfacesLeft.GetAddress((7 - 1) - j));
      ping.SetAttribute("Verbose",
                        BooleanValue(false));

      ApplicationContainer p = ping.Install(nodesLeft.Get(i));
      p.Start(Seconds(0 + (i * 10)));
      p.Stop(Seconds(50) - Seconds(0.001));
    }
  }

  Simulator::Stop(Seconds(80.));
  Simulator::Run();
  Simulator::Destroy();
}
