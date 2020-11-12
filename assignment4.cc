#include <fstream>
#include <string>
#include "ns3/core-module.h"
#include<stdlib.h>
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/gnuplot.h"
#include <algorithm>

using namespace ns3;

std::vector<double>tcpth;
std::vector<double>udpth;
std::vector< double >dtime;
std::vector< double >udprate;

NS_LOG_COMPONENT_DEFINE ("Network_Lab_4");

// Coded an Application so we could take that Socket and use it during simulation

class MyApp : public Application
{
public:

  MyApp ();
  virtual ~MyApp();

  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);
  void ChangeRate(DataRate newrate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>   m_socket;
  Address  m_peer;
  uint32_t m_packetSize;
  uint32_t m_nPackets;
  DataRate m_dataRate;
  EventId  m_sendEvent;
  bool  m_running;
  uint32_t m_packetsSent;
};

// Constructor for Myapp class

MyApp::MyApp ()
  : m_socket (0),
  m_peer (),
  m_packetSize (0),
  m_nPackets (0),
  m_dataRate (0),
  m_sendEvent (),
  m_running (false),
  m_packetsSent (0)
{
}

// Destructor for Myapp class

MyApp::~MyApp()
{
  m_socket = 0;
}

//allow the Socket to be created at configuration time

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

//required to start sending data during the simulation.
void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

//required to stop sending data during the simulation.
void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
  {
 Simulator::Cancel (m_sendEvent);
  }

  if (m_socket)
  {
 m_socket->Close ();
  }
}

// starts data flow
void
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);

  if (++m_packetsSent < m_nPackets)
  {
 ScheduleTx ();
  }
}

//schedule the send packet function tnext
void
MyApp::ScheduleTx (void)
{
  if (m_running)
  {
 Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
 m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
  }
}

//change rate of our app
void
MyApp::ChangeRate(DataRate newrate)
{
 m_dataRate = newrate;
 return;
}

//increase rate at scheduled time
void
IncRate (Ptr<MyApp> app, DataRate rate, FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon, int capture)
{
	app->ChangeRate(rate);
  if(capture){
  std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats();
 Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier());
 double sumtcp = 0,sumudp= 0;
 for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
 {
 Ipv4FlowClassifier::FiveTuple tuple= classing->FindFlow (stats->first);
 double tput =  stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024 ;

 if(tuple.protocol == 6){
   sumtcp += tput;

 }
 if(tuple.protocol == 17){
   sumudp += tput;
 }
 
 }
 //std::cout << sumtcp<< "::"<<sumudp<<"\n";
 tcpth.push_back(sumtcp);
 udpth.push_back(sumudp);
 
  }
  return;
}

//  n0 ---+ +--- n5
// | |
//  n1 ---n3 -- n4--- n6
// | |
//  n2 ---+ +--- n7

int main (int argc, char *argv[])
{
  Time::SetResolution (Time::NS);
  Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId()));

	
//making datasets for plots
  Gnuplot2dDataset dataset1;
  dataset1.SetStyle (Gnuplot2dDataset::LINES_POINTS);
  Gnuplot2dDataset dataset2;
  dataset2.SetStyle (Gnuplot2dDataset::LINES_POINTS);
  Gnuplot2dDataset dataset3;
  dataset3.SetStyle (Gnuplot2dDataset::LINES_POINTS);
  Gnuplot2dDataset dataset4;
  dataset4.SetStyle (Gnuplot2dDataset::LINES_POINTS);
  Gnuplot2dDataset dataset5;
  dataset5.SetStyle (Gnuplot2dDataset::LINES_POINTS);

//varying buffersize from 10*1500 - 800*1500,decided randomly
for(int i=0;i<=10;i++)
{ 
   srand(time(NULL));
 int bufSize=((rand()%800)+10) * 1500;
  CommandLine cmd;
  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (8);



  //installs internet stacks on our two nodes 
  InternetStackHelper internet;
  internet.Install (nodes);
  PointToPointHelper p2p;
  //settings attributes for p2p connections between nodes and routers
  p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("10ms"));
  NetDeviceContainer d0d3 = p2p.Install (nodes.Get (0), nodes.Get (3));
  NetDeviceContainer d1d3 = p2p.Install (nodes.Get (1), nodes.Get (3));
  NetDeviceContainer d2d3 = p2p.Install (nodes.Get (2), nodes.Get (3));
  NetDeviceContainer d4d5 = p2p.Install (nodes.Get (4), nodes.Get (5));
  NetDeviceContainer d4d6 = p2p.Install (nodes.Get (4), nodes.Get (6));
  NetDeviceContainer d4d7 = p2p.Install (nodes.Get (4), nodes.Get (7));

  //settings attributes for p2p connections between routers r1-r2
  //p2p.SetQueue("ns3::DropTailQueue","Mode",EnumValue (DropTailQueue::QUEUE_MODE_BYTES),"MaxBytes",UintegerValue (125000));
  p2p.SetQueue("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("100ms"));
  NetDeviceContainer d3d4 = p2p.Install (nodes.Get (3), nodes.Get (4));

  //creates interfaces and assigns IP addresses for the point-to-point devices.

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
   Ipv4InterfaceContainer host[3];
 host[0] = ipv4.Assign (d0d3);

  ipv4.SetBase ("10.1.2.0", "255.255.255.0");
 host[1] = ipv4.Assign (d1d3);

  ipv4.SetBase ("10.1.3.0", "255.255.255.0");
 host[2] = ipv4.Assign (d2d3);

  ipv4.SetBase ("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer i3i4 = ipv4.Assign (d3d4);

  ipv4.SetBase ("10.1.5.0", "255.255.255.0");
  Ipv4InterfaceContainer i4i5 = ipv4.Assign (d4d5);

  ipv4.SetBase ("10.1.6.0", "255.255.255.0");
  Ipv4InterfaceContainer i4i6 = ipv4.Assign (d4d6);

  ipv4.SetBase ("10.1.7.0", "255.255.255.0");
  Ipv4InterfaceContainer i4i7 = ipv4.Assign (d4d7);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  //tcp - h1 to h6
  //create sockets using the class ns3::TcpSocketFactory

  uint16_t sinkPort1 = 8081;
  //destination node to receive TCP connections and data
  Address sinkAddress1 (InetSocketAddress (i4i7.GetAddress (1), sinkPort1));
  PacketSinkHelper packetSinkHelper1 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort1));
  ApplicationContainer sinkApps1 = packetSinkHelper1.Install (nodes.Get (7));
  sinkApps1.Start (Seconds (0.));
  sinkApps1.Stop (Seconds (15.));

  //creating socket at host
  Ptr<Socket> ns3TcpSocket1 = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());
  	ns3TcpSocket1->SetAttribute("SndBufSize",  ns3::UintegerValue(bufSize));
  	ns3TcpSocket1->SetAttribute("RcvBufSize",  ns3::UintegerValue(bufSize));
  	
  //creating a Myapp object app1
  Ptr<MyApp> app1 = CreateObject<MyApp> ();
  app1->Setup (ns3TcpSocket1, sinkAddress1, 1500, 1000000, DataRate ("20Mbps"));
  nodes.Get (0)->AddApplication (app1);
  //setting start and stop time of app
  app1->SetStartTime (Seconds (1.));
  app1->SetStopTime (Seconds (15.));


  //tcp - h4 to h3
  uint16_t sinkPort2 = 8082;
  Address sinkAddress2 (InetSocketAddress (host[2].GetAddress (0), sinkPort2));
  PacketSinkHelper packetSinkHelper2 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort2));
  ApplicationContainer sinkApps2 = packetSinkHelper2.Install (nodes.Get (2));
  sinkApps2.Start (Seconds (0.));
  sinkApps2.Stop (Seconds (15.));

  Ptr<Socket> ns3TcpSocket2 = Socket::CreateSocket (nodes.Get (5), TcpSocketFactory::GetTypeId ());
  	ns3TcpSocket2->SetAttribute("SndBufSize",  ns3::UintegerValue(bufSize));
  	ns3TcpSocket2->SetAttribute("RcvBufSize",  ns3::UintegerValue(bufSize));

  Ptr<MyApp> app2 = CreateObject<MyApp> ();
  app2->Setup (ns3TcpSocket2, sinkAddress2, 1500, 1000000, DataRate ("20Mbps"));
  nodes.Get (5)->AddApplication (app2);
  app2->SetStartTime (Seconds (1.));
  app2->SetStopTime (Seconds (15.));

  //tcp - h1 to h2
  uint16_t sinkPort3 = 8083;
  Address sinkAddress3 (InetSocketAddress (host[1].GetAddress (0), sinkPort3));
  PacketSinkHelper packetSinkHelper3("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort3));
  ApplicationContainer sinkApps3 = packetSinkHelper3.Install (nodes.Get (1));
  sinkApps3.Start (Seconds (0.));
  sinkApps3.Stop (Seconds (15.));

  Ptr<Socket> ns3TcpSocket3 = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());
  	ns3TcpSocket3->SetAttribute("SndBufSize",  ns3::UintegerValue(bufSize));
  	ns3TcpSocket3->SetAttribute("RcvBufSize",  ns3::UintegerValue(bufSize));

  Ptr<MyApp> app3 = CreateObject<MyApp> ();
  app3->Setup (ns3TcpSocket3, sinkAddress3, 1500, 1000000, DataRate ("20Mbps"));
  nodes.Get (0)->AddApplication (app3);
  app3->SetStartTime (Seconds (1.));
  app3->SetStopTime (Seconds (15.));

  //tcp - h5 to h6
  uint16_t sinkPort4 = 8084;
  Address sinkAddress4 (InetSocketAddress (i4i7.GetAddress (1), sinkPort4));
  PacketSinkHelper packetSinkHelper4("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort4));
  ApplicationContainer sinkApps4 = packetSinkHelper4.Install (nodes.Get (7));
  sinkApps4.Start (Seconds (0.));
  sinkApps4.Stop (Seconds (15.));

  Ptr<Socket> ns3TcpSocket4 = Socket::CreateSocket (nodes.Get (6), TcpSocketFactory::GetTypeId ());
  	ns3TcpSocket4->SetAttribute("SndBufSize",  ns3::UintegerValue(bufSize));
  	ns3TcpSocket4->SetAttribute("RcvBufSize",  ns3::UintegerValue(bufSize));

  Ptr<MyApp> app4 = CreateObject<MyApp> ();
  app4->Setup (ns3TcpSocket4, sinkAddress4, 1500, 1000000, DataRate ("20Mbps"));
  nodes.Get (6)->AddApplication (app4);
  app4->SetStartTime (Seconds (1.));
  app4->SetStopTime (Seconds (15.));

  //udp - h2 to h3
  uint16_t sinkPort5 = 8085;
  Address sinkAddress5 (InetSocketAddress (i3i4.GetAddress (0), sinkPort5));
  PacketSinkHelper packetSinkHelper5 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort5));
  ApplicationContainer sinkApps5 = packetSinkHelper5.Install (nodes.Get (2));
  sinkApps5.Start (Seconds (0.));
  sinkApps5.Stop (Seconds (15.));

  Ptr<Socket> ns3UdpSocket5 = Socket::CreateSocket (nodes.Get (1), UdpSocketFactory::GetTypeId ());
  	ns3UdpSocket5->SetAttribute("RcvBufSize",  ns3::UintegerValue(bufSize));

  Ptr<MyApp> app5 = CreateObject<MyApp> ();
  app5->Setup (ns3UdpSocket5, sinkAddress5, 1500, 1000000, DataRate ("20Mbps"));
  nodes.Get (1)->AddApplication (app5);
  app5->SetStartTime (Seconds (1.));
  app5->SetStopTime (Seconds (15.));

  

  //udp - h4 to h5
  uint16_t sinkPort6 = 8086;
  Address sinkAddress6 (InetSocketAddress (i4i6.GetAddress (1), sinkPort6));
  PacketSinkHelper packetSinkHelper6 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort6));
  ApplicationContainer sinkApps6 = packetSinkHelper6.Install (nodes.Get (6));
  sinkApps6.Start (Seconds (0.));
  sinkApps6.Stop (Seconds (15.));

  Ptr<Socket> ns3UdpSocket6 = Socket::CreateSocket (nodes.Get (5), UdpSocketFactory::GetTypeId ());
  	ns3UdpSocket6->SetAttribute("RcvBufSize",  ns3::UintegerValue(bufSize));

  Ptr<MyApp> app6 = CreateObject<MyApp> ();
  app6->Setup (ns3UdpSocket6, sinkAddress6, 1500, 1000000, DataRate ("20Mbps"));
  nodes.Get (5)->AddApplication (app6);
  app6->SetStartTime (Seconds (1.));
  app6->SetStopTime (Seconds (15.));


  //Helper to enable IP flow monitoring on a set of Nodes.
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();
  dtime.resize(0);
  udprate.resize(0);
  for(int j=1;j<10;)
  {
    int x= (rand()%90+10)*1024*8;
    Simulator::Schedule (Seconds(j), &IncRate, app5, DataRate(x), &flowmon, monitor,0);
    Simulator::Schedule (Seconds(j), &IncRate, app5, DataRate(x), &flowmon, monitor,1);
    j+=(rand()%(10-j)+1);
    dtime.push_back(j);
    udprate.push_back(x);
  }
  Simulator::Schedule (Seconds(10.0), &IncRate, app5, DataRate("100Mbps"), &flowmon, monitor,1);
  Simulator::Schedule (Seconds(10.0), &IncRate, app5, DataRate("100Mbps"), &flowmon, monitor,0);
  
    dtime.push_back(10);
    udprate.push_back(100);
 

  Simulator::Stop (Seconds(15.0));
  //executing the simulation
  Simulator::Run ();
  
  	monitor->CheckForLostPackets ();

  //Classifies packets by looking at their IP and TCP/UDP headers.
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());

  //object that monitors and reports back packet flows observed during a simulation
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

  double Sumx = 0, SumSqx = 0, udpthroughput=0,tcpthroughput=0;

  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
  {
  	  Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
 // Calculating Throughput for different flows
 double TPut = i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())/1024/1024;
 Sumx += TPut;
  	  SumSqx += TPut * TPut ;
 // If the connection is using TCP protocol.
 if(t.protocol == 6){
   tcpthroughput += TPut;
 }
  }
  // Calculating UDP throughput
  udpthroughput = Sumx - tcpthroughput;
  	double FairnessIndex = (Sumx * Sumx)/ (6 * SumSqx) ;
  	dataset1.Add (bufSize/1500, FairnessIndex);
  dataset2.Add(bufSize/1500, udpthroughput);
  dataset3.Add(bufSize/1500, tcpthroughput);
  	
  std :: cout << " FairnessIndex:	" << FairnessIndex << std :: endl;
  monitor->SerializeToXmlFile("lab-1.flowmon", true, true);
  Simulator::Destroy ();

  }

/***************************************/

  // Creating plot
  

  Gnuplot plot1 ("buffvsfairness.png");
  plot1.SetTitle ("buffvsfairness");
  plot1.SetTerminal ("png");
  plot1.SetLegend ("BufferSize", "FairnessIndex");
  plot1.AppendExtra ("set xrange [0:800]");
  plot1.AppendExtra ("plot \"-\"  title \"Untitled\" smooth unique with linespoints");

  // Adding dataset to the plot

	plot1.AddDataset (dataset1);
	std :: ofstream plotFile1 ("BufferSize-vs-FairnessIndex.plt");
	plot1.GenerateOutput (plotFile1);
	plotFile1.close ();

/***************************************/
  
  Gnuplot plot2 ("buffvsudp.png");
  plot2.SetTitle ("buffvsudpthroughput");
  plot2.SetTerminal ("png");
  plot2.SetLegend ("BufferSize", "udpthroughput");
  plot2.AppendExtra ("set xrange [0:800]");
  plot2.AppendExtra ("plot \"-\"  title \"Untitled\" smooth unique with linespoints");

  plot2.AddDataset (dataset2);
  std :: ofstream plotFile2 ("BufferSize-vs-UDPthroughput.plt");
  plot2.GenerateOutput (plotFile2);
  plotFile2.close ();

/***************************************/

  Gnuplot plot3 ("buffvstcp.png");
  plot3.SetTitle ("buffvstcpthroughput");
  plot3.SetTerminal ("png");
  plot3.SetLegend ("BufferSize", "tcpthroughput");
  plot3.AppendExtra ("set xrange [0:800]");
  plot3.AppendExtra ("plot \"-\"  title \"Untitled\" smooth unique with linespoints");
  
  plot3.AddDataset (dataset3);
  std :: ofstream plotFile3 ("BufferSize-vs-TCPthroughput.plt");
  plot3.GenerateOutput (plotFile3);
  plotFile3.close ();

/***************************************/

  dtime.resize(9);
  dtime[0] = 2; dtime[1] = 3; dtime[2] = 4; dtime[3] =5; dtime[4] = 6; dtime[5] = 7; dtime[6] = 8; dtime[7] = 10; dtime[8] = 15;
 // dtime[9] = 16; dtime[10] = 17; dtime[11] = 18; dtime[12] = 20,dtime[13]=25;
  for(int i =0 ;i < (int) dtime.size(); i++){
  dataset4.Add(udprate[i],tcpth[i]);
  dataset5.Add(udprate[i],udpth[i]);
  }

  Gnuplot plot4 ("UDP vs TCP throughput.png");
  plot4.SetTitle ("Effect of UDP flow on tcpthroughput");
  plot4.SetTerminal ("png");
  plot4.SetLegend ("udp rate", "TCP throughput");

  plot4.AddDataset (dataset4);
  std :: ofstream plotFile4 ("UDPrate-vs-TCPthroughput.plt");
  plot4.GenerateOutput (plotFile4);
  plotFile4.close ();

/***************************************/

  Gnuplot plot5 ("udpvsudpthroughput.png");
  plot5.SetTitle ("Effect of UDP flow on udpthroughput");
  plot5.SetTerminal ("png");
  plot5.SetLegend ("udp rate", "UDPthroughput");

  plot5.AddDataset (dataset5);
  std :: ofstream plotFile5 ("udprate-vs-UDPthroughput.plt");
  plot5.GenerateOutput (plotFile5);
  plotFile5.close ();

}

