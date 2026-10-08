# 03. Roteiro de Bancada: Montagem Física, Higiene Elétrica e Gravação USB

> [!CAUTION]
> **ALERTA CRÍTICO DE SEGURANÇA E HIGIENE ELÉTRICA:**
> É **RIGOROSAMENTE OBRIGATÓRIA A REMOÇÃO FÍSICA DO JUMPER DE 5V** da placa do driver L298N antes da aplicação de energia!
> Como a tensão no pino de potência `VMS` é de **24,0 V** (muito acima do limite de 12V do regulador 78M05 embutido no L298N), manter esse jumper causa superaquecimento instantâneo, explosão térmica do regulador e curto-circuito catastrófico de 24V na linha lógica de 5V, destruindo irreversivelmente as placas ESP32!

---

## 1. Mapeamento Rigoroso de Pinos (ESP32-CAM)

Conecte as portas da ESP32-CAM respeitando a tabela abaixo:

| Função no UGV | Sinal Lógico | Pino ESP32-CAM | Conexão de Destino | Observações Técnicas |
|---|---|---|---|---|
| **Alimentação 5V** | 5,00 V Regulado | `5V` | Saída `OUT+` do LM2596 | Soldar capacitor de 470 µF em paralelo |
| **Referência** | GND Comum | `GND` | Barra de Terra Estrela | Referência comum de todo o robô |
| **I2C Clock** | SCL | `GPIO 22` | SCL do GY-906 | Resistor pull-up externo de 4,7 kΩ para 3.3V |
| **I2C Dados** | SDA | `GPIO 21` | SDA do GY-906 | Resistor pull-up externo de 4,7 kΩ para 3.3V |
| **Motor Esquerdo PWM** | ENA | `GPIO 14` | ENA do Driver L298N | Frequência de PWM: 1500 Hz |
| **Motor Esquerdo IN1** | IN1 | `GPIO 27` | IN1 do Driver L298N | Direção A |
| **Motor Esquerdo IN2** | IN2 | `GPIO 26` | IN2 do Driver L298N | Direção B |
| **Motor Direito PWM** | ENB | `GPIO 12` | ENB do Driver L298N | Frequência de PWM: 1500 Hz |
| **Motor Direito IN3** | IN3 | `GPIO 25` | IN3 do Driver L298N | Direção A |
| **Motor Direito IN4** | IN4 | `GPIO 33` | IN4 do Driver L298N | Direção B |
| **Iluminação Frontal** | PWM Dimming | `GPIO 4` | Gate do MOSFET D4184 | Controle de brilho dos faróis 12V |
| **Alerta Buzzer** | Sinal Lógico | `GPIO 13` | Base do BC547 (via 1kΩ) | Comuta o Buzzer Ativo 5V |
| **Alerta Visual** | Nível Lógico | `GPIO 15` | Ânodo do LED Vermelho | Alerta local de 50°C / 70°C |

---

## 2. Checklist Pré-Energização em 4 Fases

Siga rigorosamente esta ordem cronológica com um multímetro antes de conectar a bateria ao sistema:

### Fase 1: Verificações de Continuidade Elétrica e Anticurto (Desenergizado)
- [ ] **Teste de Anticurto:** Com o circuito desligado e o multímetro em teste de continuidade/resistência, meça entre as linhas **24V**, **5V**, **12V Direct** e **GND**. Confirmar impedância elevada (sem curto-circuito).
- [ ] **Polaridade do Pack 3S2P:** Checar visualmente a polaridade do conector da bateria, do porta-fusível com fusível de **15 A** e da chave unipolar KCD1-101.
- [ ] **Isolação da Ponte H L298N:** **Confirmar visualmente que o jumper de 5V da placa L298N foi extraído e descartado.**

### Fase 2: Calibração de Tensão sem Carga (Trimpots dos Conversores)
- [ ] **Calibração do Buck LM2596:** Desconectar os cabos de alimentação 5V da ESP32-CAM e do L298N. Ligar a chave geral e girar o trimpot do LM2596 até cravar exatamente **5,00 V** no multímetro.
- [ ] **Calibração do Boost 400W:** Desconectar o borne `VMS` do L298N. Ligar o sistema e girar o trimpot do Step-Up Boost até obter a leitura estável de exatamente **24,0 V**.
- [ ] Desligar a chave geral.

### Fase 3: Validação Lógica, Filtros e Gravação USB
- [ ] **Capacitor Anti-Brownout na ESP32-CAM:** Confirmar que o capacitor eletrolítico de **470 µF / 16V** está soldado diretamente nos pinos `5V` e `GND` da ESP32-CAM (absorve picos de transmissão RF do Wi-Fi).
- [ ] **Dissipador de Calor:** Confirmar a fixação do dissipador de alumínio de $9 \times 9\text{ mm}$ com adesivo térmico sobre a blindagem do SoC ESP32.
- [ ] **Filtros EMI dos Motores:** Confirmar a soldagem de 1x capacitor cerâmico disco de **100 nF / 50V (código 104)** diretamente nos bornes de cada motor JGB37-520.
- [ ] **Frequência de PWM no Firmware:** Confirmar que `include/config.h` está definido com `#define PWM_FREQ_HZ 1500` (1,5 kHz) antes de gravar.
- [ ] **Gravação via USB:** Gravar o firmware selecionando o ambiente correspondente no PlatformIO:
  ```bash
  pio run -e esp32cam_hardware --target upload
  ```

### Fase 4: Alinhamento Mecânico, Rolamentos e Tensão de Esteiras
- [ ] **Rolamentos 608-ZZ:** Girar manualmente as 4x polias livres de $\varnothing 50\text{ mm}$ e verificar rotação livre e sem folga dos 8 rolamentos 608-ZZ nos eixos mortos M8.
- [ ] **Tensionamento nos Rasgos Oblongos:** Ajustar a posição dos eixos dianteiros M8 dentro dos rasgos de $25 \times 8,2\text{ mm}$ para eliminar folgas das esteiras de pneu MTB sem sobrecarregar radialmente o eixo dos motores.
- [ ] **Porcas Autofrenantes DIN 985:** Confirmar que todas as porcas dos eixos mortos M8 e das junções M3 das esteiras são autoblocantes com anel de nylon.
