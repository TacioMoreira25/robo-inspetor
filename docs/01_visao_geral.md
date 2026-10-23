# 01. Visão Geral da Arquitetura: Robô Inspetor Terrestre (UGV) V1.0

O **Robô Inspetor Terrestre (UGV)** é uma plataforma móvel autônoma e teleoperada baseada em esteiras de borracha contínuas (derivadas de pneu MTB vulcanizado), projetada para inspeção visual e termográfica preditiva em ambientes industriais confinados (tubulações, galerias técnicas e subestações).

---

## 1. Topologia Elétrica Trifurcada e Distribuição de Potência

A arquitetura de potência do robô é dividida em três barramentos de tensão distintos a partir de uma fonte de energia primária recarregável única (Pack 3S2P Li-Ion):

```
+-----------------------------------------------------------------------------------+
|                            PACK BATERIA 3S2P Li-Ion                               |
|                     (11,1 V Nominal / 12,6 V Pico / 4400 mAh)                     |
+-----------------------------------------------------------------------------------+
                                          |
                                 [ Fusível Lâmina 15A ]
                                          |
                                 [ Chave Geral KCD1-101 ]
                                          |
        +---------------------------------+---------------------------------+
        |                                 |                                 |
        v                                 v                                 v
+-----------------------+     +-----------------------+     +-----------------------+
|  BARRAMENTO DIRECT 3S |     |  STEP-DOWN BUCK LM2596|     |  STEP-UP BOOST 400W   |
|   (11,1 V - 12,6 V)   |     |   (5,00 V Regulado)   |     |   (24,0 V Regulado)   |
+-----------------------+     +-----------------------+     +-----------------------+
        |                                 |                                 |
        +-- Faróis Olho de Águia          +-- ESP32-CAM                     +-- Driver L298N
        +-- MOSFET D4184 (Potência)       +-- ESP32 DevKit V1                   (Potência VMS)
        +-- Voltímetro LED DC             +-- Cooler 80mm (DF0922505SEL)
        +-- Divisor ADC (10k / 3,3k)      +-- Buzzer / LED (5V)
                                          +-- L298N (Lógica 5V)
```

### 1.1 Detalhamento dos Três Barramentos

1. **Linha de Potência de Tração (24,0 V Regulado):**
   - Elevada e estabilizada por um módulo conversor **Step-Up Boost de 400W / 15A**.
   - O trimpot é travado em **24,0 V** para alimentar o pino de potência `VMS` da Ponte H L298N.
   - Fornece tensão e torque nominais aos **2x motorredutores JGB37-520 (24V / 107 RPM)**.
2. **Linha Lógica Regulada (5,00 V Estabilizado):**
   - Rebaixada por módulo conversor **Step-Down Buck LM2596**, ajustado em **5,00 V ± 0,02 V**.
   - Alimenta os módulos de processamento **ESP32-CAM** e **ESP32 DevKit V1**, o **Cooler Fan de 80 mm (`DF0922505SEL` - 5,00 V DC / 0,09 A / 0,45 W)** em paralelo, os circuitos lógicos de 5V da Ponte H L298N e o circuito sonoro/visual de alerta (Buzzer/LED).
   - *Nota Crítica de Hardware:* O cooler real especificado (`DF0922505SEL`) opera em **5,00 V DC** e jamais deve ser ligado ao barramento de 12V da bateria sob risco de queima imediata do motor brushless.
3. **Barramento Auxiliar 12V Direto (11,1 V – 12,6 V Não-Regulado):**
   - Conectado diretamente à saída pós-chave/fusível da bateria.
   - Alimenta os **2x Faróis LED Olho de Águia** (chaveados pelo módulo MOSFET D4184), o Voltímetro Digital LED de painel e o **Divisor de Tensão Analógico ($10\text{ k}\Omega$ e $3,3\text{ k}\Omega$)** conectado à porta ADC `GPIO 34` da ESP32 DevKit V1 para leitura de carga da bateria.

---

## 2. Balanço de Potência e Margem Térmica do BMS 3S

A elevação de tensão (Step-Up Boost de 11,1V–12,6V para 24,0V) impõe maior demanda de corrente no barramento primário ($P_{in} = \frac{P_{out}}{\eta}$):

1. **Potência de Saída dos Motores:** Em regime severo ou partida em rampa, os dois motores JGB37-520 demandam até 3,0 A combinados em 24,0 V:
   $$P_{out} = 24,0\text{ V} \times 3,0\text{ A} = 72,0\text{ W}$$
2. **Potência de Entrada no Boost:** Com eficiência típica de conversão $\eta \approx 85\%$:
   $$P_{in} = \frac{72,0\text{ W}}{0,85} \approx 84,7\text{ W}$$
3. **Corrente Demandada da Bateria:** No pior caso operacional (bateria no patamar inferior de 11,1 V):
   $$I_{boost} = \frac{84,7\text{ W}}{11,1\text{ V}} \approx 7,63\text{ A}$$
4. **Cargas Auxiliares e Linha Lógica:**
   - Faróis LED Olho de Águia (Direct 3S): $0,50\text{ A}$
   - Linha Lógica 5V (ESP32-CAM + ESP32 DevKit V1 + Cooler 80mm 5V a $0,09\text{ A}$ + sensores): $\approx 0,34\text{ A}$ equivalente na entrada de 11,1V do Buck LM2596.
   - **Corrente Total Contínua no Primário:** $\approx 8,47\text{ A}$.
5. **Margem de Segurança do BMS:** O pack possui BMS 3S com limite contínuo de **10,0 A** e fusível lâmina de **15 A**. A demanda de 8,47 A opera com **15,3% de margem de segurança** abaixo do corte da BMS, eliminando desarmes térmicos espúrios em campo.

---

## 3. Resumo de Malhas e Tensões

| Malha / Barramento | Tensão / Ripple Máx | Corrente Nominal / Máx | Dispositivos Associados |
|---|---|---|---|
| **Entrada Primária (Direct 3S)** | 11,1 V – 12,6 V / $\le 150\text{ mV}$ | 4,4 Ah / 10,0 A (BMS) / 15 A (Fusível) | Pack 3S2P, Chave KCD1-101, Voltímetro LED, MOSFET D4184 (Faróis), Divisor ADC ($10\text{k}\Omega / 3,3\text{k}\Omega$). |
| **Linha Lógica Regulada** | 5,00 V (Buck) / $\le 30\text{ mV}$ | 2,0 A Nom. / 3,0 A Pico | LM2596, ESP32-CAM (5V), ESP32 DevKit V1, Cooler 80mm (`DF0922505SEL` 5V), Buzzer 5V, Lógica L298N. |
| **Linha de Potência Tração** | 24,0 V (Boost) / $\le 100\text{ mV}$ | 3,0 A Nom. / 15,0 A (Módulo) | Boost 400W, Pino VMS do L298N, 2x Motores JGB37-520 (24V). |
| **Linha Auxiliar Sinalização** | 3,30 V (LDO ESP32) | 150 mA | Barramento I2C, Sensor Térmico MLX90614, Pull-ups de 4,7 kΩ, Entrada ADC GPIO 34. |

---

## 4. Dinâmica de Tração e Queda de Tensão no Driver L298N

O driver L298N utiliza transistores bipolares de junção (BJT Darlington), apresentando uma queda interna de saturação ($V_{CE(sat)}$) de 1,0 V a 1,5 V por transistor. Como dois transistores conduzem simultaneamente por motor:
- **Queda de Tensão Total no Driver:** $V_{drop} \approx 2,0\text{ V a } 3,0\text{ V}$.
- **Tensão Efetiva nos Motores:** $V_{efetiva} = 24,0\text{ V} - 2,5\text{ V} \approx 21,5\text{ V}$.
- **Ajuste de Rotação:** Os motores de 107 RPM nominais operam a aproximadamente $93\text{ RPM a } 98\text{ RPM}$ em plena carga, proporcionando excelente estabilidade mecânica e elevado torque de rampa em baixas velocidades.

---

## 5. Estrutura Mecânica e CAD em PETG

- **Chassi Monobloco:** $240 \times 160 \times 75\text{ mm}$ impresso em PETG (espessura de parede 2,5 mm, 100% de infill nos suportes de eixo e motores).
  - 2x furos traseiros de $\varnothing 12,0\text{ mm}$ para acoplamento dos flanges de alumínio de 6 mm aos eixos D dos motores.
  - 2x furos centrais fixos de $\varnothing 8,2\text{ mm}$ para eixos mortos M8.
  - 2x rasgos oblongos dianteiros de $25 \times 8,2\text{ mm}$ dispostos horizontalmente como esticadores das esteiras.
- **Carenagem Superior:** Placa em PETG de $240 \times 160 \times 3,0\text{ mm}$ com canal rebaixado de $3,0 \times 2,0\text{ mm}$ para gaxeta de silicone (vedação IP65) e recorte de $\varnothing 75\text{ mm}$ para o Cooler de 80 mm.
- **Conjunto de Rodagem:** 2x polias motrizes traseiras de $\varnothing 60\text{ mm}$, 4x polias livres de $\varnothing 50\text{ mm}$ (alojando 8x rolamentos blindados 608-ZZ) e 2x esteiras de pneu MTB vulcanizado ($35\text{ a }40\text{ mm}$ de largura).
