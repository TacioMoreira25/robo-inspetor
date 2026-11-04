# Robo Inspetor Terrestre (UGV)

Projeto de firmware modular em C++ e ambiente de simulacao para Veiculo Terrestre Nao Tripulado (UGV) sobre esteiras, destinado a inspecao visual e termografica em ambientes industriais confinados.

A arquitetura consolida todo o processamento de controle (Dashboard Web, tracao diferencial via Ponte H L298N, iluminacao via Modulo MOSFET Duplo e leitura termica I2C MLX90614) na **ESP32 DevKit V1 (Master - IP `192.168.4.1`)**, que cria a rede Wi-Fi propria do robo (`ROBO_INSPETOR_UGV`, senha `12345678`), enquanto a **ESP32-CAM (Slave - IP `192.168.4.2`)** se conecta nela e fica 100% dedicada ao streaming de video MJPEG na porta 81 (`http://192.168.4.2:81/stream`).

---

## 1. Distribuicao de Energia e Potencia (12V, 24V e 5V)

O sistema parte da **Bateria de Litio 3S2P** (~11,1V a 12,6V / 4400 mAh) conectada via plugue **XT60**:

1. **Linha de Entrada e Protecao (12V Pos-Chave):**
   - O fio positivo (vermelho) do XT60 passa pela **Chave Gangorra KCD1** (interruptor geral) e pelo **Porta-Fusivel Lamina de 15A**, abrindo no **Chicote Principal em Y** que alimenta:
     - `IN+` do Conversor **Step-Up 400W**.
     - `IN+` do Conversor **Step-Down LM2596**.
     - `VCC` de entrada (`V+`) nos bornes do **Modulo MOSFET Duplo**.
     - Fios **Vermelho + Branco** (soldados juntos) do **Voltimetro Digital de 3 Fios**.
     - **Mini Chicote de 12V:** Fio positivo (vermelho) do **Cooler de 80mm (12V)** + fios positivos (vermelhos) dos dois **Farois Olho de Aguia (12V)**.
2. **Barramento de Terra (GND Comum - 0V):**
   - O fio negativo do XT60 alimenta o barramento azul/negativo (`-`) da protoboard e interliga os terras de todos os modulos (`IN-` do Step-Up 400W, `IN-` do Step-Down LM2596, `GND` de potencia e logico do Modulo MOSFET, fio preto do Voltimetro Digital, fio preto do Cooler de 80mm com fio amarelo de tacometro isolado, `GND` da Ponte H L298N, `GND` da ESP32 DevKit V1, `GND` da ESP32-CAM e `GND` do sensor MLX90614).
3. **Linha de 24V (Motores - Saida do Step-Up 400W):**
   - `OUT+ (24V)` -> Borne `12V/VCC` da **Ponte H L298N**.
   - `OUT-` -> Borne `GND` da **Ponte H L298N**.
   - **Motores JGB37-520 (24V):** Conectados nos bornes laterais `OUT1`/`OUT2` (Motor Esquerdo) e `OUT3`/`OUT4` (Motor Direito) da L298N.
4. **Linha de 5V (Logica - Saida do Step-Down LM2596):**
   - `OUT+ (5V)` -> Barramento positivo (`+`) da protoboard (com capacitor eletrolitico de filtro no trilho), distribuindo 5V para:
     - Pino `VIN` / `5V` da **ESP32 DevKit V1**.
     - Pino `5V` da **ESP32-CAM**.
     - Borne `5V` (Logica) da **Ponte H L298N** (com o jumper regulador de 5V da placa L298N removido).

---

## 2. Mapeamento Definitivo de Pinos (ESP32 DevKit V1 - Master)

| Pino ESP32 DevKit | Conectado Onde? | Funcao no Robo |
| :--- | :--- | :--- |
| `VIN` (ou `5V`) | Barramento `+` da Protoboard (5V) | Alimentacao da placa |
| `GND` | Barramento `-` da Protoboard (GND) | Referencia de terra comum |
| `3V3` | Pino `VIN` do Sensor MLX90614 | Alimentacao 3,3V dedicada ao sensor termico |
| `GPIO 26` | Pino `IN 1` da Ponte H L298N | Motor Esquerdo - Sentido A / PWM (1500 Hz) |
| `GPIO 27` | Pino `IN 2` da Ponte H L298N | Motor Esquerdo - Sentido B / PWM (1500 Hz) |
| `GPIO 14` | Pino `IN 3` da Ponte H L298N | Motor Direito - Sentido A / PWM (1500 Hz) |
| `GPIO 13` | Pino `IN 4` da Ponte H L298N | Motor Direito - Sentido B / PWM (1500 Hz) |
| `GPIO 21` | Pino `SDA` do Sensor MLX90614 | Dados I2C da temperatura (Pull-up 4,7 kOhm para 3V3) |
| `GPIO 22` | Pino `SCL` do Sensor MLX90614 | Clock I2C da temperatura (Pull-up 4,7 kOhm para 3V3) |
| `GPIO 25` | Pino `SIG / PWM` do Modulo MOSFET | Controle de brilho dos Farois 12V (PWM 1500 Hz) |
| `GPIO 16` (`RX2`) | Pino `U0T` da ESP32-CAM *(Opcional)* | Reserva fisica de comunicacao serial no chassi |
| `GPIO 17` (`TX2`) | Pino `U0R` da ESP32-CAM *(Opcional)* | Reserva fisica de comunicacao serial no chassi |

*(Nota: Os jumpers `ENA` e `ENB` da Ponte H L298N permanecem fechados na placa, pois o controle de velocidade e direcao e feito diretamente por PWM de 4 canais nos pinos `IN1` a `IN4`).*

---

## 3. Estrutura do Repositorio

- `platformio.ini`: Configuracao dos ambientes (`env:wokwi`, `env:esp32_hardware` para a Master DevKit V1 e `env:esp32cam_hardware` para a Slave ESP32-CAM).
- `wokwi.toml`: Mapeamento do firmware compilado e do chip customizado WebAssembly (`gy906.wasm`) para o simulador Wokwi.
- `diagram.json`: Circuito eletrico virtual da ESP32 DevKit V1 no Wokwi (GY-906 via I2C `GPIO 21/22`, 4 LEDs indicadores dos canais PWM da Ponte H L298N nos pinos `GPIO 26, 27, 14, 13` e LED indicador do Modulo MOSFET na `GPIO 25`).
- `chips/gy906/`: Implementacao em C e binario WebAssembly do sensor termico MLX90614 (I2C no endereco `0x5A`).
- `include/` e `src/`:
  - `config.h`: Pinagem oficial da ESP32 DevKit V1, rede Wi-Fi propria (`ROBO_INSPETOR_UGV` / `12345678`), IPs fixos (`192.168.4.1` e `192.168.4.2`), frequencia PWM (1500 Hz) e timeout de Failsafe (1000 ms).
  - `motor_controller.h` / `motor_controller.cpp`: Controle diferencial por 4 pinos PWM (`IN1..IN4`), cinematica de Joystick Virtual (`driveJoystick`), rampa de aceleracao e corte automatico de Failsafe (1 segundo).
  - `thermal_sensor.h` / `thermal_sensor.cpp`: Driver de leitura I2C SMBus dos registradores `0x06` (Ambiente) e `0x07` (Objeto).
  - `telemetry.h` / `telemetry.cpp`: Servidor Web Master na porta 80 (`http://192.168.4.1`) com Dashboard unificado em `PROGMEM` (embed do stream `http://192.168.4.2:81/stream`, Joystick Virtual em Canvas, teclado `W, A, S, D`, botao/slider de farol e rotas `/cmd`, `/temp` e `/status`).
  - `camera_stream.h` / `camera_stream.cpp`: Servidor HTTP MJPEG dedicado na porta 81 (`/stream`) para a ESP32-CAM.
  - `main.cpp`: Ciclo principal condicional (`PLACA_ESP32_DEVKIT` para Master e `PLACA_ESP32_CAM` para Slave).
- `arduino_sketches/`:
  - `esp32_devkit_master/esp32_devkit_master.ino`: Sketch standalone completo da ESP32 DevKit V1 para gravacao direta via Arduino IDE.
  - `esp32_cam_stream/esp32_cam_stream.ino`: Sketch standalone dedicado ao streaming MJPEG da ESP32-CAM para gravacao via Arduino IDE.
- `docs/`:
  - `01_visao_geral.md`: Distribuicao de energia (12V, 24V e 5V), divisao Master/Slave, balanco de potencia e mecanica em PETG.
  - `02_guia_simulacao.md`: Racional de simulacao no Wokwi e matriz de componentes simulados vs abstraidos.
  - `03_roteiro_bancada.md`: Passo a passo das alteracoes fisicas no chassi, tabela definitiva de pinos e checklist de bancada.
  - `04_pipeline_operacao.md`: Explicacao didatica do funcionamento conjunto entre a ESP32 DevKit V1 (Master) e a ESP32-CAM (Slave).

---

## 4. Como Executar a Simulacao no VS Code (Wokwi)

1. Abra a pasta do projeto no VS Code com as extensoes **PlatformIO IDE** e **Wokwi Simulator** instaladas.
2. Selecione o ambiente `env:wokwi` e compile o projeto (`Ctrl+Alt+B`).
3. Abra o arquivo `diagram.json` e inicie a simulacao.
4. No terminal serial do Wokwi, envie os comandos de teste:
   - `w`: Avanco das esteiras (`IN1` na GPIO 26 e `IN3` na GPIO 14).
   - `s`: Recuo das esteiras (`IN2` na GPIO 27 e `IN4` na GPIO 13).
   - `a`: Giro no proprio eixo para a esquerda.
   - `d`: Giro no proprio eixo para a direita.
   - `Espaco`: Parada imediata dos motores.
   - `l`: Liga ou desliga os Farois 12V no Modulo MOSFET (`GPIO 25`).
5. Clique sobre o sensor `gy906` durante a simulacao para ajustar os sliders de temperatura e validar o Alerta Preventivo (`>= 50 C`) e a Parada Critica (`>= 70 C`), alem do Failsafe automatico apos `1000 ms` sem comandos.
