/*
 * tri_tech_demo.cc
 *
 * Novo código de teste para ns-3 unificando 3 cenários independentes:
 *  - THz (UN-Lab/TeraSim)
 *  - mmWave (NYU ns3-mmwave)
 *  - Satellite (SNS3)
 *
 * Você pode ligar/desligar cada cenário por flags e rodar todos em paralelo.
 * Cada cenário escreve um FlowMonitor XML em scratch-logs/<stack>/.
 *
 * Compilação: coloque em scratch/ e use seu wrapper:
 *   ./ns3 configure --enable-examples --enable-tests
 *   ./ns3 build
 * Execução (exemplos):
 *   ./ns3 run "tri_tech_demo --enableThz=1 --duration=10"
 *   ./ns3 run "tri_tech_demo --enableMmwave=1 --duration=10"
 *   ./ns3 run "tri_tech_demo --enableSat=1 --duration=10"
 *   ./ns3 run "tri_tech_demo --enableThz=1 --enableMmwave=1 --enableSat=1 --duration=10"
 *
 * Observação importante:
 *   - As APIs podem variar entre branches; ajuste includes/classes se necessário.
 *   - Este arquivo só ativa cada tecnologia de forma isolada (IP fim-a-fim em cada uma). 
 *     Depois podemos costurar um cenário único roteado atravessando as três.
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/applications-module.h"
#include "ns3/csma-module.h"

using namespace ns3;

// ---------- Macros de disponibilidade de módulos ----------
#ifdef NS3_MODULE_SATELLITE
#include "ns3/satellite-module.h"
#endif

#ifdef NS3_MODULE_THZ
// Ajuste se seu commit do THz usar nomes diferentes
#include "ns3/thz-module.h"
#endif

#ifdef NS3_MODULE_MMWAVE
#include "ns3/mmwave-module.h"
#endif

// ---------- Utilitários ----------
static void EnsureDir (const std::string &dir)
{
  std::string cmd = std::string("mkdir -p '") + dir + "'";
  if (std::system (cmd.c_str ()) != 0) {
    NS_LOG_UNCOND ("[warn] mkdir falhou: " << dir);
  }
}

struct StackOut {
  NodeContainer nodes;
  Ipv4InterfaceContainer ifaces;
};

static void SaveFlow (Ptr<FlowMonitor> fm, const std::string &path)
{
  fm->CheckForLostPackets ();
  fm->SerializeToXmlFile (path, true, true);
}

// ---------- SATELLITE: mini cenário UT<->GW com Echo ----------
#ifdef NS3_MODULE_SATELLITE
static StackOut BuildSat (double stop, const std::string &logDir)
{
  NS_LOG_UNCOND ("[sat] init");
  InternetStackHelper inet;

  Ptr<SatHelper> satHelper = CreateObject<SatHelper> ();
  satHelper->SetBaseScenario (SatHelper::SIMPLE);

  Ptr<SatSimulationHelper> simHelper = CreateObject<SatSimulationHelper> ();
  simHelper->SetSatHelper (satHelper);
  simHelper->CreateSatScenario (satHelper->GetBaseScenario ());

  NodeContainer uts = satHelper->GetUtNodes ();
  NodeContainer gws = satHelper->GetGwNodes ();

  inet.Install (uts);
  inet.Install (gws);

  // LAN fictícia para bind de apps (mantém exemplo compacto)
  CsmaHelper csma; csma.SetChannelAttribute ("DataRate", StringValue ("1Gbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (MicroSeconds (1)));
  NodeContainer edge (uts.Get (0), gws.Get (0));
  NetDeviceContainer devs = csma.Install (edge);
  Ipv4AddressHelper ip; ip.SetBase ("10.50.0.0", "255.255.255.0");
  Ipv4InterfaceContainer ifaces = ip.Assign (devs);

  uint16_t port = 9000;
  UdpEchoServerHelper srvH (port);
  auto srv = srvH.Install (gws.Get (0));
  srv.Start (Seconds (1)); srv.Stop (Seconds (stop - 0.1));

  UdpEchoClientHelper cliH (ifaces.GetAddress (1), port);
  cliH.SetAttribute ("MaxPackets", UintegerValue (1000000));
  cliH.SetAttribute ("Interval", TimeValue (MilliSeconds (10)));
  cliH.SetAttribute ("PacketSize", UintegerValue (400));
  auto cli = cliH.Install (uts.Get (0));
  cli.Start (Seconds (2)); cli.Stop (Seconds (stop - 0.1));

  EnsureDir (logDir);
  FlowMonitorHelper fmh; Ptr<FlowMonitor> fm = fmh.InstallAll ();
  Simulator::Schedule (Seconds (stop - 0.2), &SaveFlow, fm, logDir+"/sat.xml");

  StackOut o; o.nodes = NodeContainer (uts.Get (0), gws.Get (0)); o.ifaces = ifaces; return o;
}
#endif

// ---------- THz: link LoS 300 GHz com CBR UDP ----------
#ifdef NS3_MODULE_THZ
static StackOut BuildThz (double stop, const std::string &logDir)
{
  NS_LOG_UNCOND ("[thz] init");
  NodeContainer n; n.Create (2);
  MobilityHelper mob; mob.SetMobilityModel ("ns3::ConstantPositionMobilityModel"); mob.Install (n);
  n.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (0,0,0));
  n.Get (1)->GetObject<MobilityModel> ()->SetPosition (Vector (50,0,0));

  InternetStackHelper inet; inet.Install (n);

  // Canal/Dispositivo THz (ajuste atributos conforme seu commit)
  Ptr<ThzChannel> ch = CreateObject<ThzChannel> ();
  ch->SetAttribute ("Frequency", DoubleValue (300e9));
  ch->SetAttribute ("Bandwidth", DoubleValue (2e9));

  NetDeviceContainer devs;
  for (uint32_t i=0;i<2;++i) {
    Ptr<ThzNetDevice> d = CreateObject<ThzNetDevice> ();
    d->SetChannel (ch);
    n.Get(i)->AddDevice (d);
    devs.Add (d);
  }

  Ipv4AddressHelper ip; ip.SetBase ("10.60.0.0", "255.255.255.0");
  Ipv4InterfaceContainer ifaces = ip.Assign (devs);

  uint16_t port = 5000;
  OnOffHelper on ("ns3::UdpSocketFactory", InetSocketAddress (ifaces.GetAddress(1), port));
  on.SetAttribute ("DataRate", DataRateValue (DataRate ("500Mbps")));
  on.SetAttribute ("PacketSize", UintegerValue (1200));
  on.SetAttribute ("StartTime", TimeValue (Seconds (1)));
  on.SetAttribute ("StopTime", TimeValue (Seconds (stop - 0.1)));
  on.Install (n.Get (0));

  PacketSinkHelper sink ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
  auto sapps = sink.Install (n.Get (1)); sapps.Start (Seconds (0.5));

  EnsureDir (logDir);
  FlowMonitorHelper fmh; Ptr<FlowMonitor> fm = fmh.InstallAll ();
  Simulator::Schedule (Seconds (stop - 0.2), &SaveFlow, fm, logDir+"/thz.xml");

  StackOut o; o.nodes = n; o.ifaces = ifaces; return o;
}
#endif

// ---------- mmWave: gNB+UE + tráfego UDP simplificado ----------
#ifdef NS3_MODULE_MMWAVE
static StackOut BuildMmwave (double stop, const std::string &logDir)
{
  NS_LOG_UNCOND ("[mmwave] init");
  NodeContainer gnb, ue; gnb.Create (1); ue.Create (1);

  MobilityHelper mob; mob.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mob.Install (gnb); mob.Install (ue);
  gnb.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (0,0,0));
  ue.Get (0)->GetObject<MobilityModel> ()->SetPosition (Vector (80,0,0));

  InternetStackHelper inet; inet.Install (gnb); inet.Install (ue);

  Ptr<MmWaveHelper> mm = CreateObject<MmWaveHelper> ();
  mm->SetAttribute ("Numerology", UintegerValue (3));
  mm->SetAttribute ("ChannelModel", StringValue ("ns3::MmWave3gppChannel"));

  NetDeviceContainer gnbDevs = mm->InstallEnbDevice (gnb);
  NetDeviceContainer ueDevs  = mm->InstallUeDevice (ue);

  // Para simplificar IP app, conectamos UE<->gNB via CSMA stub (somente para teste de app)
  CsmaHelper csma; csma.SetChannelAttribute ("DataRate", StringValue ("1Gbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (MicroSeconds (1)));
  NodeContainer pair (ue.Get (0), gnb.Get (0));
  NetDeviceContainer cs = csma.Install (pair);
  Ipv4AddressHelper ip; ip.SetBase ("10.70.0.0", "255.255.255.0");
  Ipv4InterfaceContainer ifaces = ip.Assign (cs);

  uint16_t port = 6000;
  OnOffHelper on ("ns3::UdpSocketFactory", InetSocketAddress (ifaces.GetAddress (1), port));
  on.SetAttribute ("DataRate", DataRateValue (DataRate ("200Mbps")));
  on.SetAttribute ("PacketSize", UintegerValue (1200));
  on.SetAttribute ("StartTime", TimeValue (Seconds (1)));
  on.SetAttribute ("StopTime", TimeValue (Seconds (stop - 0.1)));
  on.Install (ue.Get (0));

  PacketSinkHelper sink ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
  auto sapps = sink.Install (gnb.Get (0)); sapps.Start (Seconds (0.5));

  EnsureDir (logDir);
  FlowMonitorHelper fmh; Ptr<FlowMonitor> fm = fmh.InstallAll ();
  Simulator::Schedule (Seconds (stop - 0.2), &SaveFlow, fm, logDir+"/mmwave.xml");

  StackOut o; NodeContainer both; both.Add (ue); both.Add (gnb); o.nodes = both; o.ifaces = ifaces; return o;
}
#endif

// ---------- main ----------
int main (int argc, char *argv[])
{
  bool eThz=false, eMm=false, eSat=false;
  double stop=10.0;
  std::string logRoot = "scratch-logs";

  CommandLine cmd;
  cmd.AddValue ("enableThz", "Liga o cenário THz", eThz);
  cmd.AddValue ("enableMmwave", "Liga o cenário mmWave", eMm);
  cmd.AddValue ("enableSat", "Liga o cenário Satélite", eSat);
  cmd.AddValue ("duration", "Tempo de simulação (s)", stop);
  cmd.AddValue ("logRoot", "Diretório base de logs", logRoot);
  cmd.Parse (argc, argv);

  LogComponentEnableAll (LOG_PREFIX_TIME);

  uint32_t on=0;
#ifdef NS3_MODULE_THZ
  if (eThz) { BuildThz (stop, logRoot+"/thz"); ++on; }
#else
  if (eThz) NS_LOG_UNCOND ("[thz] módulo ausente: NS3_MODULE_THZ não definido");
#endif

#ifdef NS3_MODULE_MMWAVE
  if (eMm) { BuildMmwave (stop, logRoot+"/mmwave"); ++on; }
#else
  if (eMm) NS_LOG_UNCOND ("[mmwave] módulo ausente: NS3_MODULE_MMWAVE não definido");
#endif

#ifdef NS3_MODULE_SATELLITE
  if (eSat) { BuildSat (stop, logRoot+"/sat"); ++on; }
#else
  if (eSat) NS_LOG_UNCOND ("[sat] módulo ausente: NS3_MODULE_SATELLITE não definido");
#endif

  if (on==0) {
    NS_LOG_UNCOND ("No stacks enabled. Use --enableThz=1/--enableMmwave=1/--enableSat=1.");
    return 0;
  }

  Simulator::Stop (Seconds (stop));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_UNCOND ("OK. XMLs em " << logRoot);
  return 0;
}

