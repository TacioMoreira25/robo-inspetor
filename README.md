# Robo Inspetor Terrestre (UGV) - Documentacao Oficial de Hardware e Firmware

Consolidacao oficial e definitiva da montagem fisica, distribuicao de energia, pinagem e operacao do **Robo Inspetor Terrestre (UGV)**.

---

## 1. Arquitetura Geral do Hardware (Master / Slave)

* **ESP32 DevKit V1 (Master — IP `192.168.4.1` / `192.168.1.100`):** Centraliza o controle da Ponte H L298N via 4 canais PWM (`GPIO 26, 27, 14, 13`), a leitura I2C do sensor termico MLX90614 (`GPIO 21, 22`), o acionamento dos farois via Modulo MOSFET (`GPIO 25`), o Dashboard Web e o *Watchdog Failsafe* de `1000 ms`.
* **ESP32-CAM (Slave — IP `192.168.4.2` / `192.168.1.101`, Porta `81`):** Dedicada exclusivamente ao streaming de video MJPEG (`/stream`). Alimentada apenas pelos pinos `5V` e `GND`, mantendo os pinos `GPIO 12, 13, 14, 15, U0T e U0R` totalmente desconectados para evitar falha de inicializacao (*boot*).

---

## 2. Distribuicao de Energia e Modulos de Potencia

Toda a alimentacao parte da bateria **3S2P Li-Ion (~11.1V a 12.6V)** conectada via plugue **XT60**:

| Subsistema / Modulo | Ligacao Positiva (`+`) | Ligacao Negativa (`GND`) | Saidas e Detalhes da Montagem |
| --- | --- | --- | --- |
| **Chicote Principal 12V** | Sai do `XT60 (+)` -> Chave Gangorra **KCD1** -> **Fusivel 15A** | Sai do `XT60 (-)` e divide-se para o `IN-` do Step-Up, `IN-` do Step-Down e `VIN-` do MOSFET | Alimenta em 12V o Step-Up, Step-Down, MOSFET, Voltimetro e Cooler 80mm. |
| **Cooler / Fan 80mm (12V)** | Fio **Vermelho** direto no Chicote 12V pos-chave | Fio **Preto** no Trilho Azul (`-` GND) da protoboard | Fio amarelo (tacometro) isolado/sem uso. Gira continuamente ao ligar a chave. |
| **Voltimetro Digital (3 Fios)** | Fios **Vermelho + Branco unidos** no Chicote 12V pos-chave | Fio **Preto** no Trilho Azul (`-` GND) da protoboard | Mede a tensao da bateria direto no painel fisico do chassi. |
| **Modulo MOSFET (`XY-MOS V921`)** | Borne **`VIN+`** no Chicote 12V pos-chave | Borne **`VIN-`** no Chicote Negativo da bateria | • Borne **`OUT+`:** Recebe os **2 fios vermelhos (`+`)** dos Farois Olho de Aguia.<br>• Borne **`OUT-`:** Recebe os **2 fios pretos (`-`)** dos Farois Olho de Aguia.<br>• Furos logicos: `TRIG/PWM` no **`GPIO 25`** (fio cinza) e `GND` no Trilho `-` da protoboard. |
| **Step-Up 400W (12V -> 24V)** | Borne **`IN+`** no Chicote 12V | • **`IN-`** no Chicote Negativo da bateria<br>• **`OUT-`** no borne `GND` da Ponte H L298N | Borne **`OUT+` (24V)** vai direto ao borne `12V/VCC` da Ponte H L298N. |
| **Step-Down LM2596 (12V -> 5V)** | Borne **`IN+`** no Chicote 12V | • **`IN-`** no Chicote Negativo da bateria<br>• **`OUT-`** vai ao **Trilho Azul (`-`) da Protoboard** | Borne **`OUT+` (5V)** divide-se em **2 fios**: um vai para o **Trilho Vermelho (`+`) da Protoboard** e o outro vai **direto ao borne `5V` da Ponte H L298N**. |
| **Ponte H L298N** | • Motores (`12V/VCC`): **24V** do Step-Up<br>• Logica (`5V`): **5V** direto do Step-Down | Borne **`GND`** recebe o `OUT-` do Step-Up **+ 1 fio** ligado ao Trilho Azul (`-`) da Protoboard | **Jumper regulador de 5V removido**. Jumpers `ENA` e `ENB` **mantidos fechados**. Bornes `OUT1/OUT2` (Motor Esq.) e `OUT3/OUT4` (Motor Dir.). |

---

## 3. Barramentos da Protoboard e Pinagem das Placas

### A. Ligacoes nos Trilhos da Protoboard

* **Trilho Vermelho (`+` 5V):**
  * **Entrada:** Fio vindo do `OUT+` (5V) do Step-Down LM2596.
  * **Saidas:** Capacitor Eletrolitico (lado positivo) + Jumper `VIN/5V` da ESP32 DevKit V1 + Fio `5V` da ESP32-CAM. *(O borne 5V da Ponte H vem direto da saida do Step-Down).*
* **Trilho Azul (`-` GND Geral):**
  * **Entrada:** Fio vindo do `OUT-` do Step-Down LM2596 (unificando o terra da bateria com a protoboard).
  * **Saidas:** Capacitor Eletrolitico (lado da faixa negativa) + `GND` da ESP32 DevKit V1 (fio preto e branco) + `GND` da ESP32-CAM + `GND` do Sensor MLX90614 (fio azul) + Fio de `GND` da Ponte H L298N + Furo `GND` do Modulo MOSFET + Fio preto da Fan 80mm + Fio preto do Voltimetro.

---

### B. Tabela Oficial de Pinos e Cores da ESP32 DevKit V1 (Master)

| Pino na ESP32 DevKit V1 | Constante no Codigo (`config.h`) | Cor do Jumper Fisico | Onde esta Conectado (Destino) | Funcao no Sistema |
| --- | --- | --- | --- | --- |
| **`GPIO 26`** | `PIN_MOTOR_LEFT_IN1` | **Cinza** | Pino **`IN 1`** da Ponte H L298N | PWM Motor Esquerdo Sentido A (Canal LEDC 0) |
| **`GPIO 27`** | `PIN_MOTOR_LEFT_IN2` | **Azul** | Pino **`IN 2`** da Ponte H L298N | PWM Motor Esquerdo Sentido B (Canal LEDC 1) |
| **`GPIO 14`** | `PIN_MOTOR_RIGHT_IN3` | **Amarelo** | Pino **`IN 3`** da Ponte H L298N | PWM Motor Direito Sentido A (Canal LEDC 2) |
| **`GPIO 13`** | `PIN_MOTOR_RIGHT_IN4` | **Verde** | Pino **`IN 4`** da Ponte H L298N | PWM Motor Direito Sentido B (Canal LEDC 3) |
| **`GPIO 25`** | `PIN_HEADLIGHT_MOSFET` | **Cinza** (avulso) | Furo **`TRIG/PWM`** do Modulo MOSFET (`XY-MOS`) | Controle PWM/Digital dos Farois Olho de Aguia (Canal LEDC 4) |
| **`GPIO 21` (`SDA`)** | `PIN_I2C_SDA` | **Amarelo** | Pino **`SDA`** do Sensor MLX90614 | Dados I2C + Resistor pull-up de `4.7kOhm` ligado ao `3V3` |
| **`GPIO 22` (`SCL`)** | `PIN_I2C_SCL` | **Verde** | Pino **`SCL`** do Sensor MLX90614 | Clock I2C + Resistor pull-up de `4.7kOhm` ligado ao `3V3` |
| **`3V3`** | — | **Laranja** | Pino **`VIN`** do Sensor MLX90614 | Alimenta o sensor em 3.3V e os 2 resistores de `4.7kOhm` |
| **`VIN` (ou `5V`)** | — | **Branco / Vermelho** | **Trilho Vermelho (`+` 5V)** da Protoboard | Entrada de 5V da placa (retirar ao gravar via cabo USB) |
| **`GND`** | — | **Preto e Branco** | **Trilho Azul (`-` GND)** da Protoboard | Terra comum de referencia (0V) |

---

### C. Ligacoes da ESP32-CAM (Slave — Na Proa)

* **Pino `5V`:** Conectado ao **Trilho Vermelho (`+` 5V)** da Protoboard.
* **Pino `GND`:** Conectado ao **Trilho Azul (`-` GND)** da Protoboard.
* **Pinos `GPIO 12, 13, 14, 15, U0T, U0R`:** Totalmente livres/desconectados.

---

## 4. Gravacao e Teste Rapido no VS Code (PlatformIO)

1. **Gravar ESP32 DevKit V1 (Master):**
   * Remova temporariamente o jumper `VIN/5V` (Branco/Vermelho) da protoboard antes de plugar o USB.
   * Selecione `env:esp32_hardware` na barra inferior do VS Code e clique na seta `->` (Upload).
2. **Gravar ESP32-CAM (Slave):**
   * Use a ESP32 DevKit V1 em Modo Ponte (`EN` no `GND` e `IO0` da CAM no `GND`), selecione `env:esp32cam_hardware` e clique em `->` (Upload).
3. **Testar os Motores e Sensores:**
   * Conecte o celular/PC no Wi-Fi `ROBO_INSPETOR_UGV` (senha `12345678`) e abra `http://192.168.4.1`, ou pressione `t` no Monitor Serial do VS Code para executar o Auto-Teste sequencial dos 4 canais de motor e farol.
