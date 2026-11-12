# 01. Visão Geral e Distribuição Oficial de Energia (12V, 24V e 5V)

Consolidação oficial da arquitetura Master/Slave e distribuição de potência do **Robô Inspetor Terrestre (UGV)**.

---

## 1. Arquitetura Geral do Hardware (Master / Slave)

* **ESP32 DevKit V1 (Master — IP `192.168.4.1` / `192.168.1.100`):** Centraliza o controle da Ponte H L298N via 4 canais PWM (`GPIO 26, 27, 14, 13`), a leitura I2C do sensor térmico MLX90614 (`GPIO 21, 22`), o acionamento dos faróis via Módulo MOSFET (`GPIO 25`), o Dashboard Web e o *Watchdog Failsafe* de `1000 ms`.
* **ESP32-CAM (Slave — IP `192.168.4.2` / `192.168.1.101`, Porta `81`):** Dedicada exclusivamente ao streaming de vídeo MJPEG (`/stream`). Alimentada apenas pelos pinos `5V` e `GND`, mantendo os pinos `GPIO 12, 13, 14, 15, U0T e U0R` totalmente desconectados para evitar falha de inicialização (*boot*).

---

## 2. Diagrama de Distribuição de Energia (12V, 24V, 5V e GND)

```mermaid
flowchart LR
    BAT_POS["XT60 (+)<br/>Bateria 3S2P (11.1V-12.6V)"]
    KCD["Chave Gangorra KCD1"]
    FUS["Fusível 15A"]
    CHICOTE_POS["Chicote Principal 12V (+)"]

    COOLER["Cooler / Fan 80mm (12V)<br/>(Fio Amarelo isolado)"]
    VOLT["Voltímetro Digital 3 Fios<br/>(Vermelho + Branco unidos)"]
    MOS["Módulo MOSFET XY-MOS V921<br/>(VIN+ / VIN-)"]
    FAROIS["2x Faróis Olho de Águia 12V<br/>(2 Vermelhos em OUT+ | 2 Pretos em OUT-)"]

    STEPUP["Step-Up 400W<br/>(12V -> 24V)"]
    STEPDOWN["Step-Down LM2596<br/>(12V -> 5V)"]

    L298N["Ponte H L298N<br/>• 12V/VCC = 24V do Step-Up<br/>• 5V = 5V direto do Step-Down<br/>(Sem Jumper 5V | ENA/ENB fechados)"]
    MOTORES["2x Motores 24V<br/>(OUT1/OUT2 e OUT3/OUT4)"]

    TRILHO_5V["Trilho Vermelho (+ 5V)<br/>da Protoboard"]
    CAP["Capacitor Eletrolítico"]
    DEV["ESP32 DevKit V1<br/>(Pino VIN/5V)"]
    CAM["ESP32-CAM<br/>(Pino 5V)"]

    BAT_POS --> KCD --> FUS --> CHICOTE_POS

    CHICOTE_POS -->|"Fio Vermelho"| COOLER
    CHICOTE_POS -->|"Vermelho + Branco"| VOLT
    CHICOTE_POS -->|"Borne VIN+"| MOS
    MOS -->|"OUT+ e OUT-"| FAROIS

    CHICOTE_POS -->|"Borne IN+"| STEPUP
    STEPUP -->|"OUT+ (24V) -> 12V/VCC"| L298N --> MOTORES

    CHICOTE_POS -->|"Borne IN+"| STEPDOWN
    STEPDOWN -->|"Fio 1 do OUT+ (5V) -> Borne 5V"| L298N
    STEPDOWN -->|"Fio 2 do OUT+ (5V)"| TRILHO_5V

    TRILHO_5V --> CAP
    TRILHO_5V -->|"Jumper Branco/Vermelho"| DEV
    TRILHO_5V -->|"Fio 5V"| CAM
```

### Diagrama do Chicote Negativo e Trilho Azul (`-` GND Geral)

```mermaid
flowchart LR
    BAT_NEG["XT60 (-)<br/>Chicote Negativo da Bateria"]
    SU_IN["IN- do Step-Up 400W"]
    SD_IN["IN- do Step-Down LM2596"]
    MOS_IN["VIN- do Módulo MOSFET XY-MOS"]

    SD_OUT["OUT- do Step-Down LM2596"]
    SU_OUT["OUT- do Step-Up 400W"]

    TRILHO_GND["Trilho Azul (- GND Geral)<br/>da Protoboard"]
    L298N_GND["Borne GND da Ponte H L298N"]

    BAT_NEG --> SU_IN
    BAT_NEG --> SD_IN
    BAT_NEG --> MOS_IN

    SD_OUT -->|"Entrada GND da Protoboard"| TRILHO_GND
    SU_OUT --> L298N_GND
    TRILHO_GND -->|"1 Fio de unificação"| L298N_GND

    TRILHO_GND --- G1["Capacitor Eletrolítico (Faixa -)"]
    TRILHO_GND --- G2["GND da ESP32 DevKit V1 (Fio Preto e Branco)"]
    TRILHO_GND --- G3["GND da ESP32-CAM"]
    TRILHO_GND --- G4["GND do Sensor MLX90614 (Fio Azul)"]
    TRILHO_GND --- G5["Furo GND Lógico do Módulo MOSFET"]
    TRILHO_GND --- G6["Fio Preto do Cooler 80mm"]
    TRILHO_GND --- G7["Fio Preto do Voltímetro Digital"]
```

---

## 3. Tabela Completa de Distribuição de Energia e Módulos de Potência

| Subsistema / Módulo | Ligação Positiva (`+`) | Ligação Negativa (`GND`) | Saídas e Detalhes da Montagem |
| --- | --- | --- | --- |
| **Chicote Principal 12V** | Sai do `XT60 (+)` -> Chave Gangorra **KCD1** -> **Fusível 15A** | Sai do `XT60 (-)` e divide-se para o `IN-` do Step-Up, `IN-` do Step-Down e `VIN-` do MOSFET | Alimenta em 12V o Step-Up, Step-Down, MOSFET, Voltímetro e Cooler 80mm. |
| **Cooler / Fan 80mm (12V)** | Fio **Vermelho** direto no Chicote 12V pós-chave | Fio **Preto** no Trilho Azul (`-` GND) da protoboard | Fio amarelo (tacômetro) isolado/sem uso. Gira continuamente ao ligar a chave. |
| **Voltímetro Digital (3 Fios)** | Fios **Vermelho + Branco unidos** no Chicote 12V pós-chave | Fio **Preto** no Trilho Azul (`-` GND) da protoboard | Mede a tensão da bateria direto no painel físico do chassi. |
| **Módulo MOSFET (`XY-MOS V921`)** | Borne **`VIN+`** no Chicote 12V pós-chave | Borne **`VIN-`** no Chicote Negativo da bateria | • Borne **`OUT+`:** Recebe os **2 fios vermelhos (`+`)** dos Faróis Olho de Águia.<br>• Borne **`OUT-`:** Recebe os **2 fios pretos (`-`)** dos Faróis Olho de Águia.<br>• Furos lógicos: `TRIG/PWM` no **`GPIO 25`** (fio cinza) e `GND` no Trilho `-` da protoboard. |
| **Step-Up 400W (12V -> 24V)** | Borne **`IN+`** no Chicote 12V | • **`IN-`** no Chicote Negativo da bateria<br>• **`OUT-`** no borne `GND` da Ponte H L298N | Borne **`OUT+` (24V)** vai direto ao borne `12V/VCC` da Ponte H L298N. |
| **Step-Down LM2596 (12V -> 5V)** | Borne **`IN+`** no Chicote 12V | • **`IN-`** no Chicote Negativo da bateria<br>• **`OUT-`** vai ao **Trilho Azul (`-`) da Protoboard** | Borne **`OUT+` (5V)** divide-se em **2 fios**: um vai para o **Trilho Vermelho (`+`) da Protoboard** e o outro vai **direto ao borne `5V` da Ponte H L298N**. |
| **Ponte H L298N** | • Motores (`12V/VCC`): **24V** do Step-Up<br>• Lógica (`5V`): **5V** direto do Step-Down | Borne **`GND`** recebe o `OUT-` do Step-Up **+ 1 fio** ligado ao Trilho Azul (`-`) da Protoboard | **Jumper regulador de 5V removido**. Jumpers `ENA` e `ENB` **mantidos fechados**. Bornes `OUT1/OUT2` (Motor Esq.) e `OUT3/OUT4` (Motor Dir.). |
