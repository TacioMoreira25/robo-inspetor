# 04. Arquitetura de Software e Ciclo Completo de Operação e Testes (Hotspot Galaxy A20)

Fluxo oficial de operação com o **Samsung Galaxy A20** atuando como **Roteador Wi-Fi Central (Hotspot `robo_ugv`)** + servidor de vídeo **IP Webcam**, eliminando o gargalo de latência da caixa fechada da ESP32.

---

## 1. Como Funciona a Rede com o Galaxy A20 como Roteador Central

1. **Por que não precisa configurar IP fixo (`192.168.4.1`) no código da ESP32?**
   - Quando o **Galaxy A20** liga o **Roteador Wi-Fi (`robo_ugv`)**, o próprio celular vira o servidor DHCP da rede (normalmente na faixa **`192.168.43.1`**).
   - A ESP32 se conecta em modo `WIFI_STA` via `WiFi.begin("robo_ugv", "12345678")` e **recebe o IP automaticamente via DHCP do Galaxy A20** (sem precisar das constantes antigas `192.168.4.1`).
   - Além do IP numérico, a ESP32 registra o endereço **`http://ugv.local`** via mDNS, e o próprio painel descobre o IP da câmera do A20 automaticamente lendo o Gateway da rede (`WiFi.gatewayIP()`, ex.: `192.168.43.1:8080/video`).

2. **Por que a latência fica estável nessa arquitetura?**
   - O vídeo pesado (`MJPEG`) vai **direto do Galaxy A20 para o Notebook** (não passa por dentro da ESP32 na caixa fechada).
   - A ESP32 trafega apenas pacotes minúsculos de comando (`/cmd`) e telemetria (`/temp`).

---

## 2. Mapeamento Oficial de Comandos (Teclado e Controle Gamepad)

| Função no Robô | Teclado (PC) | PlayStation (PS5 / PS4) | Xbox / Machenike G5Pro |
| :--- | :--- | :--- | :--- |
| **Acelerar (Frente)** | `W` ou `▲` | **`R2`** (`buttons[7]`) | **`RT`** (`buttons[7]`) |
| **Freio / Ré (Lógica Forza)** | `S` ou `▼` | **`L2`** (1º toque freia, segurar ~320ms dá ré) | **`LT`** (1º toque freia, segurar ~320ms dá ré) |
| **Direção (Esquerda / Direita)** | `A` / `D` ou `◀` / `▶` | **Analógico Esquerdo (`◀/▶`)** | **Analógico Esquerdo (`◀/▶`)** |
| **Direção Completa (Setas)** | `▲` `▼` `◀` `▶` | **D-Pad (`▲` `▼` `◀` `▶`)** | **D-Pad (`▲` `▼` `◀` `▶`)** |
| **Curva Suave em Movimento** | `W+A` / `W+D` (`▲+◀` / `▲+▶`) | **`R2` + Analógico Esquerdo** | **`RT` + Analógico Esquerdo** |
| **Freio Total (Emergência)** | `Espaço` | **`L1`** (`buttons[4]`) | **`LB`** (`buttons[4]`) |
| **Ligar / Desligar Faróis 12V** | `L` | **Triângulo (`△`)** (`buttons[3]`) | **`Y`** (`buttons[2]` no Machenike / `buttons[3]` no Xbox) |
| **Auto-Teste de Bancada** | `T` | Botão na tela | Botão na tela |
| **Analógico Direito** | — | **100% Desativado** | **100% Desativado** (com filtro `ax[6]/ax[7]` no Machenike `0079-181c`) |

---

## 3. Ciclo Completo Passo a Passo para Testar

### Passo 1 — Configurar o Hotspot e o IP Webcam no Galaxy A20
1. No **Galaxy A20**, abra **Configurações > Conexões > Roteador Wi-Fi e Ancoragem > Roteador Wi-Fi**:
   - **Nome da rede:** `robo_ugv`
   - **Senha:** `12345678`
   - **Banda:** `2.4 GHz`
   - Ative o **Roteador Wi-Fi**. *(Observação: no Galaxy A20 não precisa nem ter chip/dados móveis ligados para a rede local funcionar).*
2. Abra o app **IP Webcam** no Galaxy A20:
   - Em **Video preferences**: Resolução `640x480`, Quality `50`, FPS limit `15` ou `20`, Orientation `Landscape`.
   - Em **Audio mode**: `Disabled`.
   - Role até o final e toque em **`Start server`**.
   - Anote o IP exibido na parte inferior da tela (no Galaxy A20 com Hotspot ligado, quase sempre é **`http://192.168.43.1:8080`**).

### Passo 2 — Compilar e Gravar a ESP32 DevKit V1
No terminal do VS Code dentro da pasta `/home/tacio/dev/academic/robo-inspetor`:
```bash
pio run -e esp32_hardware -t clean && pio run -e esp32_hardware -t upload
```

### Passo 3 — Ligar o Robô e Conectar o Notebook no Hotspot do A20
1. Ligue a **Chave Gangorra KCD1** do robô — a ESP32 vai se conectar sozinha no Hotspot **`robo_ugv`** do Galaxy A20 (você verá **1 aparelho conectado** na tela de Roteador Wi-Fi do A20).
2. Conecte o seu **Notebook** na mesma rede Wi-Fi **`robo_ugv`** (senha `12345678`).
3. Conecte o controle (**Machenike G5Pro / PS5 / Xbox**) no Notebook.
4. Abra o navegador do Notebook em qualquer uma das 3 opções abaixo:
   - **Opção A (automática via mDNS):** acesse **`http://ugv.local`**
   - **Opção B (pelo IP da ESP32):** toque no nome da ESP32 dentro da lista de *Aparelhos Conectados* do Roteador Wi-Fi do Galaxy A20 (ou olhe no Monitor Serial) e acesse `http://192.168.43.XXX`
   - **Opção C (arquivo local):** abra direto o arquivo `web/index.html` no navegador do seu computador (ele já se comunica automaticamente com `http://ugv.local`).
5. O vídeo da câmera do A20 (`192.168.43.1:8080/video`) já entra automático pelo Gateway; se precisar trocar, basta digitar o IP no campo **`IP CELULAR`** no topo da tela e clicar em **`Conectar`**.

### Passo 4 — Checklist Rápido na Pista
1. **Aperte qualquer botão do controle:** o badge do Joystick muda para **`CONTROLE [STD]`** ou **`CONTROLE [RAW]`**.
2. **Acelerar (`RT` / `R2` ou `W` / `▲`):** robô avança em linha reta.
3. **Esquerda / Direita (`Analógico Esquerdo` ou `◀/▶`):** andando faz curva suave sem parar; parado gira no eixo.
4. **Freio + Ré Forza (`LT` / `L2`):** andando para frente, 1 toque freia; segurar ~0,3s engata a ré.
5. **Freio Total (`LB` / `L1` ou `Espaço`)** e **Faróis (`Y` / `△` ou `L`)**.
