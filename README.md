# Potenciostato monitorado por LoRa

Primeira versao do enlace ponto a ponto entre:

- **ponta de aquisicao:** Arduino Uno + potenciostato + modulo externo
  SX1276 de 915 MHz;
- **estacao receptora:** LILYGO T3 V1.6.1 / LoRa32 V2.1.6, com ESP32 e
  SX1276 integrado.

O firmware foi preparado para testes progressivos. Primeiro deve ser
validado apenas o enlace de radio. A integracao com o circuito analogico
do potenciostato vem depois.

## O que veio do trabalho da Vitoria

A rotina de varredura usa como referencia o codigo do Apendice A da
dissertacao de Vitoria Costa Sombra (UFABC, 2025):

- PWM no pino D10;
- leitura do ADC no pino A0;
- faixa PWM de 184 ate 41 e retorno de 41 ate 184;
- valor neutro 127;
- dois ciclos;
- taxa inicial de 100 mV/s.

O codigo publicado nao e usado sem alteracoes. Esta versao corrige o
acesso fora do vetor (`pos <= count`), retira a transmissao serial do
trecho critico da varredura, armazena um ciclo em memoria e transmite os
dados depois da aquisicao. A transcricao de referencia esta em
`reference/codigo_vitoria_apendice_A.txt`.

## Requisitos

- Arduino IDE 2;
- pacote **Arduino AVR Boards**;
- pacote **esp32 by Espressif Systems**;
- biblioteca **LoRa by Sandeep Mistry**, versao 0.8.0;
- Python 3 e `pyserial` somente para gravar os dados em CSV.

Instalacao do logger:

```powershell
python -m pip install -r requirements.txt
```

## Ligacoes do Arduino Uno ao SX1276

Esta pinagem e a premissa da primeira versao. A serigrafia do novo
modulo SX1276 deve ser conferida antes da montagem.

| Arduino Uno | Conversor de nivel | SX1276 | Funcao |
|---|---|---|---|
| D13 | HV -> LV | SCK | clock SPI |
| D12 | HV <-> LV | MISO | dados do radio para o Uno |
| D11 | HV -> LV | MOSI | dados do Uno para o radio |
| D7 | HV -> LV | NSS/CS | selecao do radio |
| D8 | HV -> LV | RESET/RST | reinicio do radio |
| D2 | HV <-> LV | DIO0 | interrupcao/estado do radio |
| GND | GND comum | GND | referencia eletrica |

Alimentacao:

1. 5 V alimenta a entrada do regulador AMS1117.
2. A saida de 3,3 V do regulador alimenta o SX1276 e o lado LV do
   conversor.
3. O lado HV do conversor recebe 5 V.
4. Todos os GND devem ser comuns.
5. Colocar 100 nF em paralelo com 47 uF entre 3,3 V e GND, fisicamente
   proximos ao radio.

**Nao alimentar o SX1276 pelo pino 3,3 V do Arduino Uno.**

O conversor bidirecional de MOSFET pode limitar o SPI. Por isso o
firmware do Uno comeca em 1 MHz. Se o radio nao inicializar mesmo com
alimentacao e fios corretos, o conversor deve ser investigado e,
eventualmente, substituido por um tradutor adequado a sinais SPI
push-pull.

## LILYGO

Na LILYGO o radio ja esta conectado internamente:

| Sinal | GPIO |
|---|---:|
| SCK | 5 |
| MISO | 19 |
| MOSI | 27 |
| NSS/CS | 18 |
| RESET | 23 |
| DIO0 | 26 |
| LED da placa | 25 |

Para o primeiro teste, conectar somente:

- antena LoRa de 915 MHz;
- cabo USB ao computador.

Nao e necessario usar AMS1117 ou conversor de nivel na LILYGO.

## Regra de seguranca do radio

**Conectar as duas antenas antes de energizar as placas ou transmitir.**

Comecar os testes com as placas separadas por alguns metros. Nao
transmitir com uma antena ausente, solta ou especificada para outra
faixa.

## Etapa 1 - teste somente do enlace LoRa

Gravar:

- Arduino Uno:
  `firmware/01_teste_radio/uno_teste_radio/uno_teste_radio.ino`
- LILYGO:
  `firmware/01_teste_radio/lilygo_teste_radio/lilygo_teste_radio.ino`

No Arduino IDE:

1. Selecionar **Arduino Uno** para o transmissor.
2. Selecionar **ESP32 Dev Module** para a LILYGO.
3. Gravar cada sketch na placa correspondente.
4. Abrir os dois monitores seriais em **115200 baud**.
5. O Uno deve mostrar `ACK recebido` e a LILYGO deve mostrar os PINGs,
   RSSI e SNR.

Se a LILYGO nao entrar no modo de gravacao automaticamente, manter
**BOOT** pressionado, tocar **RESET**, iniciar a gravacao e soltar BOOT
quando aparecer `Connecting...`. Retirar qualquer cartao microSD antes
da gravacao.

## Etapa 2 - potenciostato com transmissao LoRa

Somente depois da Etapa 1:

- Arduino Uno:
  `firmware/02_potenciostato_lora/uno_potenciostato_lora/uno_potenciostato_lora.ino`
- LILYGO:
  `firmware/02_potenciostato_lora/lilygo_receptor/lilygo_receptor.ino`

Procedimento:

1. Gravar os dois firmwares.
2. Manter inicialmente a entrada A0 em uma condicao de teste conhecida;
   nao iniciar com uma amostra ambiental.
3. Abrir o monitor serial da LILYGO em 115200 baud.
4. Enviar a letra `S`.
5. A LILYGO transmite o comando remoto de inicio.
6. O Uno adquire um ciclo completo em memoria e somente depois envia os
   blocos.
7. A LILYGO imprime as linhas `DATA` em CSV.

O protocolo inclui:

- identificador de sessao;
- sequencia;
- metadados da varredura;
- blocos de ate 16 amostras;
- ACK e ate tres tentativas;
- deteccao de duplicatas;
- CRC-16 do conjunto de amostras, calculado nas duas pontas e conferido
  ao fim de cada ciclo;
- RSSI e SNR medidos pela receptora.

Esta versao ainda nao implementa criptografia, bateria, energia solar,
LoRaWAN ou envio para nuvem.

## Gravar os dados em CSV

Descobrir a porta COM da LILYGO e executar:

```powershell
python tools/serial_logger.py --port COM5 --start
```

O programa cria:

- um log bruto com todas as mensagens;
- um CSV somente com as amostras recebidas.

Os arquivos sao gravados em `data/`, que nao e enviado ao GitHub por
padrao.

## Como compartilhar com o colega

Como o repositorio e privado, ha duas opcoes:

1. **Recomendada:** no GitHub, abrir `Settings > Collaborators`, convidar
   o usuario GitHub do colega e pedir que ele use **Code > Download ZIP**
   ou clone o repositorio.
2. Gerar um ZIP da pasta `firmware`, `tools`, `README.md` e
   `requirements.txt` e enviar por Drive, e-mail ou mensageiro.

O colega nao precisa de Codex. Ele precisa apenas do Arduino IDE, dos
pacotes de placas, da biblioteca LoRa e, opcionalmente, do Python para
o logger.

## Estado de validacao

Os sketches sao compilados automaticamente neste projeto para Arduino
Uno e ESP32 Dev Module. A validacao eletrica e por radio depende do
hardware fisico e deve seguir a ordem de testes acima. A pinagem do
modulo SX1276 externo deve ser confirmada quando a peca substituta
chegar.

## Fontes tecnicas principais

- SOMBRA, V. C. Dissertacao, Apendice A, UFABC, 2025.
- Arduino Uno R3 - documentacao e pinout oficiais.
- Semtech SX1276 - datasheet.
- LILYGO T3 V1.6.1 / LoRa32 V2.1.6 - documentacao oficial.
- Biblioteca Arduino LoRa, de Sandeep Mistry.
