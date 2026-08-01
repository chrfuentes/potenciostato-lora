# Guia do diagrama no Fritzing 1.0.7

## Conjunto de figuras recomendado

1. `01_arquitetura_geral`: diagrama em blocos com as duas pontas e o
   enlace LoRa, sem representar o radio como um fio.
2. `02_ligacoes_uno_sx1276`: vista Breadboard detalhada da montagem do
   Arduino Uno, regulador, HW-209, capacitores e SX1276.
3. `03_estacao_lilygo`: figura simples da LILYGO com antena de 915 MHz e
   cabo USB para o computador.

A segunda figura e a mais importante para a montagem fisica. A LILYGO
nao necessita de um diagrama eletrico complexo porque ESP32 e SX1276 ja
estao ligados internamente na placa.

## Nao usar as pecas antigas como referencia final

- O modulo fotografado anteriormente possui SX1278 e indicacao para
  433 MHz; ele nao representa o SX1276 de 915 MHz substituto.
- O primeiro modulo fotografado como regulador tambem foi devolvido.
- Ate receber fotos nitidas dos substitutos, representar esses dois
  componentes como placeholders e identificar os pinos por funcao.

## Vista recomendada

Usar a vista **Breadboard** para a figura de montagem. A vista
**Schematic** pode ser limpa depois e exportada como uma figura eletrica
mais formal. Nao usar a vista PCB nesta fase, pois ainda nao esta sendo
projetada uma placa de circuito impresso.

## Disposicao visual

Da esquerda para a direita:

1. Arduino Uno;
2. dominio de 5 V;
3. conversor HW-209 no centro;
4. dominio de 3,3 V;
5. modulo SX1276 e antena na direita.

Colocar o regulador acima do HW-209 e os capacitores imediatamente ao
lado do pino VCC do SX1276. Adicionar notas `DOMINIO 5 V` e
`DOMINIO 3,3 V` para deixar a fronteira eletrica evidente.

## Pecas no Fritzing

- Arduino Uno R3: procurar na biblioteca principal.
- Breadboard: usar uma placa de tamanho suficiente para os modulos.
- HW-209: usar temporariamente `Mystery Part` com 24 pinos ou uma peca
  generica equivalente e renomear os conectores.
- Regulador 5 V para 3,3 V: usar `Mystery Part` de 3 pinos, com
  `IN`, `OUT 3V3` e `GND`, ate a chegada do substituto.
- SX1276: usar um placeholder com os oito conectores utilizados:
  `VCC`, `GND`, `SCK`, `MISO`, `MOSI`, `NSS`, `RESET` e `DIO0`.
- Capacitor ceramico de 100 nF: peca sem polaridade.
- Capacitor eletrolitico de 47 uF: observar os terminais positivo e
  negativo.

Para criar ou alterar uma peca generica, selecionar a peca, ajustar o
numero de pinos no Inspector e usar `Part > Edit` para nomear os
conectores. Salvar como nova peca na biblioteca `Mine`.

## Alimentacao

| Origem | Destino |
|---|---|
| Uno 5V | entrada do regulador |
| Uno 5V | HW-209 VCCB |
| regulador OUT 3V3 | SX1276 VCC |
| regulador OUT 3V3 | HW-209 VCCA |
| GND do Uno | regulador GND, HW-209 GND e SX1276 GND |

Os dois pinos VCCA do HW-209 pertencem ao mesmo dominio de 3,3 V e os
dois VCCB ao mesmo dominio de 5 V. Na figura, basta alimentar um de cada,
desde que a peca represente internamente essa duplicacao. Para evitar
ambiguidade visual, pode-se ligar os dois pares.

## Sinais digitais

| Arduino Uno | HW-209 B (5 V) | HW-209 A (3,3 V) | SX1276 |
|---|---|---|---|
| D13 | B0 | A0 | SCK |
| D12 | B1 | A1 | MISO |
| D11 | B2 | A2 | MOSI |
| D7 | B3 | A3 | NSS/CS |
| D8 | B4 | A4 | RESET/RST |
| D2 | B5 | A5 | DIO0 |

A6/B6 e A7/B7 ficam livres.

## Capacitores

Os capacitores de 100 nF e 47 uF ficam em paralelo:

- um terminal de cada capacitor em 3,3 V;
- o outro terminal em GND;
- no eletrolitico, `+` em 3,3 V e `-` em GND.

Eles devem aparecer proximos ao SX1276, pois a funcao e estabilizar a
alimentacao do radio durante os picos de corrente.

## Cores sugeridas

| Cor | Sinal |
|---|---|
| vermelho | 5 V |
| laranja | 3,3 V |
| preto | GND |
| verde | SCK |
| azul | MISO |
| amarelo | MOSI |
| roxo | NSS/CS |
| cinza | RESET |
| branco | DIO0 |

Usar pontos de dobra para evitar cruzamentos e manter os fios de cada
canal alinhados horizontalmente ao atravessar o HW-209.

## Figura da LILYGO

Representar somente:

- placa LILYGO T3 V1.6.1 / LoRa32 V2.1.6;
- antena LoRa de 915 MHz no conector SMA;
- cabo USB ligado ao computador;
- nota: `ESP32 + SX1276 integrados; sem conversor de nivel externo`.

Nao desenhar AMS1117 ou HW-209 nessa ponta.

## Conferencia antes da exportacao

- confirmar que nao existe fio entre as duas pontas LoRa;
- confirmar VCCA = 3,3 V e VCCB = 5 V;
- confirmar D10 reservado para o PWM do potenciostato;
- confirmar A0 reservado para a leitura analogica;
- confirmar NSS em D7, RESET em D8 e DIO0 em D2;
- confirmar polaridade do capacitor de 47 uF;
- escrever `ANTENA 915 MHz - conectar antes de energizar`.

Exportar a vista em **SVG** para edicao e alta qualidade, e em **PNG**
para consulta rapida. Manter tambem o arquivo `.fzz` original.

Documentacao oficial do Fritzing:

- https://fritzing.org/learning/tutorials/building-circuit
- https://fritzing.org/learning/tutorials/creating-custom-parts/using-generic-parts/
- https://fritzing.org/learning/tutorials/creating-custom-parts/using-sharing
