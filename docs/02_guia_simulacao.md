# 02. Guia de Simulação no Wokwi Simulator: Racional de Modelagem e Componentes

Este documento estabelece o **Racional de Engenharia da Simulação Virtual**: por que determinados componentes do Robô Inspetor (UGV) estão presentes no simulador Wokwi, por que outros foram intencionalmente abstraídos e qual a exata utilidade de cada um no ecossistema de validação pré-bancada.

---

## 1. Filosofia de Simulação: O que Simular vs. O que Abstrair

Em projetos de sistemas embarcados e robótica crítica, **uma simulação digital em nível de instrução (Cycle-Accurate / Hardware-in-the-Loop simulado) tem como objetivo primordial validar software, máquinas de estado, integridade de barramentos lógicos e regras de segurança**.

Simular elementos analógicos passivos ou eletroquímicos em um emulador lógico adiciona ruído computacional sem nenhum benefício para a lógica do firmware. A divisão rigorosa dos componentes é apresentada a seguir.

---

## 2. Matriz Completa de Componentes: Simulados vs. Abstraídos

A tabela abaixo correlaciona todos os 31 itens do projeto real, justificando com rigor técnico a presença ou ausência de cada um no simulador:

| Componente Real (BOM) | Presente no Wokwi? | Categoria | Racional Técnico e Utilidade no Projeto |
|---|:---:|:---:|---|
| **ESP32 DevKit / ESP32-CAM** | **SIM** *(com nuances)* | Microcontrolador | **Núcleo de Processamento Central:** Executa o firmware C++ real compilado em arquitetura Xtensa. Simula os timers de PWM (LEDC), barramento I2C, UART Serial e GPIOs. *(Nota de engenharia: O pipeline de vídeo DVP/DMA da OV2640 na ESP32-CAM é emulado via Mock JPEG, pois nenhum emulador web executa o sensor CMOS de alta velocidade com fidelidade física).* |
| **Sensor MLX90614 (GY-906)** | **SIM** | Sensor I2C (Custom Chip) | **Termografia Preditiva:** Emulado via WebAssembly (`gy906.wasm`). Responde ao protocolo I2C/SMBus no endereço `0x5A`, transmitindo os registradores `0x06` (Ambiente) e `0x07` (Objeto). Permite injetar variações térmicas via sliders e validar os gatilhos de 50°C e 70°C. |
| **Driver Ponte H L298N (Sinais)** | **SIM** *(via LEDs de estado)* | Potência Lógica | **Tração das Esteiras:** Emulado pelos sinais de direção (`IN1..IN4`) e PWM (`ENA`/`ENB` a 1500 Hz). Os LEDs verdes (`led_left_fwd` e `led_right_fwd`) comprovam visualmente aceleração em rampa, sentido de rotação e corte imediato de torque. |
| **Faróis LED + MOSFET D4184** | **SIM** *(via LED Branco)* | Iluminação / Potência | **Iluminação Frontal em Espaços Confinados:** Emulado por LED branco de alto brilho conectado ao canal PWM da `GPIO 4`. Valida o controle de intensidade luminosa (*dimming*) pelo dashboard web e comandos seriais. |
| **LED Difuso Vermelho 5mm** | **SIM** | Sinalização Visual | **Alerta Local de 50°C / 70°C:** Conectado à `GPIO 15`. Acende quando o ativo inspecionado entra em temperatura de risco. |
| **Buzzer Ativo 5V (BC547)** | **SIM** | Sinalização Sonora | **Alarme Local Acústico:** Emulado pelo módulo `wokwi-buzzer` na `GPIO 13`. Comprova o padrão de bipe intermitente no alerta preventivo (50°C) e alarme contínuo na parada crítica (70°C). |
| **Pack Bateria 3S2P Li-Ion (4400mAh)** | **NÃO** | Eletroquímica / Potência | **Dispensável no Simulador:** É uma fonte de energia eletroquímica pura. O simulador assume barramentos de alimentação estáveis ideais (`VCC`/`3V3`). O comportamento de descarga e capacidade é validado na bancada com carga real. |
| **Placa BMS 3S (10A)** | **NÃO** | Proteção Passiva de Bateria | **Dispensável no Simulador:** Atua exclusivamente no corte físico de corrente/tensão da bateria. Não interage com sinais lógicos ou barramentos do microcontrolador. |
| **Chave Geral KCD1-101 e Fusível 15A** | **NÃO** | Chaveamento Eletromecânico | **Dispensável no Simulador:** Elementos puramente mecânicos/passivos de corte de energia. Iniciar ou pausar a simulação no Wokwi cumpre exatamente o papel de abrir ou fechar a chave. |
| **Voltímetro Digital LED DC** | **NÃO** | Monitoramento Visual Passivo | **Dispensável no Simulador:** Exibe a tensão da bateria no painel superior físico do robô. No simulador, a telemetria já transmite a tensão e status de forma digital no monitor JSON. |
| **Step-Down Buck LM2596 (5,00V)** | **NÃO** | Conversão Analógica DC-DC | **Dispensável no Simulador:** O simulador não resolve equações de chaveamento indutivo analógico; ele já provê os trilhos de 5V e 3.3V perfeitamente regulados para a lógica da ESP32. |
| **Step-Up Boost 400W (24,0V)** | **NÃO** | Conversão Analógica DC-DC | **Dispensável no Simulador:** A elevação de 12V para 24V alimenta os enrolamentos dos motores físicos. Os sinais lógicos da ponte H operam em 3.3V/5V, sendo totalmente desacoplados da tensão do motor. |
| **Cooler Fan Rise Mode 80mm** | **NÃO** | Atuador Eletromecânico 12V | **Dispensável no Simulador:** Conectado diretamente no barramento de 12V da bateria (sem controle por GPIO do microcontrolador), operando em rotação contínua quando a chave geral é ligada. |
| **Capacitores Anti-Ruído e Filtros EMI** | **NÃO** | Filtragem Passiva | **Dispensável no Simulador:** O ambiente virtual não possui ruído eletromagnético de escovas de carvão nem transientes indutivos. Esses capacitores são essenciais no mundo físico para proteger a placa real. |
| **Chassi PETG, Polias, Rolamentos e Esteiras** | **NÃO** | Mecânica / Cinemática | **Dispensável no Simulador Eletrônico:** Pertencem ao projeto mecânico CAD (Onshape) e manufatura aditiva 3D. O simulador atesta que os comandos diferenciais de esteira (*Skid Steering*) são despachados corretamente nos canais PWM. |

---

## 3. O Que É Comprovado com Exatidão Matemática na Simulação

Para fins de validação técnica de engenharia, a simulação no Wokwi comprova com **100% de precisão**:

1. **Protocolo I2C / SMBus:** O handshake mestre-escravo com o endereço `0x5A`, o suporte a *Repeated Start* e a decodificação dos 16 bits dos registradores `0x06` e `0x07`.
2. **Máquina de Estados Térmica:**
   - Transição para alerta preventivo aos **50 °C** com acionamento do Buzzer intermitente e LED vermelho.
   - Disparo da parada de emergência crítica aos **70 °C** com corte instantâneo de PWM dos motores (`0V`).
3. **Mecanismo Failsafe (Watchdog):** O desligamento autônomo dos motores após 500 ms sem recepção de pacotes de controle.
4. **Frequência e Rampas de PWM (LEDC):** Geração estável de 1500 Hz e escalonamento suave a cada 20 ms via *Slew Rate Limiter*.
5. **Telemetria JSON em Tempo Real:** Formatação de dados a 5 Hz (200 ms) sem travamento da CPU ou consumo excessivo de memória dinâmica.

---

## 4. O Que a Simulação NÃO Substitui (Validação Obrigatória em Bancada)

O simulador valida **software e sinais de controle**. Os seguintes fenômenos físicos devem ser rigorosamente validados na bancada:

1. **Torque e Atrito das Esteiras de Pneu MTB:** O atrito estático real contra o solo e a perda de tração lateral em manobras de giro (*skid steering*).
2. **Queda de Tensão no Driver L298N:** A perda de 2,0V a 3,0V nos transistores Darlington e seu aquecimento sob carga contínua.
3. **Ruído Eletromagnético (EMI):** A eficácia dos capacitores de 100 nF soldados nas escovas dos motores para impedir travamento do barramento I2C.
4. **Calibração com Multímetro:** O ajuste dos trimpots do LM2596 (5,00V) e do Boost 400W (24,0V) antes da conexão com a eletrônica sensível.

---

## 5. Como Executar a Simulação Interativa no VS Code

1. Selecione o ambiente **`env:wokwi`** no PlatformIO.
2. Compile o código clicando no ícone de **Build (✓)** na barra inferior.
3. Abra o arquivo [`diagram.json`] e clique no botão verde de **Play**.
4. **Comandos Interativos via Wokwi Terminal:**
   - Digite **`w`**: Avanço das esteiras (PWM 180 com rampa suave).
   - Digite **`s`**: Recuo das esteiras (Ré).
   - Digite **`a`** / **`d`**: Giro no próprio eixo (*Zero Turn*).
   - Digite **` ` (espaço)**: Parada total imediata.
   - Digite **`l`**: Liga / Desliga os Faróis Olho de Águia (MOSFET na GPIO 4).
5. **Injeção Térmica:** Clique no sensor `gy906` na tela e mova os sliders para testemunhar a atuação das proteções de 50°C e 70°C ao vivo.
