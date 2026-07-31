# Guia rapido para quem esta com os componentes

Este guia resume o primeiro teste. O detalhamento completo esta no
`README.md`.

## Antes de montar

- [ ] Confirmar na serigrafia que o modulo externo e realmente
      **SX1276 para 915 MHz**.
- [ ] Confirmar a identificacao dos pinos: 3V3/VCC, GND, SCK, MISO,
      MOSI, NSS/CS, RESET/RST e DIO0.
- [ ] Conectar uma antena de 915 MHz em cada radio.
- [ ] Nao ligar o SX1276 no pino 3,3 V do Arduino Uno.
- [ ] Nao conectar o circuito analogico do potenciostato no primeiro
      teste.

## Montagem do lado Arduino Uno

1. Ligar 5 V ao lado HV do conversor de nivel.
2. Ligar a saida 3,3 V do AMS1117 ao lado LV e ao VCC do SX1276.
3. Unir todos os GND.
4. Colocar 100 nF e 47 uF entre 3,3 V e GND, perto do SX1276.
5. Fazer as ligacoes:

| Uno | SX1276 |
|---|---|
| D13 | SCK |
| D12 | MISO |
| D11 | MOSI |
| D7 | NSS/CS |
| D8 | RESET/RST |
| D2 | DIO0 |

As linhas passam pelos canais correspondentes do conversor de nivel.

## Gravacao do teste

### Arduino Uno

1. Abrir
   `firmware/01_teste_radio/uno_teste_radio/uno_teste_radio.ino`.
2. Selecionar a placa **Arduino Uno**.
3. Selecionar a porta COM do Uno.
4. Clicar em **Carregar**.

### LILYGO

1. Abrir
   `firmware/01_teste_radio/lilygo_teste_radio/lilygo_teste_radio.ino`.
2. Selecionar a placa **ESP32 Dev Module**.
3. Selecionar a porta COM da LILYGO.
4. Clicar em **Carregar**.
5. Se parar em `Connecting...`, manter BOOT pressionado, tocar RESET e
   soltar BOOT quando a gravacao iniciar.

A biblioteca exigida e **LoRa by Sandeep Mistry**, versao 0.8.0.

## Resultado esperado

Abrir cada monitor serial em 115200 baud.

No Uno deve aparecer:

```text
Enviado: PING,1
ACK recebido; RSSI=...
Resultado: enlace OK.
```

Na LILYGO deve aparecer:

```text
Recebido: PING,1; RSSI=... dBm; SNR=... dB
```

## Se o teste falhar

Enviar para Christian:

1. foto nítida da frente e do verso do novo SX1276;
2. foto geral da montagem;
3. foto aproximada das ligacoes no conversor;
4. tensao medida entre VCC e GND do SX1276;
5. copia completa das mensagens dos dois monitores seriais;
6. nomes das portas COM e da placa selecionada no Arduino IDE.

Nao alterar a pinagem no codigo antes de registrar essas informacoes.

## Depois que PING/ACK funcionar

Gravar:

- Uno:
  `firmware/02_potenciostato_lora/uno_potenciostato_lora/uno_potenciostato_lora.ino`
- LILYGO:
  `firmware/02_potenciostato_lora/lilygo_receptor/lilygo_receptor.ino`

No monitor serial da LILYGO, enviar `S`. O Uno executara dois ciclos,
transmitira os dados e a LILYGO mostrara linhas iniciadas por `META`,
`DATA` e `END`.

Antes de conectar amostras ou eletrodos reais, o conjunto deve ser
testado com uma entrada conhecida e acompanhado por quem conhece o
circuito analogico do potenciostato.
