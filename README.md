# Robo Inspetor Terrestre (UGV) - Firmware Unico e Simulacao V1.0

Projeto de firmware modular em C++ para Veiculo Terrestre Nao Tripulado (UGV) sobre esteiras, destinado a inspecao preditiva e termografica em ambientes industriais confinados.

O projeto adota o principio de Firmware Unico com Camada de Abstracao de Hardware (HAL): o mesmo codigo-fonte e simulado no Wokwi Simulator dentro do VS Code e gravado via USB no hardware fisico (ESP32 DevKit V1 e ESP32-CAM) na bancada.

---

## 1. Arquitetura de Hardware e Alimentacao

A distribuicao eletrica do robo utiliza topologia trifurcada a partir de um pack de baterias Li-Ion 3S2P (11,1 V nominal / 12,6 V pico / 4400 mAh com BMS de 10 A e fusivel lamina de 15 A):

1. Linha de Tracao (24,0 V Regulado): Elevada por conversor Step-Up Boost de 400W para alimentar o pino VMS do driver Ponte H L298N e os dois motorredutores JGB37-520 (24V / 107 RPM).
2. Linha Logica (5,00 V Regulado): Rebaixada por conversor Step-Down Buck LM2596 para alimentar a ESP32-CAM, a ESP32 DevKit V1, o Cooler Fan de 80 mm (`DF0922505SEL` - 5,00 V DC / 0,09 A / 0,45 W), o buzzer ativo e o estagio logico do L298N.
3. Linha Auxiliar 12V Direto (11,1 V a 12,6 V): Alimenta diretamente os Farois LED Olho de Aguia (via modulo MOSFET D4184), o voltimetro LED de painel e o divisor de tensao analogico (10 kOhm e 3,3 kOhm conectado a GPIO 34 da ESP32 DevKit V1).

Avisos Criticos de Seguranca:
- A remocao do jumper de 5V da placa L298N e obrigatoria antes da energizacao com 24V.
- O Cooler Fan de 80 mm (`DF0922505SEL`) opera em 5,00 V DC e deve ser conectado exclusivamente na saida regulada de 5,00 V do conversor LM2596 (jamais nos 12V diretos da bateria).

---

## 2. Estrutura do Repositorio

- `platformio.ini`: Configuracao dos ambientes de compilacao (`env:wokwi`, `env:esp32_hardware` e `env:esp32cam_hardware`).
- `wokwi.toml`: Mapeamento de binarios e do chip customizado WebAssembly para o simulador.
- `diagram.json`: Circuito eletrico virtual interativo (ESP32, GY-906, potenciometro divisor de tensao da bateria na GPIO 34, sinalizacao bidirecional de motores, farol, LED de alerta e buzzer).
- `chips/gy906/`: Implementacao em C e binario WebAssembly (`gy906.wasm`) do sensor termico MLX90614 (I2C no endereco 0x5A).
- `include/`:
  - `config.h`: Mapeamento de GPIOs, frequencia PWM (1500 Hz), parametros do divisor de tensao da bateria (10k/3.3k), limites termicos e temporizacoes.
  - `motor_controller.h`: Tipos e assinaturas de controle diferencial com rampa de aceleracao e failsafe.
  - `thermal_sensor.h`: Driver de leitura I2C via registradores de RAM 0x06 e 0x07.
  - `telemetry.h`: Servidor Web SoftAP, rotas de comando e serializacao JSON (incluindo tensao e porcentagem da bateria).
  - `camera_stream.h`: Abstracao do pipeline de video (Mock JPEG no simulador vs OV2640 DMA no hardware real).
- `src/`:
  - `main.cpp`: Ciclo principal, leitura ADC da bateria (GPIO 34), autoteste de hardware e maquina de estados operacionais.
  - `motor_controller.cpp`: Geracao de PWM via LEDC, controle de rampa (50 Hz), deadband e corte por watchdog (500 ms).
  - `thermal_sensor.cpp`: Comunicacao I2C com Repeated Start e conversao Kelvin para Celsius.
  - `telemetry.cpp`: Dashboard HTML5, despacho JSON na rota `/status` e comandos via `/control` e `/light`.
  - `camera_stream.cpp`: Tratamento condicional para streaming de video MJPEG na porta 81.
- `docs/`:
  - `01_visao_geral.md`: Especificacoes eletricas (Cooler 5V no LM2596), balanco de potencia, curva de queda do L298N e projeto mecanico PETG.
  - `02_guia_simulacao.md`: Justificativa tecnica de componentes simulados vs abstraidos e lista dos 8 itens de protecao de bancada.
  - `03_roteiro_bancada.md`: Tabela de ligacoes, localizacao dos 8 componentes de protecao/vedacao e checklist de pre-energizacao em 4 fases.
  - `04_pipeline_operacao.md`: Ciclo de vida operacional com termos tecnicos traduzidos e explicados.

---

## 3. Como Executar a Simulacao no VS Code

### Pre-Requisitos
- Visual Studio Code instalado.
- Extensao PlatformIO IDE (`platformio.platformio-ide`).
- Extensao Wokwi Simulator (`wokwi.wokwi-vscode`).

### Passo a Passo
1. Abra a pasta do projeto no VS Code:
   ```bash
   code "/home/tacio/dev/academic/robo-inspetor"
   ```
2. Selecione o ambiente `env:wokwi` na barra inferior do PlatformIO.
3. Compile o projeto clicando no icone Build (✓) ou pressionando `Ctrl+Alt+B`.
4. Abra o arquivo `diagram.json` e clique no botao Play do Wokwi (ou pressione `F1` e digite `Wokwi: Start Simulator`).

---

## 4. Comandos de Teste no Monitor Serial

Com a simulacao em execucao, clique no Wokwi Terminal e envie os seguintes caracteres:

- `w`: Avanco das esteiras (Frente - PWM 180 com rampa suave).
- `s`: Recuo das esteiras (Re - PWM -180).
- `a`: Manobra de giro para a esquerda (Zero Turn - esteira esquerda reverte, direita avanca).
- `d`: Manobra de giro para a direita (Zero Turn - esteira esquerda avanca, direita reverte).
- `Espaco`: Parada total imediata (PWM 0 em ambos os motores).
- `l`: Alterna os Farois LED Olho de Aguia entre ligado (100%) e desligado.

---

## 5. Validacao dos Sensores, Bateria e Segurancas

1. Ajuste Termico Interativo: Clique no sensor `gy906` na tela do simulador para abrir os controles deslizantes.
   - Ajuste para 50 °C a 69 °C: Dispara o Alerta Preventivo (LED vermelho acende e o buzzer apita de forma intermitente).
   - Ajuste para 70 °C ou superior: Dispara a Parada Critica de Emergencia (buzzer emite som continuo e a tracao corta imediatamente para zero).
2. Simulacao de Descarga da Bateria 3S (GPIO 34): Gire o potenciometro `Divisor Bateria 3S (GPIO 34)` no simulador.
   - Tensao entre 10,50 V e 12,60 V: Operacao normal.
   - Tensao abaixo de 10,50 V (~22%): Dispara Alerta Preventivo de Bateria Baixa (buzzer intermitente, LED de alerta e flag `low_warn` no JSON).
   - Tensao abaixo de 9,90 V (0%): Dispara Corte Critico de Bateria (desliga os motores imediatamente para proteger as celulas Li-Ion 3S).
3. Failsafe de Conectividade: Apos 500 ms sem recepcao de comandos de movimento, o watchdog assume estado ativo (`"watchdog": true`) e trava os motores, prevenindo deslocamento desgovernado em caso de perda de sinal.
