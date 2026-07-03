/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * TCP Dumbbell Simulation - Correct Implementation
 * Fixed topology: 3 senders + 2 routers + 3 receivers
 * Variable: number of flows (5, 20, 50) and packet rate (100, 300, 1000 pkt/s)
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"
#include "ns3/traffic-control-module.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpDumbbellProject");

// Global variables for cwnd tracking
std::map<uint32_t, std::ofstream*> g_cwndFiles;

// Callback for cwnd changes
void CwndTracer(uint32_t flowId, uint32_t oldCwnd, uint32_t newCwnd)
{
    if (g_cwndFiles.find(flowId) != g_cwndFiles.end()) {
        *g_cwndFiles[flowId] << Simulator::Now().GetSeconds() << "\t" 
                             << newCwnd << std::endl;
    }
}

// Connect cwnd trace to socket
void ConnectCwndTrace(uint32_t flowId, Ptr<Socket> socket, std::string outputDir)
{
    std::string filename = outputDir + "/cwnd/cwnd-flow-" + std::to_string(flowId) + ".dat";
    g_cwndFiles[flowId] = new std::ofstream(filename);
    
    if (g_cwndFiles[flowId]->is_open()) {
        *g_cwndFiles[flowId] << "# Time(s)\tCwnd(segments)" << std::endl;
        socket->TraceConnectWithoutContext("CongestionWindow",
            MakeBoundCallback(&CwndTracer, flowId));
    }
}

// Calculate Jain's Fairness Index
double CalculateJainsFairnessIndex(const std::vector<double>& throughputs)
{
    if (throughputs.empty()) return 0.0;
    
    double sum = 0.0;
    double sumSquares = 0.0;
    
    for (double thr : throughputs) {
        sum += thr;
        sumSquares += thr * thr;
    }
    
    if (sumSquares == 0.0) return 0.0;
    
    return (sum * sum) / (throughputs.size() * sumSquares);
}

// Custom OnOff application with specific packet rate
class RateControlledSender : public Application
{
public:
    RateControlledSender();
    virtual ~RateControlledSender();
    
    void Setup(Ptr<Socket> socket, Address address, uint32_t packetSize,
               double packetsPerSecond, uint32_t flowId, std::string outputDir);

private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);
    void SendPacket(void);
    
    Ptr<Socket>     m_socket;
    Address         m_peer;
    uint32_t        m_packetSize;
    double          m_packetsPerSecond;
    EventId         m_sendEvent;
    bool            m_running;
    uint32_t        m_flowId;
    std::string     m_outputDir;
};

RateControlledSender::RateControlledSender()
    : m_socket(0),
      m_peer(),
      m_packetSize(0),
      m_packetsPerSecond(0),
      m_sendEvent(),
      m_running(false),
      m_flowId(0),
      m_outputDir("")
{
}

RateControlledSender::~RateControlledSender()
{
    m_socket = 0;
}

void RateControlledSender::Setup(Ptr<Socket> socket, Address address,
                                 uint32_t packetSize, double packetsPerSecond,
                                 uint32_t flowId, std::string outputDir)
{
    m_socket = socket;
    m_peer = address;
    m_packetSize = packetSize;
    m_packetsPerSecond = packetsPerSecond;
    m_flowId = flowId;
    m_outputDir = outputDir;
}

void RateControlledSender::StartApplication(void)
{
    m_running = true;
    m_socket->Bind();
    m_socket->Connect(m_peer);
    
    // Connect cwnd trace
    ConnectCwndTrace(m_flowId, m_socket, m_outputDir);
    
    SendPacket();
}

void RateControlledSender::StopApplication(void)
{
    m_running = false;
    
    if (m_sendEvent.IsPending()) {  // ✅ FIXED: IsPending() instead of IsRunning()
        Simulator::Cancel(m_sendEvent);
    }
    
    if (m_socket) {
        m_socket->Close();
    }
}


void RateControlledSender::SendPacket(void)
{
    if (m_running) {
        Ptr<Packet> packet = Create<Packet>(m_packetSize);
        m_socket->Send(packet);
        
        // Schedule next packet based on desired rate
        double interval = 1.0 / m_packetsPerSecond; // seconds
        Time tNext = Seconds(interval);
        m_sendEvent = Simulator::Schedule(tNext, &RateControlledSender::SendPacket, this);
    }
}

// Main simulation function
void RunSimulation(std::string tcpType, uint32_t nFlows, double packetRate,
                   std::string outputDir, double simTime)
{
    std::cout << "\n========================================" << std::endl;
    std::cout << "TCP: " << tcpType << std::endl;
    std::cout << "Flows: " << nFlows << std::endl;
    std::cout << "Packet Rate: " << packetRate << " pkt/s" << std::endl;
    std::cout << "Simulation Time: " << simTime << " s" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Clean up previous cwnd files
    for (auto& entry : g_cwndFiles) {
        if (entry.second && entry.second->is_open()) {
            entry.second->close();
            delete entry.second;
        }
    }
    g_cwndFiles.clear();
    
    // Create directories
    system(("mkdir -p " + outputDir).c_str());
    system(("mkdir -p " + outputDir + "/cwnd").c_str());
    
    // Create nodes: 3 senders, 2 routers, 3 receivers (fixed topology)
    NodeContainer senders;
    senders.Create(3);
    
    NodeContainer routers;
    routers.Create(2);
    
    NodeContainer receivers;
    receivers.Create(3);
    
    // Set up mobility for NetAnim visualization
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    
    // Left side: senders
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));    // Sender 0
    positionAlloc->Add(Vector(0.0, 30.0, 0.0));   // Sender 1
    positionAlloc->Add(Vector(0.0, 60.0, 0.0));   // Sender 2
    
    // Middle: routers
    positionAlloc->Add(Vector(50.0, 30.0, 0.0));  // Router 0 (left)
    positionAlloc->Add(Vector(100.0, 30.0, 0.0)); // Router 1 (right)
    
    // Right side: receivers
    positionAlloc->Add(Vector(150.0, 0.0, 0.0));   // Receiver 0
    positionAlloc->Add(Vector(150.0, 30.0, 0.0));  // Receiver 1
    positionAlloc->Add(Vector(150.0, 60.0, 0.0));  // Receiver 2
    
    mobility.SetPositionAllocator(positionAlloc);
    
    NodeContainer allNodes;
    allNodes.Add(senders);
    allNodes.Add(routers);
    allNodes.Add(receivers);
    mobility.Install(allNodes);
    
    // Configure TCP variant
    if (tcpType == "ns3::TcpReno") {
        tcpType = "ns3::TcpNewReno"; // Compatibility
    }
    
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue(tcpType));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(10));
    Config::SetDefault("ns3::TcpSocketBase::Sack", BooleanValue(true));
    Config::SetDefault("ns3::TcpSocketBase::WindowScaling", BooleanValue(true));
    
    // Install Internet stack
    InternetStackHelper internet;
    internet.Install(allNodes);
    
    // Set queue size to 100 packets (as specified)
    Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize", 
                       QueueSizeValue(QueueSize("100p")));
    
    // Create point-to-point links
    PointToPointHelper accessLink;
    accessLink.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    accessLink.SetChannelAttribute("Delay", StringValue("20ms"));
    accessLink.SetQueue("ns3::DropTailQueue<Packet>", 
                        "MaxSize", QueueSizeValue(QueueSize("100p")));
    
    // Connect senders to left router
    std::vector<NetDeviceContainer> senderDevices;
    for (uint32_t i = 0; i < 3; i++) {
        NetDeviceContainer link = accessLink.Install(senders.Get(i), routers.Get(0));
        senderDevices.push_back(link);
    }
    
    // Connect receivers to right router
    std::vector<NetDeviceContainer> receiverDevices;
    for (uint32_t i = 0; i < 3; i++) {
        NetDeviceContainer link = accessLink.Install(receivers.Get(i), routers.Get(1));
        receiverDevices.push_back(link);
    }
    
    // Bottleneck link between routers (10Mbps, 50ms delay)
    PointToPointHelper bottleneckLink;
    bottleneckLink.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    bottleneckLink.SetChannelAttribute("Delay", StringValue("50ms"));
    bottleneckLink.SetQueue("ns3::DropTailQueue<Packet>",
                            "MaxSize", QueueSizeValue(QueueSize("100p")));
    NetDeviceContainer routerDevices = bottleneckLink.Install(routers.Get(0), routers.Get(1));
    
    // Assign IP addresses
    Ipv4AddressHelper ipv4;
    
    std::vector<Ipv4InterfaceContainer> senderInterfaces;
    for (uint32_t i = 0; i < 3; i++) {
        std::ostringstream subnet;
        subnet << "10.1." << (i + 1) << ".0";
        ipv4.SetBase(subnet.str().c_str(), "255.255.255.0");
        senderInterfaces.push_back(ipv4.Assign(senderDevices[i]));
    }
    
    std::vector<Ipv4InterfaceContainer> receiverInterfaces;
    for (uint32_t i = 0; i < 3; i++) {
        std::ostringstream subnet;
        subnet << "10.2." << (i + 1) << ".0";
        ipv4.SetBase(subnet.str().c_str(), "255.255.255.0");
        receiverInterfaces.push_back(ipv4.Assign(receiverDevices[i]));
    }
    
    ipv4.SetBase("10.3.0.0", "255.255.255.0");
    Ipv4InterfaceContainer routerInterfaces = ipv4.Assign(routerDevices);
    
    // Enable routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    
    // Create applications
    // Distribute nFlows across 3 sender-receiver pairs
    uint16_t basePort = 5000;
    uint32_t flowsPerPair = nFlows / 3;
    uint32_t extraFlows = nFlows % 3;
    
    uint32_t flowId = 0;
    
    for (uint32_t pairIdx = 0; pairIdx < 3; pairIdx++) {
        uint32_t flowsForThisPair = flowsPerPair;
        if (pairIdx < extraFlows) {
            flowsForThisPair++;
        }
        
        Ptr<Node> senderNode = senders.Get(pairIdx);
        Ptr<Node> receiverNode = receivers.Get(pairIdx);
        Ipv4Address receiverAddr = receiverInterfaces[pairIdx].GetAddress(0);
        
        for (uint32_t f = 0; f < flowsForThisPair; f++) {
            uint16_t port = basePort + flowId;
            
            // PacketSink on receiver
            PacketSinkHelper sinkHelper("ns3::TcpSocketFactory",
                InetSocketAddress(Ipv4Address::GetAny(), port));
            ApplicationContainer sinkApp = sinkHelper.Install(receiverNode);
            sinkApp.Start(Seconds(0.0));
            sinkApp.Stop(Seconds(simTime + 5.0));
            
            // RateControlledSender on sender
            Ptr<Socket> tcpSocket = Socket::CreateSocket(senderNode, 
                TcpSocketFactory::GetTypeId());
            
            Ptr<RateControlledSender> senderApp = CreateObject<RateControlledSender>();
            senderNode->AddApplication(senderApp);
            
            senderApp->Setup(tcpSocket,
                           InetSocketAddress(receiverAddr, port),
                           1448,           // packet size
                           packetRate,     // packets per second
                           flowId,
                           outputDir);
            
            senderApp->SetStartTime(Seconds(1.0 + f * 0.01)); // Staggered start
            senderApp->SetStopTime(Seconds(simTime));
            
            std::cout << "Flow " << flowId << ": Sender" << pairIdx 
                      << " -> Receiver" << pairIdx << " (port " << port << ")" << std::endl;
            
            flowId++;
        }
    }
    
    std::cout << "Total flows created: " << flowId << std::endl;
    
    // Install FlowMonitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    
    // Setup NetAnim
    std::string tcpShort = tcpType.substr(6); // Remove "ns3::Tcp" prefix
    std::string animFile = outputDir + "/anim-" + tcpShort + 
                          "-f" + std::to_string(nFlows) +
                          "-r" + std::to_string((int)packetRate) + ".xml";
    
    AnimationInterface anim(animFile);
    
    // Set node descriptions and colors
    for (uint32_t i = 0; i < 3; i++) {
        anim.UpdateNodeDescription(senders.Get(i), "S" + std::to_string(i));
        anim.UpdateNodeDescription(receivers.Get(i), "R" + std::to_string(i));
        anim.UpdateNodeColor(senders.Get(i), 0, 255, 0);     // Green
        anim.UpdateNodeColor(receivers.Get(i), 255, 0, 0);   // Red
    }
    anim.UpdateNodeDescription(routers.Get(0), "R1");
    anim.UpdateNodeDescription(routers.Get(1), "R2");
    anim.UpdateNodeColor(routers.Get(0), 0, 0, 255);         // Blue
    anim.UpdateNodeColor(routers.Get(1), 0, 0, 255);         // Blue
    
    anim.EnablePacketMetadata(true);
    
    // Run simulation
    std::cout << "\nRunning simulation..." << std::endl;
    Simulator::Stop(Seconds(simTime + 5.0));
    Simulator::Run();
    
    // Collect statistics
    std::cout << "Collecting statistics..." << std::endl;
    monitor->CheckForLostPackets();
    
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(
        flowmon.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    
    std::vector<double> throughputs;
    double totalThroughput = 0.0;
    double totalDelay = 0.0;
    uint64_t totalTxPackets = 0;
    uint64_t totalRxPackets = 0;
    uint64_t totalLostPackets = 0;
    uint64_t totalRxBytes = 0;
    uint32_t flowCount = 0;
    
    // Save detailed flow statistics
    std::string detailFile = outputDir + "/details-" + tcpShort +
                            "-f" + std::to_string(nFlows) +
                            "-r" + std::to_string((int)packetRate) + ".txt";
    std::ofstream detailOut(detailFile);
    
    detailOut << "=== Flow Statistics ===" << std::endl;
    detailOut << "TCP: " << tcpType << std::endl;
    detailOut << "Flows: " << nFlows << ", Rate: " << packetRate << " pkt/s" << std::endl;
    detailOut << "Simulation Time: " << simTime << " s" << std::endl << std::endl;
    detailOut << "FlowID\tSrc->Dst\tTxPkts\tRxPkts\tLost\tLoss%\t"
              << "Throughput(kbps)\tDelay(ms)\tGoodput(kbps)" << std::endl;
    detailOut << "------\t--------\t------\t------\t----\t-----\t"
              << "---------------\t---------\t-------------" << std::endl;
    
    for (auto it = stats.begin(); it != stats.end(); ++it) {
        FlowMonitor::FlowStats fs = it->second;
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(it->first);
        
        if (fs.rxPackets == 0) continue;
        
        double duration = fs.timeLastRxPacket.GetSeconds() - 
                         fs.timeFirstTxPacket.GetSeconds();
        if (duration <= 0) continue;
        
        // double throughput = fs.rxBytes * 8.0 / duration / 1000.0; // kbps
        double throughput = fs.txBytes * 8.0 / duration / 1000.0; // kbps
        double avgDelay = fs.delaySum.GetSeconds() / fs.rxPackets * 1000.0; // ms
        double lossRate = (fs.txPackets > 0) ? 
                         (fs.lostPackets * 100.0 / fs.txPackets) : 0.0;
        
        // Goodput = application layer throughput (excluding retransmissions)
        // double goodput = throughput * (1.0 - lossRate / 100.0);
        double goodput = fs.rxBytes * 8.0 / duration / 1000.0; // kbps
        
        detailOut << it->first << "\t"
                 << t.sourceAddress << "->" << t.destinationAddress << "\t"
                 << fs.txPackets << "\t"
                 << fs.rxPackets << "\t"
                 << fs.lostPackets << "\t"
                 << std::fixed << std::setprecision(2) << lossRate << "\t"
                 << std::fixed << std::setprecision(2) << throughput << "\t"
                 << std::fixed << std::setprecision(2) << avgDelay << "\t"
                 << std::fixed << std::setprecision(2) << goodput << std::endl;
        
        throughputs.push_back(throughput);
        totalThroughput += throughput;
        totalDelay += avgDelay;
        totalTxPackets += fs.txPackets;
        totalRxPackets += fs.rxPackets;
        totalLostPackets += fs.lostPackets;
        totalRxBytes += fs.rxBytes;
        flowCount++;
    }
    
    // Calculate aggregate metrics
    double avgThroughput = (flowCount > 0) ? totalThroughput / flowCount : 0.0;
    double avgDelay = (flowCount > 0) ? totalDelay / flowCount : 0.0;
    double totalLossRate = (totalTxPackets > 0) ? 
                          (totalLostPackets * 100.0 / totalTxPackets) : 0.0;
    double jainsIndex = CalculateJainsFairnessIndex(throughputs);
    double totalGoodput = totalRxBytes * 8.0 / simTime / 1000.0; // kbps
    
    detailOut << "\n=== Aggregate Statistics ===" << std::endl;
    detailOut << "Active Flows: " << flowCount << std::endl;
    detailOut << "Average Throughput: " << avgThroughput << " kbps" << std::endl;
    detailOut << "Average Delay: " << avgDelay << " ms" << std::endl;
    detailOut << "Total Loss Rate: " << totalLossRate << "%" << std::endl;
    detailOut << "Jain's Fairness Index: " << jainsIndex << std::endl;
    detailOut << "Total Goodput: " << totalGoodput << " kbps" << std::endl;
    detailOut << "Total Tx: " << totalTxPackets << ", Rx: " << totalRxPackets 
              << ", Lost: " << totalLostPackets << std::endl;
    
    detailOut.close();
    
    // Print summary to console
    std::cout << "\n=== Summary ===" << std::endl;
    std::cout << "Active Flows: " << flowCount << std::endl;
    std::cout << "Avg Throughput: " << avgThroughput << " kbps" << std::endl;
    std::cout << "Avg Delay: " << avgDelay << " ms" << std::endl;
    std::cout << "Loss Rate: " << totalLossRate << "%" << std::endl;
    std::cout << "Jain's Fairness: " << jainsIndex << std::endl;
    std::cout << "Total Goodput: " << totalGoodput << " kbps" << std::endl;
    
    // Append to CSV for comparison
    std::ofstream csvFile(outputDir + "/summary.csv", std::ios::app);
    csvFile << tcpShort << "," 
            << nFlows << "," 
            << packetRate << ","
            << avgThroughput << ","
            << avgDelay << ","
            << totalLossRate << ","
            << jainsIndex << ","
            << totalGoodput << std::endl;
    csvFile.close();
    
    // Close cwnd files
    for (auto& entry : g_cwndFiles) {
        if (entry.second && entry.second->is_open()) {
            entry.second->close();
            delete entry.second;
        }
    }
    g_cwndFiles.clear();
    
    Simulator::Destroy();
    std::cout << "Simulation completed!" << std::endl;
    std::cout << "Results saved to: " << outputDir << std::endl << std::endl;
}

int main(int argc, char *argv[])
{
    // Default parameters
    bool runAll = true;
    std::string tcpType = "ns3::TcpNewReno";
    uint32_t nFlows = 20;
    double packetRate = 300.0;
    double simTime = 60.0;
    std::string outputDir = "results";
    
    CommandLine cmd(__FILE__);
    cmd.AddValue("runAll", "Run all test scenarios", runAll);
    cmd.AddValue("tcpType", "TCP variant (TcpNewReno/TcpCubic/TcpVegas)", tcpType);
    cmd.AddValue("nFlows", "Number of flows", nFlows);
    cmd.AddValue("packetRate", "Packet rate (packets/second)", packetRate);
    cmd.AddValue("simTime", "Simulation time (seconds)", simTime);
    cmd.AddValue("outputDir", "Output directory", outputDir);
    cmd.Parse(argc, argv);
    
    // Create output directory
    system(("mkdir -p " + outputDir).c_str());
    
    // Create CSV header
    std::ofstream csvFile(outputDir + "/summary.csv");
    csvFile << "TCP,Flows,PacketRate,AvgThroughput(kbps),AvgDelay(ms),"
            << "LossRate(%),JainsFairness,TotalGoodput(kbps)" << std::endl;
    csvFile.close();
    
    if (runAll) {
        std::cout << "\n*******************************************" << std::endl;
        std::cout << "    Running Complete Test Matrix" << std::endl;
        std::cout << "*******************************************\n" << std::endl;
        
        std::vector<std::string> tcpVariants = {
            "ns3::TcpNewReno",  // Reno
            "ns3::TcpCubic",
            "ns3::TcpVegas"
        };
        
        std::vector<uint32_t> flowCounts = {5, 20, 50};
        std::vector<double> packetRates = {100.0, 300.0, 1000.0};
        
        for (const auto& tcp : tcpVariants) {
            for (uint32_t flows : flowCounts) {
                for (double rate : packetRates) {
                    RunSimulation(tcp, flows, rate, outputDir, simTime);
                }
            }
        }
        
        std::cout << "\n*******************************************" << std::endl;
        std::cout << "    All Simulations Completed!" << std::endl;
        std::cout << "    Results: " << outputDir << "/summary.csv" << std::endl;
        std::cout << "*******************************************" << std::endl;
        
    } else {
        // Run single simulation
        RunSimulation(tcpType, nFlows, packetRate, outputDir, simTime);
    }
    
    return 0;
}
