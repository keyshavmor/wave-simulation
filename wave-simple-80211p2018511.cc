/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006,2007 INRIA
 * Copyright (c) 2013 Dalian University of Technology
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Author: Junling Bu <linlinjavaer@gmail.com>
 *
 */
/**
 * This example shows basic construction of an 802.11p node.  Two nodes
 * are constructed with 802.11p devices, and by default, one node sends a single
 * packet to another node (the number of packets and interval between
 * them can be configured by command-line arguments).  The example shows
 * typical usage of the helper classes for this mode of WiFi (where "OCB" refers
 * to "Outside the Context of a BSS")."
 */

#include "ns3/vector.h"
#include "ns3/string.h"
#include "ns3/socket.h"
#include "ns3/double.h"
#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/command-line.h"
#include "ns3/mobility-model.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/position-allocator.h"
#include "ns3/mobility-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/netanim-module.h"
#include <iostream>
#include "ns3/constant-velocity-mobility-model.h"
#include "ns3/ocb-wifi-mac.h"
#include "ns3/wifi-80211p-helper.h"
#include "ns3/wave-mac-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/seq-ts-header.h"
#include <math.h>
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WifiSimpleOcb");

/*
 * In WAVE module, there is no net device class named like "Wifi80211pNetDevice",
 * instead, we need to use Wifi80211pHelper to create an object of
 * WifiNetDevice class.
 *
 * usage:
 *  NodeContainer nodes;
 *  NetDeviceContainer devices;
 *  nodes.Create (2);
 *  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
 *  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
 *  wifiPhy.SetChannel (wifiChannel.Create ());
 *  NqosWaveMacHelper wifi80211pMac = NqosWave80211pMacHelper::Default();
 *  Wifi80211pHelper wifi80211p = Wifi80211pHelper::Default ();
 *  devices = wifi80211p.Install (wifiPhy, wifi80211pMac, nodes);
 *
 * The reason of not providing a 802.11p class is that most of modeling
 * 802.11p standard has been done in wifi module, so we only need a high
 * MAC class that enables OCB mode.
 */
typedef struct receiveFlag{

uint8_t flag=0;
uint8_t queryToken=0;
uint8_t confusion=0;
uint8_t agreement=0;
uint8_t move=0;
uint8_t arrival=0;
uint8_t confirmation1=0;
uint8_t confirmation2=0;
uint8_t confirmation3=0;
}receiveFlag;

receiveFlag receiveIndicator;

SeqTsHeader seqTs; // class for important time-stamps

 void ReceivePacket (Ptr<Socket> socket)  // receivePacket from broken car
    {

      while (socket->Recv ())
        {
  std::cout << "  recvTime = " << Simulator::Now ().GetSeconds () << "s," << std::endl;
if(receiveIndicator.flag == 1)
{
             NS_LOG_UNCOND ("I need a spare part , my car's broken!"); 
receiveIndicator.flag = 0 ;

 }

         if(receiveIndicator.confusion == 1)
{
        NS_LOG_UNCOND ("I got 5 affirmatives. Only the nearest by GPS proximity send");
          receiveIndicator.queryToken = 0 ;

receiveIndicator.confusion = 0;
 } 

if(receiveIndicator.agreement == 1)
{
         NS_LOG_UNCOND ("Ok the nearest vehicle will come. Rest vehicles would pass by.");
         
receiveIndicator.agreement = 0 ;

 }

      
          if(receiveIndicator.confirmation1 && receiveIndicator.confirmation2 && receiveIndicator.confirmation3  == 1)
{
        NS_LOG_UNCOND ("I am a Black Volkswagen on the left-side");
          receiveIndicator.confirmation1 =0;
    receiveIndicator.confirmation2 =0;
 receiveIndicator.confirmation3 = 0 ;


 }
          if(receiveIndicator.move == 1)
{
            NS_LOG_UNCOND ("Ok I will be there.");
          receiveIndicator.move = 0 ;
receiveIndicator.arrival = 1;
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
    std::cout << "  sendTime = " << Simulator::Now ().GetSeconds () << "s," << std::endl;
    }
  else
    {
      socket->Close ();
    }
}

int main (int argc, char *argv[])
{
 std::string phyMode ("OfdmRate6MbpsBW10MHz");
     uint32_t packetSize = 1000; // bytes
     uint32_t numPackets = 1;
     double interval = 1.0; // seconds
     bool verbose = false;
   
     CommandLine cmd;
   
     cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
      cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
     cmd.AddValue ("numPackets", "number of packets generated", numPackets);
     cmd.AddValue ("interval", "interval (seconds) between packets", interval);
     cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
     cmd.Parse (argc, argv);
     // Convert to time object
     Time interPacketInterval = Seconds (interval);


  NodeContainer c;
     c.Create (30);
   
     // The below set of helpers will help us to put together the wifi NICs we want
     YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
     YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
     Ptr<YansWifiChannel> channel = wifiChannel.Create ();
     wifiPhy.SetChannel (channel);
     // ns-3 supports generate a pcap trace
     wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11);
     NqosWaveMacHelper wifi80211pMac = NqosWaveMacHelper::Default ();
     Wifi80211pHelper wifi80211p = Wifi80211pHelper::Default ();
     if (verbose)
       {
         wifi80211p.EnableLogComponents ();      // Turn on all Wifi 802.11p logging
       }
   
     wifi80211p.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                         "DataMode",StringValue (phyMode),
                                         "ControlMode",StringValue (phyMode));
     NetDeviceContainer devices = wifi80211p.Install (wifiPhy, wifi80211pMac, c);
   
     // Tracing
     wifiPhy.EnablePcap ("wave-simple-80211p", devices);
   
     MobilityHelper mobility;
     Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
     positionAlloc->Add (Vector (7.5, 0.0, 0.0));
     

for(double i=1.0;i<=30.0;i++)
{

double j=0.0;
double a=1.03; // latus rectum of parabolic high way
j=(4.0*a*(i));
j=pow(j,0.5);
     positionAlloc->Add (Vector (i , j, 0.0)); // form a parabola of cars to demonstrate a highway curve
}
     mobility.SetPositionAllocator (positionAlloc);
     mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel"); // because transmission is too fast for any noticable vehicular displacement
     mobility.Install (c);
  


     InternetStackHelper internet;
     internet.Install (c);
   
     Ipv4AddressHelper ipv4;
     NS_LOG_INFO ("Assign IP Addresses.");
     ipv4.SetBase ("10.1.1.0", "255.255.255.0");
     Ipv4InterfaceContainer i = ipv4.Assign (devices);

     TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
     Ptr<Socket> recvSink = Socket::CreateSocket (c.Get (0), tid);
     InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80); // receive sink of broken car hence the receive packet from moving cars
     recvSink->Bind (local);
     recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));

     TypeId tid1 = TypeId::LookupByName ("ns3::UdpSocketFactory");
     Ptr<Socket> recvSink1 = Socket::CreateSocket (c.Get (1), tid1);
     InetSocketAddress local1 = InetSocketAddress (Ipv4Address::GetAny (), 80); //  receive sink of moving car hence the receive packet from broken or other cars
     recvSink1->Bind (local1);
     recvSink1->SetRecvCallback (MakeCallback (&ReceivePacket));


  TypeId tid2 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink2 = Socket::CreateSocket (c.Get (2), tid2);
  InetSocketAddress local2 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink2->Bind (local2);
  recvSink2->SetRecvCallback (MakeCallback (&ReceivePacket));

  TypeId tid3 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink3 = Socket::CreateSocket (c.Get (3), tid3);
  InetSocketAddress local3 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink3->Bind (local3);
  recvSink3->SetRecvCallback (MakeCallback (&ReceivePacket));

  TypeId tid4 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink4 = Socket::CreateSocket (c.Get (4), tid4);
  InetSocketAddress local4 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink4->Bind (local4);
  recvSink4->SetRecvCallback (MakeCallback (&ReceivePacket));

  TypeId tid5 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink5 = Socket::CreateSocket (c.Get (5), tid5);
  InetSocketAddress local5 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink5->Bind (local5);
  recvSink5->SetRecvCallback (MakeCallback (&ReceivePacket));

  TypeId tid6 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink6 = Socket::CreateSocket (c.Get (6), tid6);
  InetSocketAddress local6 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink6->Bind (local6);
  recvSink6->SetRecvCallback (MakeCallback (&ReceivePacket));

  TypeId tid7 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink7 = Socket::CreateSocket (c.Get (7), tid7);
  InetSocketAddress local7 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink7->Bind (local7);
  recvSink7->SetRecvCallback (MakeCallback (&ReceivePacket));

  TypeId tid8 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink8 = Socket::CreateSocket (c.Get (8), tid8);
  InetSocketAddress local8 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink8->Bind (local8);
  recvSink8->SetRecvCallback (MakeCallback (&ReceivePacket));

  TypeId tid9 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink9 = Socket::CreateSocket (c.Get (9), tid9);
  InetSocketAddress local9 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink9->Bind (local9);
  recvSink9->SetRecvCallback (MakeCallback (&ReceivePacket));

  TypeId tid10 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink10 = Socket::CreateSocket (c.Get (10), tid10);
  InetSocketAddress local10 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink10->Bind (local10);
  recvSink10->SetRecvCallback (MakeCallback (&ReceivePacket));

  TypeId tid11 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink11 = Socket::CreateSocket (c.Get (11), tid11);
  InetSocketAddress local11 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink11->Bind (local11);
  recvSink11->SetRecvCallback (MakeCallback (&ReceivePacket));

  TypeId tid12 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink12 = Socket::CreateSocket (c.Get (12), tid12);
  InetSocketAddress local12 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink12->Bind (local12);
  recvSink12->SetRecvCallback (MakeCallback (&ReceivePacket));

  TypeId tid13 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink13 = Socket::CreateSocket (c.Get (13), tid13);
  InetSocketAddress local13 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink13->Bind (local13);
  recvSink13->SetRecvCallback (MakeCallback (&ReceivePacket));

  TypeId tid14 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink14 = Socket::CreateSocket (c.Get (14), tid14);
  InetSocketAddress local14 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink14->Bind (local14);
  recvSink14->SetRecvCallback (MakeCallback (&ReceivePacket));

  TypeId tid15 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink15 = Socket::CreateSocket (c.Get (15), tid15);
  InetSocketAddress local15 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink15->Bind (local15);
  recvSink15->SetRecvCallback (MakeCallback (&ReceivePacket));

  TypeId tid16 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink16 = Socket::CreateSocket (c.Get (16), tid16);
  InetSocketAddress local16 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink16->Bind (local16);
  recvSink16->SetRecvCallback (MakeCallback (&ReceivePacket));

  TypeId tid17 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink17 = Socket::CreateSocket (c.Get (17), tid17);
  InetSocketAddress local17 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink17->Bind (local17);
  recvSink17->SetRecvCallback (MakeCallback (&ReceivePacket));

  TypeId tid18 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink18 = Socket::CreateSocket (c.Get (18), tid18);
  InetSocketAddress local18 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink18->Bind (local18);
  recvSink18->SetRecvCallback (MakeCallback (&ReceivePacket));

  TypeId tid19 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink19 = Socket::CreateSocket (c.Get (19), tid19);
  InetSocketAddress local19 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink19->Bind (local19);
  recvSink19->SetRecvCallback (MakeCallback (&ReceivePacket));

  TypeId tid20 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink20 = Socket::CreateSocket (c.Get (20), tid20);
  InetSocketAddress local20 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink20->Bind (local20);
  recvSink20->SetRecvCallback (MakeCallback (&ReceivePacket));

  TypeId tid21 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink21 = Socket::CreateSocket (c.Get (21), tid21);
  InetSocketAddress local21 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink21->Bind (local21);
  recvSink21->SetRecvCallback (MakeCallback (&ReceivePacket));

  TypeId tid22 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink22 = Socket::CreateSocket (c.Get (22), tid22);
  InetSocketAddress local22 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink22->Bind (local22);
  recvSink22->SetRecvCallback (MakeCallback (&ReceivePacket));

  TypeId tid23 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink23 = Socket::CreateSocket (c.Get (23), tid23);
  InetSocketAddress local23 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink23->Bind (local23);
  recvSink23->SetRecvCallback (MakeCallback (&ReceivePacket));

  TypeId tid24 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink24 = Socket::CreateSocket (c.Get (24), tid24);
  InetSocketAddress local24 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink24->Bind (local24);
  recvSink24->SetRecvCallback (MakeCallback (&ReceivePacket));

  TypeId tid25 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink25 = Socket::CreateSocket (c.Get (25), tid25);
  InetSocketAddress local25 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink25->Bind (local25);
  recvSink25->SetRecvCallback (MakeCallback (&ReceivePacket));

  TypeId tid26 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink26 = Socket::CreateSocket (c.Get (26), tid26);
  InetSocketAddress local26 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink26->Bind (local26);
  recvSink26->SetRecvCallback (MakeCallback (&ReceivePacket));

  TypeId tid27 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink27 = Socket::CreateSocket (c.Get (27), tid27);
  InetSocketAddress local27 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink27->Bind (local27);
  recvSink27->SetRecvCallback (MakeCallback (&ReceivePacket));

  TypeId tid28 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink28 = Socket::CreateSocket (c.Get (28), tid28);
  InetSocketAddress local28 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink28->Bind (local28);
  recvSink28->SetRecvCallback (MakeCallback (&ReceivePacket));

  TypeId tid29 = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink29 = Socket::CreateSocket (c.Get (29), tid29);
  InetSocketAddress local29 = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink29->Bind (local29);
  recvSink29->SetRecvCallback (MakeCallback (&ReceivePacket));


 
     Ptr<Socket> source = Socket::CreateSocket (c.Get (0), tid);
     InetSocketAddress remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), 80); // broken car socket
     source->SetAllowBroadcast (true);
     source->Connect (remote);
   
     Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
                                  Seconds (0.0), &GenerateTraffic,
                                  source, packetSize, numPackets, interPacketInterval); //distress signal





receiveIndicator.flag =1;

      source = Socket::CreateSocket (c.Get (8), tid8);
      remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), 80); // moving car socket
     source->SetAllowBroadcast (true);
     source->Connect (remote);

 Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
                                  Seconds (0.02), &GenerateTraffic,
                                  source, packetSize, numPackets, interPacketInterval); // multiple cars signal spare availability braodcast for everyone else

    source = Socket::CreateSocket (c.Get (17), tid17);
      remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), 80); // moving car socket
     source->SetAllowBroadcast (true);
     source->Connect (remote);

 Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
                                  Seconds (0.04), &GenerateTraffic,
                                  source, packetSize, numPackets, interPacketInterval); // multiple cars signal spare availability braodcast for everyone else

    source = Socket::CreateSocket (c.Get (25), tid25);
      remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), 80); // moving car socket
     source->SetAllowBroadcast (true);
     source->Connect (remote);

 Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
                                  Seconds (0.06), &GenerateTraffic,
                                  source, packetSize, numPackets, interPacketInterval); // multiple cars signal spare availability braodcast for everyone else




  

receiveIndicator.confusion=1;
source = Socket::CreateSocket (c.Get (0), tid);
      remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), 80); // broken car socket
     source->SetAllowBroadcast (true);
     source->Connect (remote);

  Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
                                  Seconds (0.08), &GenerateTraffic,
                                  source, packetSize, numPackets, interPacketInterval); //confusion indicated.


receiveIndicator.agreement =1;


      source = Socket::CreateSocket (c.Get (8), tid8);
      remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), 80); // moving car socket
     source->SetAllowBroadcast (true);
     source->Connect (remote);

 Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
                                  Seconds (0.1), &GenerateTraffic,
                                  source, packetSize, numPackets, interPacketInterval); // All cars agree that nearest one would send.

receiveIndicator.confirmation1 =1;

source = Socket::CreateSocket (c.Get (17), tid17);
      remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), 80); // moving car socket
     source->SetAllowBroadcast (true);
     source->Connect (remote);

 Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
                                  Seconds (0.12), &GenerateTraffic,
                                  source, packetSize, numPackets, interPacketInterval); // All cars agree that nearest one would send.
receiveIndicator.confirmation2 =1;

source = Socket::CreateSocket (c.Get (25), tid25);
      remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), 80); // moving car socket
     source->SetAllowBroadcast (true);
     source->Connect (remote);

 Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
                                  Seconds (0.14), &GenerateTraffic,
                                  source, packetSize, numPackets, interPacketInterval); // All cars agree that nearest one would send.


receiveIndicator.confirmation3 =1;

     source = Socket::CreateSocket (c.Get (0), tid);
     remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), 80); // broken car socket
     source->SetAllowBroadcast (true);
     source->Connect (remote);

  Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
                                  Seconds (0.16), &GenerateTraffic,
                                  source, packetSize, numPackets, interPacketInterval); //broken car sends location


receiveIndicator.move =1;


     source = Socket::CreateSocket (c.Get (25), tid25);
     remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), 80); // broken car socket
     source->SetAllowBroadcast (true);
     source->Connect (remote);

  Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
                                  Seconds (0.18), &GenerateTraffic,
                                  source, packetSize, numPackets, interPacketInterval); //helping car moves to that location.




  AnimationInterface anim ("wave-animation.xml"); 
  
  anim.UpdateNodeDescription (0, "Broken Car");
  anim.SetConstantPosition (c.Get(0),7.5 , 0.0);
   if(receiveIndicator.arrival == 1)
{
anim.SetConstantPosition (c.Get(22),8.5 , 1.0); //move car near broken car
receiveIndicator.arrival = 0;
}
     Simulator::Run ();



     Simulator::Destroy ();
  
     return 0;

}
