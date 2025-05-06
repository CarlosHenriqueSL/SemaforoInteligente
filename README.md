# Semáforo inteligente com a Raspberry Pi Pico W**
**EMBARCATECH - Fase 2**

**Sistema de Semáforo Inteligente usando FreeRTOS e a BitDogLab**

## Desenvolvedor
- **Carlos Henrique Silva Lopes**

## 🔗 **Link do vídeo de Ensaio**


---

### 📄 Descrição

Este é um projeto que simula um semáforo real em uma placa **BitDogLab (RP2040)**, utilizando o **FreeRTOS** para gerenciar tarefas concorrentes. O sistema opera em dois modos:

* **Modo Normal:** Ciclo clássico de semáforo (verde, amarelo, vermelho) com contagem regressiva na matriz de LEDs, desenho dinâmico no display SSD1306 e sinal sonoro via buzzer.
* **Modo Noturno:** Apenas a luz amarela (LED RGB e matriz de LEDs) pisca suavemente com PWM, e o buzzer emite um beep intermitente a cada 2 s,
o display mostra apenas a luz do centro.

A troca de modos é feita pressionando o **Botão A** da BitDogLab, com debounce implementado.

---

### 🎯 Objetivo Geral

Utilizar tarefas do **FreeRTOS** para controlar LEDs, buzzer, matriz WS2812 e display SSD1306, simulando um semáforo inteligente em tempo real.

---

### ⚙️ Funcionalidades

* **Controle de LEDs RGB:** Verde (6 s), amarelo (3 s), vermelho (6 s) no modo normal; brilho oscilante no modo noturno.
* **Matriz WS2812:** Exibe contagem regressiva (6→1 para verde e vermelho; 3→1 para amarelo) e simula PWM para modo noturno.
* **Display SSD1306:** Desenha semáforo estático que muda de luz conforme o estado atual.
* **Buzzer *(PWM)*:** Sons distintos para cada cor no modo normal; beep lento a cada 2 s no modo noturno.
* **Debounce do Botão A:** Garantia de alternância de modo estável.

---

### 📌 Mapeamento de Pinos

| Função             | GPIO |
| ------------------ | ---- |
| WS2812 (Matriz)    | 7    |
| Botão A            | 5    |
| LED Verde (PWM)    | 11   |
| LED Vermelho (PWM) | 13   |
| Buzzer (PWM)       | 21   |

---

### 🛠️ Estrutura do Código

* **vTaskRGB**: Gerencia LEDs RGB via PWM.
* **vTaskMatriz**: Controla matriz WS2812 e desenha números com PIO.
* **vTaskDisplay**: Inicializa e atualiza o SSD1306 via I2C.
* **vTaskBuzzer**: Emite sons com PWM conforme o modo e cor.
* **vTaskBotao**: Lê o botão A, implementa debounce e alterna `modoNormalOn`.

Cada tarefa roda com `configMINIMAL_STACK_SIZE` e prioridade `tskIDLE_PRIORITY`.

---

## Estrutura do Código

### Principais Arquivos
- **`SemaforoInteligente.c`**: Contém a lógica principal do programa, com todas as tarefas.
    de LED e para configuração do FreeRTOS.
- **`lib/`**: Contém os arquivos com a lógica principal para desenhar no display ssd1306, também contém os arquivos com os desenhos da matriz
    de LED e para configuração do FreeRTOS.
- **`lib/numeros.c`**:  Contém os desenhos dos números que serão feitos na matriz de LEDs.
- **`lib/ssd1306.c`**: Contém as funções para desenhar no display ssd1306.
- **`blink.pio`**: Contém a configuração em Assembly para funcionamento do pio.
- **`README.md`**: Documentação detalhada do projeto.
