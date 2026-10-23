# 02. Guia de Simulação no Wokwi Simulator: Racional de Modelagem e Componentes

Este documento estabelece o **Racional de Engenharia da Simulação Virtual**: por que determinados componentes do Robô Inspetor (UGV V1.0) estão presentes no simulador Wokwi, por que outros foram intencionalmente abstraídos e qual a exata utilidade de cada um no ecossistema de validação pré-bancada.

---

## 1. Filosofia de Simulação: O que Simular vs. O que Abstrair

Em projetos de sistemas embarcados e robótica crítica, **uma simulação digital em nível de instrução (Cycle-Accurate / Hardware-in-the-Loop simulado) tem como objetivo primordial validar software, máquinas de estado, integridade de barramentos lógicos e regras de segurança**.

Simular elementos analógicos passivos ou eletroquímicos em um emulador lógico adiciona ruído computacional sem nenhum benefício para a lógica do firmware. A divisão rigorosa dos componentes é apresentada a seguir.

---

## 2. Matriz Completa de Componentes: Simulados vs. Abstraídos

A tabela abaixo correlaciona todos os itens do projeto real, justificando com rigor técnico a presença ou ausência de cada um no simulador:

| Componente Real (BOM) | Presente no Wokwi? | Categoria | Racional Técnico e Utilidade no Projeto |
|---|:---:|:---:|---|
| **ESP32 DevKit V1 / ESP32-CAM** | **SIM** *(com nuances)* | Microcontrolador | **Núcleo de Processamento e Co-processamento:** Executa o firmware C++ real compilado em arquitetura Xtensa. Simula os timers de PWM (LEDC), barramento I2C, conversor ADC (`GPIO 34`), UART Serial e GPIOs. *(Nota: O pipeline de vídeo DVP/DMA da OV2640 na ESP32-CAM é emulado via Mock JPEG, pois nenhum emulador web executa o sensor CMOS de alta velocidade com fidelidade física).* |
| **Sensor MLX90614 (GY-906)** | **SIM** | Sensor I2C (Custom Chip) | **Termografia Preditiva:** Emulado via WebAssembly (`gy906.wasm`). Responde ao protocolo I2C/SMBus no endereço `0x5A`, transmitindo os registradores `0x06` (Ambiente) e `0x07` (Objeto). Permite injetar variações térmicas via sliders e validar os gatilhos de 50°C e 70°C. |
| **Divisor de Tensão Bateria ($10\text{k}\Omega / 3,3\text{k}\Omega$)** | **SIM** *(via Potenciômetro)* | Sensoriamento Analógico (ADC) | **Monitoramento de Carga 3S2P:** Emulado pelo potenciômetro conectado à `GPIO 34` (`ADC1_CH6`). Converte a faixa da bateria (9,9V–12,6V) para 0–3,12V, validando o alerta de bateria fraca (<10,5V) e o corte crítico (<9,9V). |
| **Driver Ponte H L298N (Sinais)** | **SIM** *(via LEDs de estado)* | Potência Lógica | **Tração das Esteiras:** Emulado pelos sinais de direção (`IN1..IN4`) e PWM (`ENA`/`ENB` a 1500 Hz). Os 4 LEDs (`led_left_fwd`, `led_left_rev`, `led_right_fwd`, `led_right_rev`) comprovam visualmente aceleração em rampa, sentido bidirecional e corte imediato de torque. |
| **Faróis LED + MOSFET D4184** | **SIM** *(via LED Branco)* | Iluminação / Potência | **Iluminação Frontal em Espaços Confinados:** Emulado por LED branco de alto brilho conectado ao canal PWM da `GPIO 4`. Valida o controle de intensidade luminosa (*dimming*) pelo dashboard web e comandos seriais. |
| **LED Difuso Vermelho 5mm** | **SIM** | Sinalização Visual | **Alerta Local de Temperatura e Bateria:** Conectado à `GPIO 15`. Acende quando o ativo atinge $\ge 50^\circ\text{C}$ ou a bateria cai abaixo de $10,5\text{V}$. |
| **Buzzer Ativo 5V (BC547)** | **SIM** | Sinalização Sonora | **Alarme Local Acústico:** Emulado pelo módulo `wokwi-buzzer` na `GPIO 13`. Comprova o padrão de bipe intermitente no alerta preventivo (50°C / <10,5V) e alarme contínuo na parada crítica (70°C / <9,9V). |
| **Cooler Fan 80mm (`DF0922505SEL` - 5V)** | **NÃO** | Exaustão Térmica 5V DC | **Dispensável no Simulador:** O cooler real opera em **5,00 V DC (0,09 A / 0,45 W)** ligado diretamente na saída regulada de 5,00V do Step-Down LM2596 (sem controle por GPIO), girando continuamente enquanto o robô está energizado. |
| **Pack Bateria 3S2P Li-Ion (4400mAh)** | **NÃO** | Eletroquímica / Potência | **Dispensável no Simulador:** Fonte eletroquímica pura. O simulador assume trilhos de alimentação ideais (`VCC`/`3V3`), enquanto a curva de descarga é injetada pelo potenciômetro na `GPIO 34`. |
| **Placa BMS 3S (10A)** | **NÃO** | Proteção Passiva de Bateria | **Dispensável no Simulador:** Atua exclusivamente no corte físico de sobrecorrente/subtensão das células. |
| **Chave Geral KCD1-101 e Fusível 15A** | **NÃO** | Chaveamento Eletromecânico | **Dispensável no Simulador:** Elementos puramente mecânicos/passivos de seccionamento e proteção contra curto-circuito. |
| **Voltímetro Digital LED DC** | **NÃO** | Monitoramento Visual Passivo | **Dispensável no Simulador:** Exibe a tensão no painel superior físico; no simulador e dashboard, a leitura já é feita via ADC na `GPIO 34`. |
| **Step-Down Buck LM2596 (5,00V)** | **NÃO** | Conversão Analógica DC-DC | **Dispensável no Simulador:** Regulador indutivo analógico que alimenta a linha de 5,00V (ESP32-CAM, ESP32 DevKit V1, Cooler 80mm 5V e lógica L298N). |
| **Step-Up Boost 400W (24,0V)** | **NÃO** | Conversão Analógica DC-DC | **Dispensável no Simulador:** Eleva 12V para 24,0V para os enrolamentos dos motores JGB37-520 no pino `VMS` da L298N. |
| **Capacitores Anti-Brownout / EMI e Vedação** | **NÃO** | Filtragem e Proteção Física | **Dispensáveis no Simulador:** O ambiente virtual não sofre quedas de tensão por RF, ruído de escovas (EMI) ou infiltração de água/poeira (IP65). |

---

## 3. O Que É Comprovado com Exatidão Matemática na Simulação

1. **Protocolo I2C / SMBus:** Handshake mestre-escravo no endereço `0x5A`, *Repeated Start* e leitura de 16 bits dos registradores `0x06` e `0x07`.
2. **Máquina de Estados Térmica e de Bateria:**
   - Alerta preventivo aos **50 °C** ou bateria **< 10,5 V** (Buzzer intermitente + LED vermelho).
   - Parada crítica de emergência aos **70 °C** ou bateria **< 9,9 V** (corte instantâneo de PWM dos motores e alarme contínuo).
3. **Leitura Analógica ADC (Divisor $10\text{k}\Omega / 3,3\text{k}\Omega$):** Conversão de 12 bits na `GPIO 34` com cálculo em tempo real da tensão real (V) e porcentagem de carga (%).
4. **Mecanismo Failsafe (Watchdog):** Desligamento autônomo dos motores após 500 ms sem recepção de pacotes de controle.
5. **Frequência e Rampas de PWM (LEDC):** Geração estável de 1500 Hz e escalonamento suave a cada 20 ms via *Slew Rate Limiter*.

---

## 4. Como Executar a Simulação Interativa no VS Code

1. Selecione o ambiente **`env:wokwi`** no PlatformIO e compile clicando em **Build (✓)**.
2. Abra o arquivo [`diagram.json`](file:///home/tacio/dev/academic/robo-inspetor/diagram.json) e clique no botão verde de **Play**.
3. **Comandos Interativos via Wokwi Terminal:**
   - Digite **`w`**: Avanço das esteiras (PWM 180 com rampa suave).
   - Digite **`s`**: Recuo das esteiras (Ré).
   - Digite **`a`** / **`d`**: Giro no próprio eixo (*Zero Turn*).
   - Digite **` ` (espaço)**: Parada total imediata.
   - Digite **`l`**: Liga / Desliga os Faróis Olho de Águia (MOSFET na GPIO 4).
4. **Injeção Térmica e de Bateria:**
   - Clique no sensor `gy906` e mova os sliders para testar os gatilhos de 50°C e 70°C.
   - Gire o potenciômetro `Divisor Bateria 3S (GPIO 34)` para simular a queda de tensão da bateria abaixo de 10,5V e 9,9V.
