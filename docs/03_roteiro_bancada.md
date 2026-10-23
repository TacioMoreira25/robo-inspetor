# 03. Roteiro de Bancada: Montagem Física, Higiene Elétrica e Gravação USB

> [!CAUTION]
> **DOIS ALERTAS CRÍTICOS DE SEGURANÇA ELÉTRICA ANTES DE ENERGIZAR:**
> 1. **REMOÇÃO OBRIGATÓRIA DO JUMPER DE 5V DA PONTE H L298N:** Como a tensão no pino `VMS` é de **24,0 V** (acima do limite de 12V do regulador interno 78M05 da placa L298N), manter o jumper causará a queima imediata do regulador e injetará 24V na linha de 5V, destruindo as placas ESP32!
> 2. **COOLER FAN 80mm (`DF0922505SEL`) NA LINHA DE 5,00V:** O cooler real especificado opera em **5,00 V DC (0,09 A / 0,45 W)**. Ele deve ser ligado **exclusivamente na saída de 5,00V regulados do Step-Down LM2596** (em paralelo com a ESP32-CAM e a ESP32 DevKit V1). **Jamais conecte o cooler aos 12V da bateria**, sob pena de queima instantânea do motor brushless.

---

## 1. Mapeamento Rigoroso de Pinos (ESP32-CAM e ESP32 DevKit V1)

| Função no UGV | Sinal Lógico | Pino ESP32 | Conexão de Destino | Observações Técnicas |
|---|---|---|---|---|
| **Alimentação 5V** | 5,00 V Regulado | `5V` / `VIN` | Saída `OUT+` do LM2596 | Alimenta ESP32-CAM, DevKit V1, Cooler 80mm (5V) e lógica L298N |
| **Referência** | GND Comum | `GND` | Barra de Terra Estrela | Referência comum de todo o robô |
| **I2C Clock** | SCL | `GPIO 22` | SCL do GY-906 | Resistor pull-up externo de 4,7 kΩ para 3,3V na Mini Protoboard |
| **I2C Dados** | SDA | `GPIO 21` | SDA do GY-906 | Resistor pull-up externo de 4,7 kΩ para 3,3V na Mini Protoboard |
| **Leitura Bateria** | ADC Analógico | `GPIO 34` *(DevKit V1)* | Ponto central Divisor ($10\text{k}\Omega / 3,3\text{k}\Omega$) | Converte 9,9V–12,6V da bateria para 2,45V–3,12V no pino ADC |
| **Motor Esquerdo PWM** | ENA | `GPIO 14` | ENA do Driver L298N | Frequência de PWM: 1500 Hz |
| **Motor Esquerdo IN1** | IN1 | `GPIO 27` | IN1 do Driver L298N | Direção A |
| **Motor Esquerdo IN2** | IN2 | `GPIO 26` | IN2 do Driver L298N | Direção B |
| **Motor Direito PWM** | ENB | `GPIO 12` | ENB do Driver L298N | Frequência de PWM: 1500 Hz |
| **Motor Direito IN3** | IN3 | `GPIO 25` | IN3 do Driver L298N | Direção A |
| **Motor Direito IN4** | IN4 | `GPIO 33` | IN4 do Driver L298N | Direção B |
| **Iluminação Frontal** | PWM Dimming | `GPIO 4` | Gate do MOSFET D4184 | Controle de brilho dos faróis 12V |
| **Alerta Buzzer** | Sinal Lógico | `GPIO 13` | Base do BC547 (via 1kΩ) | Comuta o Buzzer Ativo 5V |
| **Alerta Visual** | Nível Lógico | `GPIO 15` | Ânodo do LED Vermelho | Alerta local de 50°C / 70°C e Bateria <10,5V |

---

## 2. Lista Completa dos 8 Componentes de Proteção, Motivos e Localização Exata

| # | Item / Componente | Motivo da Necessidade no Robô Real | Onde Vai Ficar no Robô |
| :--- | :--- | :--- | :--- |
| **1** | **Capacitor Eletrolítico Anti-Brownout ($470\,\mu\text{F}$ / 16V ou 25V)** | Absorve os picos rápidos de corrente (350–500 mA) gerados na inicialização do Wi-Fi e streaming de vídeo da ESP32-CAM, evitando quedas de tensão e resets contínuos. | Soldado **entre os pinos 5V e GND**, posicionado o mais próximo possível dos pinos da **ESP32-CAM** na bandeja interna. |
| **2** | **Capacitores Cerâmicos de Filtro EMI ($100\text{ nF}$ - Código 104, 2x)** | Elimina o ruído eletromagnético de alta frequência e faiscamento gerado pelas escovas dos motores DC JGB37-520 de 24V, impedindo o congelamento do vídeo e travamentos do barramento I2C. | Soldados **diretamente nos dois terminais metálicos de cada motor DC** (na parede traseira interna do chassi). |
| **3** | **Resistores de Pull-Up do Barramento I2C ($2\times 4,7\text{ k}\Omega$)** | Estabilizam os sinais digitais de dados (SDA - GPIO 21) e clock (SCL - GPIO 22) em nível alto (3,3V), garantindo a comunicação I2C/SMBus sem travamentos com o sensor térmico MLX90614. | Montados na **Mini Protoboard de 170 pontos** (na bandeja interna de eletrônica), conectando a linha SDA a 3,3V e a linha SCL a 3,3V da ESP32 DevKit V1. |
| **4** | **Porta-Fusível Lâmina + Fusível Lâmina (15A)** | Proteção geral contra sobrecorrente e curtos-circuitos causados por travamento mecânico das esteiras ou falhas na linha de potência de 24V, protegendo a bateria 3S2P e a placa BMS. | Intercalado no cabo positivo (+12V) **logo após o conector XT60 da bateria e antes da Chave Gangorra KCD1-101**. |
| **5** | **Divisor de Tensão Analógico ($10\text{ k}\Omega$ e $3,3\text{ k}\Omega$)** | Reduz a escala de tensão da bateria 3S (11,1V a 12,6V) para um intervalo seguro de 0–3,12V, permitindo que a ESP32 DevKit V1 meça a bateria via pino ADC (`GPIO 34`) e informe a porcentagem de carga no Dashboard. | Montado na **Mini Protoboard de 170 pontos**, ligado entre a linha de +12V da bateria, a porta ADC `GPIO 34` da ESP32 DevKit V1 e o GND comum. |
| **6** | **Fita / Gaxeta de Silicone Perimetral** | Garante estanqueidade e grau de proteção IP65 no compartimento interno seco contra entrada de água, poeira e detritos em ambientes confinados (NR-33 / NR-10). | Alojada no **canal perimetral de $3,0\text{ mm} \times 2,0\text{ mm}$** usinado na borda de encaixe da tampa superior de PETG. |
| **7** | **Remoção do Jumper de 5V da Ponte H L298N (Ação Física)** | Impede a queima imediata do regulador linear interno da placa L298N ao receber 24V no pino de potência $V_{MS}$. | **Removido** da placa L298N (o pino 5V lógico da L298N passa a ser alimentado diretamente pelo Step-Down LM2596). |
| **8** | **Anéis de Vedação O-Ring / Passa-Cabos de Borracha** | Vedar as aberturas do chassi ao redor da lente do sensor MLX90614, faróis LED Olho de Águia e haste da antena Wi-Fi. | Instalados nas **furações de passagem na proa frontal do chassi** e na **tampa superior**. |

---

## 3. Checklist Pré-Energização em 4 Fases

Siga rigorosamente esta ordem cronológica com um multímetro antes de conectar a bateria ao sistema:

### Fase 1: Verificações de Continuidade Elétrica e Anticurto (Desenergizado)
- [ ] **Teste de Anticurto:** Com o circuito desligado e o multímetro em teste de continuidade/resistência, meça entre as linhas **24V**, **5V**, **12V Direct** e **GND**. Confirmar impedância elevada (sem curto-circuito).
- [ ] **Polaridade do Pack 3S2P e Fusível 15A:** Checar visualmente a polaridade do conector XT60 da bateria, do porta-fusível lâmina com fusível de **15 A** e da chave unipolar KCD1-101.
- [ ] **Isolação da Ponte H L298N:** **Confirmar visualmente que o jumper de 5V da placa L298N foi extraído e descartado.**
- [ ] **Verificação do Cooler 80mm (`DF0922505SEL`):** Confirmar que os fios do cooler estão conectados na barra de **5,00V da saída do LM2596** (e **não** na linha de 12V da bateria).

### Fase 2: Calibração de Tensão sem Carga (Trimpots dos Conversores)
- [ ] **Calibração do Buck LM2596:** Desconectar os cabos de alimentação 5V da ESP32-CAM, ESP32 DevKit V1, Cooler 80mm e L298N. Ligar a chave geral e girar o trimpot do LM2596 até cravar exatamente **5,00 V** no multímetro.
- [ ] **Calibração do Boost 400W:** Desconectar o borne `VMS` do L298N. Ligar o sistema e girar o trimpot do Step-Up Boost até obter a leitura estável de exatamente **24,0 V**.
- [ ] **Aferição do Divisor de Tensão ($10\text{k}\Omega / 3,3\text{k}\Omega$):** Antes de conectar na `GPIO 34` da ESP32 DevKit V1, meça a tensão no nó central do divisor com a bateria conectada e confirme que a tensão está entre **2,70 V e 3,12 V** (nunca superior a 3,30 V).
- [ ] Desligar a chave geral.

### Fase 3: Validação Lógica, Filtros e Gravação USB
- [ ] **Capacitor Anti-Brownout na ESP32-CAM:** Confirmar que o capacitor eletrolítico de **470 µF / 16V (ou 25V)** está soldado diretamente nos pinos `5V` e `GND` da ESP32-CAM.
- [ ] **Dissipador de Calor:** Confirmar a fixação do dissipador de alumínio de $9 \times 9\text{ mm}$ com adesivo térmico sobre a blindagem do SoC ESP32.
- [ ] **Filtros EMI dos Motores:** Confirmar a soldagem de 1x capacitor cerâmico disco de **100 nF / 50V (código 104)** diretamente nos bornes de cada motor JGB37-520.
- [ ] **Pull-Ups I2C na Mini Protoboard:** Confirmar a instalação dos 2x resistores de **4,7 kΩ** entre SDA/3,3V e SCL/3,3V.
- [ ] **Frequência de PWM no Firmware:** Confirmar que `include/config.h` está definido com `#define PWM_FREQ_HZ 1500` (1,5 kHz) antes de gravar.
- [ ] **Gravação via USB:** Gravar o firmware selecionando o ambiente correspondente no PlatformIO:
  ```bash
  pio run -e esp32_hardware --target upload
  # ou para a ESP32-CAM:
  pio run -e esp32cam_hardware --target upload
  ```

### Fase 4: Alinhamento Mecânico, Vedação IP65 e Tensão de Esteiras
- [ ] **Vedação Perimetral e O-Rings:** Verificar o assentamento da gaxeta de silicone no canal de $3,0 \times 2,0\text{ mm}$ da tampa superior e os anéis O-Ring na lente do sensor MLX90614, faróis e antena SMA.
- [ ] **Rolamentos 608-ZZ:** Girar manualmente as 4x polias livres de $\varnothing 50\text{ mm}$ e verificar rotação livre e sem folga dos 8 rolamentos 608-ZZ nos eixos mortos M8.
- [ ] **Tensionamento nos Rasgos Oblongos:** Ajustar a posição dos eixos dianteiros M8 dentro dos rasgos de $25 \times 8,2\text{ mm}$ para eliminar folgas das esteiras de pneu MTB sem sobrecarregar radialmente o eixo dos motores.
- [ ] **Porcas Autofrenantes DIN 985:** Confirmar que todas as porcas dos eixos mortos M8 e das junções M3 das esteiras são autoblocantes com anel de nylon.
