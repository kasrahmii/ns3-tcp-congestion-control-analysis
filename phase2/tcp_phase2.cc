// tcp_phase2.cc - Phase 2: Variable Topology Testing
// Fixed version - UNIDIRECTIONAL flows only

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

#include <cmath>
#include <fstream>
#include <map>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpPhase2");

// Global variables for cwnd tracking
std::map<uint32_t, std::ofstream*> g_cwndFiles;

// Custom application for rate-controlled sending
class RateControlledSender : public Application
{
  public:
    RateControlledSender();
    virtual ~RateControlledSender();

    void Setup(Ptr<Socket> socket,
               Address address,
               uint32_t packetSize,
               double packetRate,
               uint32_t flowId,
               std::string outputDir);

  private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);

    void ScheduleTx(void);
    void SendPacket(void);

    Ptr<Socket> m_socket;
    Address m_peer;
    uint32_t m_packetSize;
    double m_packetRate;
    EventId m_sendEvent;
    bool m_running;
    uint32_t m_packetsSent;
    uint32_t m_flowId;
    std::string m_outputDir;
};

RateControlledSender::RateControlledSender()
    : m_socket(0),
      m_peer(),
      m_packetSize(0),
      m_packetRate(0),
      m_sendEvent(),
      m_running(false),
      m_packetsSent(0),
      m_flowId(0),
      m_outputDir("")
{
}

RateControlledSender::~RateControlledSender()
{
    m_socket = 0;
}

void
RateControlledSender::Setup(Ptr<Socket> socket,
                            Address address,
                            uint32_t packetSize,
                            double packetRate,
                            uint32_t flowId,
                            std::string outputDir)
{
    m_socket = socket;
    m_peer = address;
    m_packetSize = packetSize;
    m_packetRate = packetRate;
    m_flowId = flowId;
    m_outputDir = outputDir;
}

void
RateControlledSender::StartApplication(void)
{
    m_running = true;
    m_packetsSent = 0;
    m_socket->Bind();
    m_socket->Connect(m_peer);
    SendPacket();
}

void
RateControlledSender::StopApplication(void)
{
    m_running = false;

    if (m_sendEvent.IsPending())
    {
        Simulator::Cancel(m_sendEvent);
    }

    if (m_socket)
    {
        m_socket->Close();
    }
}

void
RateControlledSender::SendPacket(void)
{
    Ptr<Packet> packet = Create<Packet>(m_packetSize);
    m_socket->Send(packet);
    m_packetsSent++;

    if (m_running)
    {
        ScheduleTx();
    }
}

void
RateControlledSender::ScheduleTx(void)
{
    if (m_running)
    {
        Time tNext(Seconds(1.0 / m_packetRate));
        m_sendEvent = Simulator::Schedule(tNext, &RateControlledSender::SendPacket, this);
    }
}

// Congestion window tracer
static void
CwndTracer(uint32_t flowId, uint32_t oldCwnd, uint32_t newCwnd)
{
    if (g_cwndFiles.find(flowId) != g_cwndFiles.end() && g_cwndFiles[flowId])
    {
        *g_cwndFiles[flowId] << Simulator::Now().GetSeconds() << "," << newCwnd << std::endl;
    }
}

// Function to save statistics - FIXED to only count data flows
void
SaveStatistics(std::string outputDir,
               std::string detailFilename,
               std::string tcpType,
               uint32_t nNodes,
               uint32_t nFlows,
               double packetRate,
               FlowMonitorHelper& flowmon,
               Ptr<FlowMonitor> monitor,
               double simTime,
               std::string topologyType)
{
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    std::ofstream detailFile(outputDir + "/" + detailFilename);
    detailFile << "Flow statistics for " << topologyType << " topology\n";
    detailFile << "TCP: " << tcpType << ", Nodes: " << nNodes << ", Flows: " << nFlows
               << ", Rate: " << packetRate << " pkt/s\n\n";

    double totalThroughput = 0.0;
    double totalDelay = 0.0;
    double totalLost = 0.0;
    double totalTx = 0.0;
    std::vector<double> flowThroughputs;
    uint32_t flowCount = 0;

    // Only count flows with significant data transfer (> 100 packets)
    // This filters out ACK-only reverse flows
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin();
         i != stats.end();
         ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);

        double throughput = i->second.rxBytes * 8.0 / simTime / 1000.0;
        double delay = 0.0;
        if (i->second.rxPackets > 0)
        {
            delay = i->second.delaySum.GetMilliSeconds() / i->second.rxPackets;
        }

        detailFile << "Flow " << i->first << " (" << t.sourceAddress << " -> "
                   << t.destinationAddress << ")\n";
        detailFile << "  Tx Packets: " << i->second.txPackets << "\n";
        detailFile << "  Rx Packets: " << i->second.rxPackets << "\n";
        detailFile << "  Lost Packets: " << i->second.lostPackets << "\n";
        detailFile << "  Throughput: " << throughput << " kbps\n";
        detailFile << "  Mean Delay: " << delay << " ms\n\n";

        // Only include flows with significant data (filters out ACK-only flows)
        if (i->second.txPackets > 100)
        {
            totalThroughput += throughput;
            if (i->second.rxPackets > 0)
            {
                totalDelay += delay;
            }
            totalLost += i->second.lostPackets;
            totalTx += i->second.txPackets;
            flowThroughputs.push_back(throughput);
            flowCount++;
        }
    }

    double avgThroughput = totalThroughput / flowCount;
    double avgDelay = totalDelay / flowCount;
    double lossRate = (totalLost / totalTx) * 100.0;

    // Calculate Jain's Fairness Index
    double sumThroughput = 0.0;
    double sumSquaredThroughput = 0.0;
    for (double tp : flowThroughputs)
    {
        sumThroughput += tp;
        sumSquaredThroughput += tp * tp;
    }
    double jainsFairness =
        (sumThroughput * sumThroughput) / (flowThroughputs.size() * sumSquaredThroughput);

    detailFile << "=== Summary Statistics (Data Flows Only) ===\n";
    detailFile << "Data Flows Counted: " << flowCount << "\n";
    detailFile << "Average Throughput: " << avgThroughput << " kbps\n";
    detailFile << "Average Delay: " << avgDelay << " ms\n";
    detailFile << "Loss Rate: " << lossRate << " %\n";
    detailFile << "Jain's Fairness Index: " << jainsFairness << "\n";
    detailFile << "Total Goodput: " << totalThroughput << " kbps\n";

    detailFile.close();

    // Append to CSV summary
    std::ofstream csvFile(outputDir + "/phase2_summary.csv", std::ios::app);
    std::string tcpShort = tcpType.substr(5); // Changed from 6 to 5 to get "TcpNewReno"
    csvFile << topologyType << "," << tcpShort << "," << nNodes << "," << flowCount << ","
            << packetRate << "," << avgThroughput << "," << avgDelay << "," << lossRate << ","
            << jainsFairness << "," << totalThroughput << std::endl;
    csvFile.close();

    std::cout << "\nSummary (Data Flows: " << flowCount << "):" << std::endl;
    std::cout << "  Avg Throughput: " << avgThroughput << " kbps" << std::endl;
    std::cout << "  Avg Delay: " << avgDelay << " ms" << std::endl;
    std::cout << "  Loss Rate: " << lossRate << " %" << std::endl;
    std::cout << "  Jain's Fairness: " << jainsFairness << std::endl;
}

// DUMBBELL TOPOLOGY WITH VARIABLE NODES
void
RunDumbbellSimulation(std::string tcpType,
                      uint32_t nNodes,
                      uint32_t nFlows,
                      double packetRate,
                      std::string outputDir,
                      double simTime)
{
    std::cout << "\n========================================" << std::endl;
    std::cout << "DUMBBELL TOPOLOGY" << std::endl;
    std::cout << "TCP: " << tcpType << std::endl;
    std::cout << "Nodes per side: " << nNodes << std::endl;
    std::cout << "Flows: " << nFlows << std::endl;
    std::cout << "Packet Rate: " << packetRate << " pkt/s" << std::endl;
    std::cout << "========================================" << std::endl;

    // Clear previous cwnd files
    for (auto& entry : g_cwndFiles)
    {
        if (entry.second && entry.second->is_open())
        {
            entry.second->close();
            delete entry.second;
        }
    }
    g_cwndFiles.clear();

    // Create output directory for cwnd files
    system(("mkdir -p " + outputDir + "/cwnd").c_str());

    // Create nodes
    NodeContainer senders, receivers, routers;
    senders.Create(nNodes);
    receivers.Create(nNodes);
    routers.Create(2);

    // Set up mobility for NetAnim
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    // Position senders (left side)
    double spacing = 40.0 / nNodes;
    for (uint32_t i = 0; i < nNodes; i++)
    {
        positionAlloc->Add(Vector(0.0, i * spacing, 0.0));
    }

    // Position routers (middle)
    positionAlloc->Add(Vector(50.0, 20.0, 0.0));  // Router 0
    positionAlloc->Add(Vector(100.0, 20.0, 0.0)); // Router 1

    // Position receivers (right side)
    for (uint32_t i = 0; i < nNodes; i++)
    {
        positionAlloc->Add(Vector(150.0, i * spacing, 0.0));
    }

    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(senders);
    mobility.Install(routers);
    mobility.Install(receivers);

    // Configure TCP
    if (tcpType == "ns3::TcpReno")
    {
        tcpType = "ns3::TcpNewReno";
    }

    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue(tcpType));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(10));
    Config::SetDefault("ns3::TcpSocketBase::Sack", BooleanValue(true));
    Config::SetDefault("ns3::TcpSocketBase::WindowScaling", BooleanValue(true));

    // Install Internet stack
    InternetStackHelper internet;
    internet.Install(senders);
    internet.Install(receivers);
    internet.Install(routers);

    // Configure queue size
    Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize", QueueSizeValue(QueueSize("100p")));

    // Create links
    PointToPointHelper accessLink;
    accessLink.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    accessLink.SetChannelAttribute("Delay", StringValue("20ms"));
    accessLink.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", QueueSizeValue(QueueSize("100p")));

    PointToPointHelper bottleneckLink;
    bottleneckLink.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    bottleneckLink.SetChannelAttribute("Delay", StringValue("50ms"));
    bottleneckLink.SetQueue("ns3::DropTailQueue<Packet>",
                            "MaxSize",
                            QueueSizeValue(QueueSize("100p")));

    // Install links
    std::vector<NetDeviceContainer> senderDevices, receiverDevices;

    for (uint32_t i = 0; i < nNodes; i++)
    {
        senderDevices.push_back(accessLink.Install(senders.Get(i), routers.Get(0)));
        receiverDevices.push_back(accessLink.Install(routers.Get(1), receivers.Get(i)));
    }

    NetDeviceContainer routerDevices = bottleneckLink.Install(routers.Get(0), routers.Get(1));

    // Assign IP addresses
    Ipv4AddressHelper ipv4;
    std::vector<Ipv4InterfaceContainer> senderInterfaces, receiverInterfaces;

    for (uint32_t i = 0; i < nNodes; i++)
    {
        std::ostringstream subnet;
        subnet << "10.1." << (i + 1) << ".0";
        ipv4.SetBase(subnet.str().c_str(), "255.255.255.0");
        senderInterfaces.push_back(ipv4.Assign(senderDevices[i]));
    }

    ipv4.SetBase("10.2.1.0", "255.255.255.0");
    Ipv4InterfaceContainer routerInterfaces = ipv4.Assign(routerDevices);

    for (uint32_t i = 0; i < nNodes; i++)
    {
        std::ostringstream subnet;
        subnet << "10.3." << (i + 1) << ".0";
        ipv4.SetBase(subnet.str().c_str(), "255.255.255.0");
        receiverInterfaces.push_back(ipv4.Assign(receiverDevices[i]));
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Create applications - UNIDIRECTIONAL ONLY (left to right)
    uint16_t basePort = 5000;
    uint32_t flowId = 0;

    uint32_t flowsPerSender = nFlows / nNodes;
    uint32_t extraFlows = nFlows % nNodes;

    for (uint32_t i = 0; i < nNodes; i++)
    {
        uint32_t flowsForThisSender = flowsPerSender;
        if (i < extraFlows)
        {
            flowsForThisSender++;
        }

        uint32_t receiverIdx = i % nNodes;
        Ipv4Address receiverAddr = receiverInterfaces[receiverIdx].GetAddress(1);

        for (uint32_t f = 0; f < flowsForThisSender; f++)
        {
            uint16_t port = basePort + flowId;

            // Install sink on receiver
            PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
                                        InetSocketAddress(Ipv4Address::GetAny(), port));
            ApplicationContainer sinkApp = sinkHelper.Install(receivers.Get(receiverIdx));
            sinkApp.Start(Seconds(0.0));
            sinkApp.Stop(Seconds(simTime + 5.0));

            // Create sender socket
            Ptr<Socket> tcpSocket =
                Socket::CreateSocket(senders.Get(i), TcpSocketFactory::GetTypeId());

            // Setup cwnd tracing
            std::string cwndFilename = outputDir + "/cwnd/dumbbell_" + tcpType.substr(5) + "_n" +
                                       std::to_string(nNodes) + "_flow" + std::to_string(flowId) +
                                       ".csv";
            g_cwndFiles[flowId] = new std::ofstream(cwndFilename);
            *g_cwndFiles[flowId] << "Time,Cwnd" << std::endl;

            std::ostringstream oss;
            oss << "/NodeList/" << senders.Get(i)->GetId() << "/$ns3::TcpL4Protocol/SocketList/"
                << f << "/CongestionWindow";
            Config::ConnectWithoutContext(oss.str(), MakeBoundCallback(&CwndTracer, flowId));

            // Create rate-controlled sender
            Ptr<RateControlledSender> senderApp = CreateObject<RateControlledSender>();
            senders.Get(i)->AddApplication(senderApp);

            senderApp->Setup(tcpSocket,
                             InetSocketAddress(receiverAddr, port),
                             1448,
                             packetRate,
                             flowId,
                             outputDir);

            senderApp->SetStartTime(Seconds(1.0 + f * 0.01));
            senderApp->SetStopTime(Seconds(simTime));

            flowId++;
        }
    }

    std::cout << "Total flows created: " << flowId << std::endl;

    // Install FlowMonitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    // Enable NetAnim
    std::string tcpShort = tcpType.substr(5);
    std::string animFile =
        outputDir + "/anim-dumbbell-" + tcpShort + "-n" + std::to_string(nNodes) + ".xml";

    AnimationInterface anim(animFile);

    // Color nodes
    for (uint32_t i = 0; i < nNodes; i++)
    {
        anim.UpdateNodeDescription(senders.Get(i), "S" + std::to_string(i));
        anim.UpdateNodeColor(senders.Get(i), 0, 255, 0); // Green
    }

    anim.UpdateNodeDescription(routers.Get(0), "R0");
    anim.UpdateNodeDescription(routers.Get(1), "R1");
    anim.UpdateNodeColor(routers.Get(0), 0, 0, 255); // Blue
    anim.UpdateNodeColor(routers.Get(1), 0, 0, 255);

    for (uint32_t i = 0; i < nNodes; i++)
    {
        anim.UpdateNodeDescription(receivers.Get(i), "D" + std::to_string(i));
        anim.UpdateNodeColor(receivers.Get(i), 255, 0, 0); // Red
    }

    anim.EnablePacketMetadata(true);

    // Run simulation
    std::cout << "\nRunning simulation..." << std::endl;
    Simulator::Stop(Seconds(simTime + 5.0));
    Simulator::Run();

    // Save statistics
    std::string detailFilename = "details-dumbbell-" + tcpShort + "-n" + std::to_string(nNodes) +
                                 "-f" + std::to_string(nFlows) + ".txt";
    SaveStatistics(outputDir,
                   detailFilename,
                   tcpType,
                   nNodes,
                   nFlows,
                   packetRate,
                   flowmon,
                   monitor,
                   simTime,
                   "Dumbbell");

    // Cleanup
    for (auto& entry : g_cwndFiles)
    {
        if (entry.second && entry.second->is_open())
        {
            entry.second->close();
            delete entry.second;
        }
    }
    g_cwndFiles.clear();

    Simulator::Destroy();
    std::cout << "Simulation completed!\n" << std::endl;
}

// BUS TOPOLOGY
void
RunBusSimulation(std::string tcpType,
                 uint32_t nNodes,
                 uint32_t nFlows,
                 double packetRate,
                 std::string outputDir,
                 double simTime)
{
    std::cout << "\n========================================" << std::endl;
    std::cout << "BUS TOPOLOGY" << std::endl;
    std::cout << "TCP: " << tcpType << std::endl;
    std::cout << "Nodes: " << nNodes << std::endl;
    std::cout << "Flows: " << nFlows << std::endl;
    std::cout << "Packet Rate: " << packetRate << " pkt/s" << std::endl;
    std::cout << "========================================" << std::endl;

    // Clear previous cwnd files
    for (auto& entry : g_cwndFiles)
    {
        if (entry.second && entry.second->is_open())
        {
            entry.second->close();
            delete entry.second;
        }
    }
    g_cwndFiles.clear();

    system(("mkdir -p " + outputDir + "/cwnd").c_str());

    // Create nodes
    NodeContainer busNodes;
    busNodes.Create(nNodes);

    // Set up mobility
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    double spacing = 150.0 / (nNodes - 1);
    for (uint32_t i = 0; i < nNodes; i++)
    {
        positionAlloc->Add(Vector(i * spacing, 20.0, 0.0));
    }

    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(busNodes);

    // Configure TCP
    if (tcpType == "ns3::TcpReno")
    {
        tcpType = "ns3::TcpNewReno";
    }

    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue(tcpType));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(10));
    Config::SetDefault("ns3::TcpSocketBase::Sack", BooleanValue(true));
    Config::SetDefault("ns3::TcpSocketBase::WindowScaling", BooleanValue(true));

    InternetStackHelper internet;
    internet.Install(busNodes);

    Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize", QueueSizeValue(QueueSize("100p")));

    // Create bus links
    PointToPointHelper busLink;
    busLink.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    busLink.SetChannelAttribute("Delay", StringValue("20ms"));
    busLink.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", QueueSizeValue(QueueSize("100p")));

    std::vector<NetDeviceContainer> busDevices;

    for (uint32_t i = 0; i < nNodes - 1; i++)
    {
        NetDeviceContainer link = busLink.Install(busNodes.Get(i), busNodes.Get(i + 1));
        busDevices.push_back(link);
    }

    // Assign IP addresses
    Ipv4AddressHelper ipv4;
    std::vector<Ipv4InterfaceContainer> busInterfaces;

    for (uint32_t i = 0; i < busDevices.size(); i++)
    {
        std::ostringstream subnet;
        subnet << "10.1." << (i + 1) << ".0";
        ipv4.SetBase(subnet.str().c_str(), "255.255.255.0");
        busInterfaces.push_back(ipv4.Assign(busDevices[i]));
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Create applications - UNIDIRECTIONAL (first half to second half)
    uint16_t basePort = 5000;
    uint32_t flowId = 0;

    uint32_t numSenders = nNodes / 2;
    uint32_t numReceivers = nNodes - numSenders;

    uint32_t flowsPerSender = nFlows / numSenders;
    uint32_t extraFlows = nFlows % numSenders;

    for (uint32_t senderIdx = 0; senderIdx < numSenders; senderIdx++)
    {
        uint32_t flowsForThisSender = flowsPerSender;
        if (senderIdx < extraFlows)
        {
            flowsForThisSender++;
        }

        uint32_t receiverIdx = numSenders + (senderIdx % numReceivers);

        Ptr<Node> senderNode = busNodes.Get(senderIdx);
        Ptr<Node> receiverNode = busNodes.Get(receiverIdx);

        // Get receiver's IP address
        Ipv4Address receiverAddr = receiverNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

        for (uint32_t f = 0; f < flowsForThisSender; f++)
        {
            uint16_t port = basePort + flowId;

            PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
                                        InetSocketAddress(Ipv4Address::GetAny(), port));
            ApplicationContainer sinkApp = sinkHelper.Install(receiverNode);
            sinkApp.Start(Seconds(0.0));
            sinkApp.Stop(Seconds(simTime + 5.0));

            Ptr<Socket> tcpSocket = Socket::CreateSocket(senderNode, TcpSocketFactory::GetTypeId());

            std::string cwndFilename = outputDir + "/cwnd/bus_" + tcpType.substr(5) + "_n" +
                                       std::to_string(nNodes) + "_flow" + std::to_string(flowId) +
                                       ".csv";
            g_cwndFiles[flowId] = new std::ofstream(cwndFilename);
            *g_cwndFiles[flowId] << "Time,Cwnd" << std::endl;

            std::ostringstream oss;
            oss << "/NodeList/" << senderNode->GetId() << "/$ns3::TcpL4Protocol/SocketList/" << f
                << "/CongestionWindow";
            Config::ConnectWithoutContext(oss.str(), MakeBoundCallback(&CwndTracer, flowId));

            Ptr<RateControlledSender> senderApp = CreateObject<RateControlledSender>();
            senderNode->AddApplication(senderApp);

            senderApp->Setup(tcpSocket,
                             InetSocketAddress(receiverAddr, port),
                             1448,
                             packetRate,
                             flowId,
                             outputDir);

            senderApp->SetStartTime(Seconds(1.0 + f * 0.01));
            senderApp->SetStopTime(Seconds(simTime));

            flowId++;
        }
    }

    std::cout << "Total flows created: " << flowId << std::endl;

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    std::string tcpShort = tcpType.substr(5);
    std::string animFile =
        outputDir + "/anim-bus-" + tcpShort + "-n" + std::to_string(nNodes) + ".xml";

    AnimationInterface anim(animFile);

    for (uint32_t i = 0; i < nNodes; i++)
    {
        anim.UpdateNodeDescription(busNodes.Get(i), "N" + std::to_string(i));

        if (i < numSenders)
        {
            anim.UpdateNodeColor(busNodes.Get(i), 0, 255, 0); // Green for senders
        }
        else
        {
            anim.UpdateNodeColor(busNodes.Get(i), 255, 0, 0); // Red for receivers
        }
    }

    anim.EnablePacketMetadata(true);

    std::cout << "\nRunning simulation..." << std::endl;
    Simulator::Stop(Seconds(simTime + 5.0));
    Simulator::Run();

    std::string detailFilename = "details-bus-" + tcpShort + "-n" + std::to_string(nNodes) + "-f" +
                                 std::to_string(nFlows) + ".txt";
    SaveStatistics(outputDir,
                   detailFilename,
                   tcpType,
                   nNodes,
                   nFlows,
                   packetRate,
                   flowmon,
                   monitor,
                   simTime,
                   "Bus");

    for (auto& entry : g_cwndFiles)
    {
        if (entry.second && entry.second->is_open())
        {
            entry.second->close();
            delete entry.second;
        }
    }
    g_cwndFiles.clear();

    Simulator::Destroy();
    std::cout << "Simulation completed!\n" << std::endl;
}

// TREE TOPOLOGY
// TREE TOPOLOGY
void
RunTreeSimulation(std::string tcpType,
                  uint32_t depth,
                  uint32_t nFlows,
                  double packetRate,
                  std::string outputDir,
                  double simTime)
{
    std::cout << "\n========================================" << std::endl;
    std::cout << "TREE TOPOLOGY" << std::endl;
    std::cout << "TCP: " << tcpType << std::endl;
    std::cout << "Tree Depth: " << depth << std::endl;
    std::cout << "Flows: " << nFlows << std::endl;
    std::cout << "Packet Rate: " << packetRate << " pkt/s" << std::endl;
    std::cout << "========================================" << std::endl;

    // Clear previous cwnd files
    for (auto& entry : g_cwndFiles)
    {
        if (entry.second && entry.second->is_open())
        {
            entry.second->close();
            delete entry.second;
        }
    }
    g_cwndFiles.clear();

    system(("mkdir -p " + outputDir + "/cwnd").c_str());

    // Calculate number of nodes in binary tree
    uint32_t nNodes = (1 << (depth + 1)) - 1; // 2^(depth+1) - 1

    NodeContainer treeNodes;
    treeNodes.Create(nNodes);

    // Set up mobility
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    // Position nodes in tree structure
    for (uint32_t level = 0; level <= depth; level++)
    {
        uint32_t nodesInLevel = 1 << level;
        uint32_t startIdx = (1 << level) - 1;

        double levelY = level * 40.0;
        double levelWidth = 150.0;
        double spacing = levelWidth / (nodesInLevel + 1);

        for (uint32_t i = 0; i < nodesInLevel; i++)
        {
            double x = (i + 1) * spacing;
            positionAlloc->Add(Vector(x, levelY, 0.0));
        }
    }

    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(treeNodes);

    // Configure TCP
    if (tcpType == "ns3::TcpReno")
    {
        tcpType = "ns3::TcpNewReno";
    }

    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue(tcpType));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(10));
    Config::SetDefault("ns3::TcpSocketBase::Sack", BooleanValue(true));
    Config::SetDefault("ns3::TcpSocketBase::WindowScaling", BooleanValue(true));

    InternetStackHelper internet;
    internet.Install(treeNodes);

    Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize", QueueSizeValue(QueueSize("100p")));

    // Create tree links
    PointToPointHelper treeLink;
    treeLink.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    treeLink.SetChannelAttribute("Delay", StringValue("20ms"));
    treeLink.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", QueueSizeValue(QueueSize("100p")));

    std::vector<NetDeviceContainer> treeDevices;

    // Connect parent to children
    for (uint32_t i = 0; i < nNodes; i++)
    {
        uint32_t leftChild = 2 * i + 1;
        uint32_t rightChild = 2 * i + 2;

        if (leftChild < nNodes)
        {
            NetDeviceContainer link = treeLink.Install(treeNodes.Get(i), treeNodes.Get(leftChild));
            treeDevices.push_back(link);
        }

        if (rightChild < nNodes)
        {
            NetDeviceContainer link = treeLink.Install(treeNodes.Get(i), treeNodes.Get(rightChild));
            treeDevices.push_back(link);
        }
    }

    // Assign IP addresses
    Ipv4AddressHelper ipv4;
    std::vector<Ipv4InterfaceContainer> treeInterfaces;

    for (uint32_t i = 0; i < treeDevices.size(); i++)
    {
        std::ostringstream subnet;
        subnet << "10.1." << (i / 250 + 1) << "." << (i % 250) * 4;
        ipv4.SetBase(subnet.str().c_str(), "255.255.255.252");
        treeInterfaces.push_back(ipv4.Assign(treeDevices[i]));
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Create applications - UNIDIRECTIONAL (leaf nodes to root)
    uint16_t basePort = 5000;
    uint32_t flowId = 0;

    // Identify leaf nodes (last level of tree)
    uint32_t leafStart = (1 << depth) - 1;
    uint32_t numLeaves = 1 << depth;

    // Root node is the receiver (node 0)
    Ptr<Node> rootNode = treeNodes.Get(0);
    Ipv4Address rootAddr = rootNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

    uint32_t flowsPerLeaf = nFlows / numLeaves;
    uint32_t extraFlows = nFlows % numLeaves;

    for (uint32_t leafIdx = 0; leafIdx < numLeaves; leafIdx++)
    {
        uint32_t flowsForThisLeaf = flowsPerLeaf;
        if (leafIdx < extraFlows)
        {
            flowsForThisLeaf++;
        }

        Ptr<Node> leafNode = treeNodes.Get(leafStart + leafIdx);

        for (uint32_t f = 0; f < flowsForThisLeaf; f++)
        {
            uint16_t port = basePort + flowId;

            // Install sink on root
            PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
                                        InetSocketAddress(Ipv4Address::GetAny(), port));
            ApplicationContainer sinkApp = sinkHelper.Install(rootNode);
            sinkApp.Start(Seconds(0.0));
            sinkApp.Stop(Seconds(simTime + 5.0));

            // Create sender socket on leaf
            Ptr<Socket> tcpSocket = Socket::CreateSocket(leafNode, TcpSocketFactory::GetTypeId());

            std::string cwndFilename = outputDir + "/cwnd/tree_" + tcpType.substr(5) + "_d" +
                                       std::to_string(depth) + "_flow" + std::to_string(flowId) +
                                       ".csv";
            g_cwndFiles[flowId] = new std::ofstream(cwndFilename);
            *g_cwndFiles[flowId] << "Time,Cwnd" << std::endl;

            std::ostringstream oss;
            oss << "/NodeList/" << leafNode->GetId() << "/$ns3::TcpL4Protocol/SocketList/" << f
                << "/CongestionWindow";
            Config::ConnectWithoutContext(oss.str(), MakeBoundCallback(&CwndTracer, flowId));

            Ptr<RateControlledSender> senderApp = CreateObject<RateControlledSender>();
            leafNode->AddApplication(senderApp);

            senderApp->Setup(tcpSocket,
                             InetSocketAddress(rootAddr, port),
                             1448,
                             packetRate,
                             flowId,
                             outputDir);

            senderApp->SetStartTime(Seconds(1.0 + f * 0.01));
            senderApp->SetStopTime(Seconds(simTime));

            flowId++;
        }
    }

    std::cout << "Total flows created: " << flowId << std::endl;

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    std::string tcpShort = tcpType.substr(5);
    std::string animFile =
        outputDir + "/anim-tree-" + tcpShort + "-d" + std::to_string(depth) + ".xml";

    AnimationInterface anim(animFile);

    // Color root node
    anim.UpdateNodeDescription(treeNodes.Get(0), "Root");
    anim.UpdateNodeColor(treeNodes.Get(0), 255, 0, 0); // Red for receiver

    // Color intermediate nodes
    for (uint32_t i = 1; i < leafStart; i++)
    {
        anim.UpdateNodeDescription(treeNodes.Get(i), "I" + std::to_string(i));
        anim.UpdateNodeColor(treeNodes.Get(i), 0, 0, 255); // Blue for intermediate
    }

    // Color leaf nodes
    for (uint32_t i = leafStart; i < nNodes; i++)
    {
        anim.UpdateNodeDescription(treeNodes.Get(i), "L" + std::to_string(i - leafStart));
        anim.UpdateNodeColor(treeNodes.Get(i), 0, 255, 0); // Green for senders
    }

    anim.EnablePacketMetadata(true);

    std::cout << "\nRunning simulation..." << std::endl;
    Simulator::Stop(Seconds(simTime + 5.0));
    Simulator::Run();

    std::string detailFilename = "details-tree-" + tcpShort + "-d" + std::to_string(depth) + "-f" +
                                 std::to_string(nFlows) + ".txt";
    SaveStatistics(outputDir,
                   detailFilename,
                   tcpType,
                   nNodes,
                   nFlows,
                   packetRate,
                   flowmon,
                   monitor,
                   simTime,
                   "Tree");

    for (auto& entry : g_cwndFiles)
    {
        if (entry.second && entry.second->is_open())
        {
            entry.second->close();
            delete entry.second;
        }
    }
    g_cwndFiles.clear();

    Simulator::Destroy();
    std::cout << "Simulation completed!\n" << std::endl;
}

// MAIN FUNCTION
int
main(int argc, char* argv[])
{
    std::string outputDir = "results_phase2";
    double simTime = 40.0;
    uint32_t nFlows = 20;
    double packetRate = 300.0;

    bool runAll = false;
    std::string topology = "all";
    std::string tcpType = "all";
    uint32_t nNodes = 5;
    uint32_t treeDepth = 2;

    CommandLine cmd;
    cmd.AddValue("outputDir", "Output directory", outputDir);
    cmd.AddValue("simTime", "Simulation time (seconds)", simTime);
    cmd.AddValue("nFlows", "Number of flows", nFlows);
    cmd.AddValue("packetRate", "Packet rate (packets/sec)", packetRate);
    cmd.AddValue("runAll", "Run all test combinations", runAll);
    cmd.AddValue("topology", "Topology type (dumbbell/bus/tree/all)", topology);
    cmd.AddValue("tcpType", "TCP type (NewReno/CUBIC/Vegas/all)", tcpType);
    cmd.AddValue("nNodes", "Number of nodes (for dumbbell/bus)", nNodes);
    cmd.AddValue("treeDepth", "Tree depth (for tree topology)", treeDepth);
    cmd.Parse(argc, argv);

    // Create output directory
    system(("mkdir -p " + outputDir).c_str());

    // Initialize CSV summary file
    std::ofstream csvFile(outputDir + "/phase2_summary.csv");
    csvFile << "Topology,TCP,Nodes,Flows,PacketRate,AvgThroughput(kbps),AvgDelay(ms),"
            << "LossRate(%),JainsFairness,TotalGoodput(kbps)" << std::endl;
    csvFile.close();

    if (runAll)
    {
        std::cout << "\n==================================================" << std::endl;
        std::cout << "RUNNING COMPLETE PHASE 2 TEST MATRIX" << std::endl;
        std::cout << "Fixed: nFlows=" << nFlows << ", packetRate=" << packetRate << std::endl;
        std::cout << "==================================================" << std::endl;

        std::vector<std::string> tcpTypes = {"ns3::TcpNewReno", "ns3::TcpCubic", "ns3::TcpVegas"};

        // Dumbbell topology tests (N = 3, 5, 7)
        std::vector<uint32_t> dumbbellNodes = {3, 5, 7};
        for (const auto& tcp : tcpTypes)
        {
            for (uint32_t n : dumbbellNodes)
            {
                RunDumbbellSimulation(tcp, n, nFlows, packetRate, outputDir, simTime);
            }
        }

        // Bus topology tests (N = 6, 8, 10)
        std::vector<uint32_t> busNodes = {6, 8, 10};
        for (const auto& tcp : tcpTypes)
        {
            for (uint32_t n : busNodes)
            {
                RunBusSimulation(tcp, n, nFlows, packetRate, outputDir, simTime);
            }
        }

        // Tree topology tests (D = 2, 3)
        std::vector<uint32_t> treeDepths = {2, 3};
        for (const auto& tcp : tcpTypes)
        {
            for (uint32_t d : treeDepths)
            {
                RunTreeSimulation(tcp, d, nFlows, packetRate, outputDir, simTime);
            }
        }

        std::cout << "\n==================================================" << std::endl;
        std::cout << "ALL PHASE 2 SIMULATIONS COMPLETED!" << std::endl;
        std::cout << "Results saved to: " << outputDir << "/phase2_summary.csv" << std::endl;
        std::cout << "==================================================" << std::endl;
    }
    else
    {
        // Single simulation mode
        std::vector<std::string> tcpTypes;
        if (tcpType == "all")
        {
            tcpTypes = {"ns3::TcpNewReno", "ns3::TcpCubic", "ns3::TcpVegas"};
        }
        else if (tcpType == "NewReno")
        {
            tcpTypes = {"ns3::TcpNewReno"};
        }
        else if (tcpType == "CUBIC")
        {
            tcpTypes = {"ns3::TcpCubic"};
        }
        else if (tcpType == "Vegas")
        {
            tcpTypes = {"ns3::TcpVegas"};
        }

        for (const auto& tcp : tcpTypes)
        {
            if (topology == "dumbbell" || topology == "all")
            {
                RunDumbbellSimulation(tcp, nNodes, nFlows, packetRate, outputDir, simTime);
            }

            if (topology == "bus" || topology == "all")
            {
                RunBusSimulation(tcp, nNodes, nFlows, packetRate, outputDir, simTime);
            }

            if (topology == "tree" || topology == "all")
            {
                RunTreeSimulation(tcp, treeDepth, nFlows, packetRate, outputDir, simTime);
            }
        }
    }

    return 0;
}
