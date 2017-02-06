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
 *

./waf --run "vanetCloud --traceFile=$HOME/workspace/sumoexample/data/nsmobility.tcl --nodeNum=324 --duration=500 --logFile=$HOME/workspace/main-ns2-mob.log"

evaluation criteria: how many seconds into the simulation does request start (saturation), number of applications in network (participation), distance from infrastructure going away (reach)

 * Author: Josh Pelkey <jpelkey@gatech.edu>
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "ns3/mobility-module.h"
#include "ns3/constant-velocity-mobility-model.h"
#include "ns3/ns2-mobility-helper.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
#include "ns3/applications-module.h"


using namespace ns3;

int main (int argc, char *argv[])
{
  Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (512));
  Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue ("500kb/s"));
  Packet::EnablePrinting ();
  uint32_t packetSize = 1000; // bytes
  uint32_t numPackets = 500;
  double interval = 1.0; // seconds
  // Convert to time object
  Time interPacketInterval = Seconds (interval);

  std::string animFile = "animation.xml";
  std::string traceFile;
  std::string logFile;
  int    nodeNum;
  double duration;
// Parse command line attribute
  CommandLine cmd;
  cmd.AddValue ("traceFile", "Ns2 movement trace file", traceFile);
  cmd.AddValue ("nodeNum", "Number of nodes", nodeNum);
  cmd.AddValue ("duration", "Duration of Simulation", duration);
  cmd.AddValue ("logFile", "Log file", logFile);
  cmd.Parse (argc,argv);

  // Check command line arguments
  if (traceFile.empty () || nodeNum <= 0 || duration <= 0 || logFile.empty ())
    {
      std::cout << "Usage of " << argv[0] << " :\n\n"
      "./waf --run \"vanetCloud"
      " --traceFile=src/mobility/examples/default.ns_movements"
      " --nodeNum=2 --duration=100.0 --logFile=ns2-mob.log\" \n\n"
      "NOTE: ns2-traces-file could be an absolute or relative path. You could use the file default.ns_movements\n"
      "      included in the same directory of this example file.\n\n"
      "NOTE 2: Number of nodes present in the trace file must match with the command line argument and must\n"
      "        be a positive number. Note that you must know it before to be able to load it.\n\n"
      "NOTE 3: Duration must be a positive number. Note that you must know it before to be able to load it.\n\n";

      return 0;
    }
  // Create Ns2MobilityHelper with the specified trace log file as parameter
  Ns2MobilityHelper ns2 = Ns2MobilityHelper (traceFile);

// Create all vehicle nodes.
  NodeContainer nodes;
  nodes.Create (nodeNum);

  ns2.Install (); // configure movements for each node, while reading trace file

  // Configure Network Channels

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  channel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  channel.AddPropagationLoss ("ns3::RangePropagationLossModel","MaxRange",DoubleValue (250));
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  // Add a mac and disable rate control
  std::string phyMode ("DsssRate1Mbps");
  WifiMacHelper wifiMac;
  wifi.SetStandard(WIFI_PHY_STANDARD_80211b);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
  // Set it to adhoc mode
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install (phy, wifiMac, nodes);

  InternetStackHelper internet;
  internet.Install (nodes);

//
// We've got the "hardware" in place.  Now we need to add IP addresses.
//
  //NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.0.0", "255.255.0.0");
  Ipv4InterfaceContainer interfaces = ipv4.Assign (devices);
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

  //Address serverAddress = Address(interfaces.GetAddress (0));
  Address serverAddress = Address(Ipv4Address ("255.255.255.255") );
  //NS_LOG_INFO ("Create Applications.");
//
// Create a VehicleNetServer application on node one.
//
  uint16_t port = 9;  // well-known echo port number
  //VehicleNetServerHelper server (port);
  //ApplicationContainer apps = server.Install (nodes.Get (0));
  //apps.Start (Seconds (150.0));
  //apps.Stop (Seconds (350.0));

//
// Create a VehicleNetClient application to send UDP datagrams from node zero to
// node one.
//
  //Time interPacketInterval = Seconds (1.);
  VehicleNetClientHelper client (serverAddress, port);
  client.SetAttribute ("MaxPackets", UintegerValue (numPackets));
  client.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client.SetAttribute ("PacketSize", UintegerValue (packetSize));
  ApplicationContainer apps = client.Install (nodes.Get (2));
  double w = 2/100 + 250;
  apps.Start (Seconds (w));
  apps.Stop (Seconds (w+250.0));
  for (int q = 3; q < 320; q++)
  {
  VehicleNetClientHelper client (serverAddress, port);
  client.SetAttribute ("MaxPackets", UintegerValue (numPackets));
  client.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client.SetAttribute ("PacketSize", UintegerValue (packetSize));
  ApplicationContainer apps = client.Install (nodes.Get (q));
  double w = q/100 + 255;
  apps.Start (Seconds (w));
  apps.Stop (Seconds (w+250.0));
  }
/*
  VehicleNetClientHelper client2 (serverAddress, port);
  client2.SetAttribute ("MaxPackets", UintegerValue (numPackets));
  client2.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client2.SetAttribute ("PacketSize", UintegerValue (packetSize));
  apps = client.Install (nodes.Get (14));
  apps.Start (Seconds (251.5));
  apps.Stop (Seconds (351.5));

  VehicleNetClientHelper client3 (serverAddress, port);
  client3.SetAttribute ("MaxPackets", UintegerValue (numPackets));
  client3.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client3.SetAttribute ("PacketSize", UintegerValue (packetSize));
  apps = client.Install (nodes.Get (27));
  apps.Start (Seconds (252.7));
  apps.Stop (Seconds (352.7));


  VehicleNetClientHelper client4 (serverAddress, port);
  client4.SetAttribute ("MaxPackets", UintegerValue (numPackets));
  client4.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client4.SetAttribute ("PacketSize", UintegerValue (packetSize));
  apps = client.Install (nodes.Get (30));
  apps.Start (Seconds (253.25));
  apps.Stop (Seconds (353.25));
*/


  // Create the animation object and configure for specified output
  AnimationInterface anim (animFile);

  // Set up the actual simulation
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}








