# 6G NTN – THz, mmWave & Satellite Simulation (NS-3.42)

Este repositório contém um ambiente de simulação completo para estudos em **6G**, com foco em **Non-Terrestrial Networks (NTN)**, integrando:

- 📡 **Módulo Satellite (NTN – 3GPP TR 38.811)**
- 📶 **Módulo mmWave (NYU/UniPd 3GPP 38.901)**
- ⚡ **Módulo THz (6G Terahertz experimental)**
- 🛰️ **Simulação de links diretos Satélite ↔ Estação Base**
- 📈 **SNR, perda de caminho, Doppler, beamforming e mobilidade**

Compatível com a **versão 3.42 do NS-3**.

---

## 🚀 Objetivo do Projeto

Este projeto busca fornecer uma base sólida para pesquisas em **6G NTN**, permitindo a integração de:

1. **mmWave 5G/6G**
2. **THz 6G**
3. **NTN via satélite em LEO**

Permitindo simular:
- Links diretos (D2D / D2C / UE ↔ Sat)
- Cenários multi-satélite
- SNR, throughput, delay e BER
- Modelos 3GPP completos (urban, rural, UMi, UMa, RMa)
- RTS, UPS, mobilidade orbital via TLE

---

## 🧩 Estrutura do Repositório

ns3_6G_NTN/
├── scratch/
│ ├── ntn-direct-link.cc # exemplo de link direto Sat ↔ BS
│ ├── thz-satellite-example.cc # teste THz experimental
│ └── mmwave-ntn-test.cc # exemplo mmWave + NTN
├── src/
│ ├── satellite/
│ ├── mmwave/
│ ├── thz/
│ └── ...
├── .github/workflows/
│ └── main.yml # (opcional) integração contínua
└── CMakeLists.txt 


---

## 🛠️ Pré-requisitos

- Ubuntu 20.04 / 22.04 / 24.04  
- Python 3  
- CMake (versões recentes)  
- GCC 9+ ou Clang  
- Git  
- Dependências NS-3 (libgtk-3-dev, libxml2, etc.)

## Instalação rápida:

bash
sudo apt update
sudo apt install -y build-essential cmake git python3 python3-pip \
                    libgtk-3-dev libxml2-dev libsqlite3-dev
cd ns3_6G_NTN
./ns3 configure --enable-examples --enable-tests
./ns3 build
mkdir build
cd build
cmake ..
make -j$(nproc)

# Como Executar as Simulações

1. Simulação NTN (satélite LEO)
   ./ns3 run "ntn-direct-link --distance=600000 --enable-logs=1"

 
2.  . Simulação mmWave + NTN
   ./ns3 run "mmwave-ntn-test --snr=10 --mobility=urban
    
3. Simulação THz
   ./ns3 run "thz-satellite-example --frequency=300GHz"
📡 Módulo Satellite (NTN – TR 38.811)

Inclui:

Canais Sat ↔ UE baseados no 3GPP

Modelos LEO orbitais

Tracking, Doppler shift, SNR

Beamforming dinâmico

Geração de logs e valores de enlace

Este módulo permite simular links diretos em banda Ka, Ku, mmWave ou THz.

📶 Módulo mmWave

Baseado no modelo 3GPP 38.901/38.900, incluindo:

UMi / UMa / Indoor

Blockage dinâmico

Beamforming com codebooks

Perda de caminho multi-cluster

Modelos de mobilidade 6G

Suporte para portadoras 28 GHz, 39 GHz e acima de 100 GHz.

⚡ Módulo THz (Terahertz)

Extensão experimental, baseado em literatura 6G:

Frequências de 0.1–1 THz

Absorção molecular

Perda de caminho baseada em distâncias curtas e médias

Modelos para transmissão Sat ↔ BS e Sat ↔ UE

Ideal para pesquisas avançadas do 6G.

📊 Métricas Suportadas

Throughput

Delay

Jitter

Perda de pacotes

SNR / SINR

Desvanecimento

Doppler

Potência recebida

Trajetória orbital

🧭 Roadmap

 Adicionar exemplos multi-satélite

 Simulações com Starlink/OneWeb usando TLE

 THz com beamforming adaptativo

 Implementar HARQ/ARQ

 Fusão mmWave + THz (handover dual-layer)

 Grafana + Prometheus para visualização

 Resultados para o relatório do mestrado

📝 Licença

Distribuído sob licença GPLv2, mesma licença do NS-3.

👩‍💻 Autora

Ariel Luane Bentes
Mestranda em Engenharia Elétrica – NTN/6G
Pesquisa: Simulação Satélite + mmWave + THz
GitHub: https://github.com/arielluane
