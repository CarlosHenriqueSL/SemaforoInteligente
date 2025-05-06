#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "blink.pio.h"
#include "ssd1306.h"
#include "numeros.h"

// Definicao dos pinos
#define WS2812_PIN 7

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO 0x3C

#define BUTTON_A 5
#define LED_PIN_GREEN 11
#define LED_PIN_RED 13

#define DEBOUNCE_MS 400

#define BUZZER_PIN 21

// Flag para alternar o modo. True = modo normal, false = modo noturno.
volatile bool modoNormalOn = true;

/* Tarefa para tocar o buzzer com pwm */
void vTaskBuzzer()
{
    uint slice = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_enabled(slice, true);
    uint chan = pwm_gpio_to_channel(BUZZER_PIN);
    uint wrap = 125000000 / 3500;
    pwm_set_wrap(slice, wrap);

    while (true)
    {
        if (modoNormalOn)
        {
            wrap = 125000000 / 3500;

            // 1 beep curto por um segundo no sinal verde
            for (int i = 0; i < 6 && modoNormalOn; i += 1)
            {
                pwm_set_chan_level(slice, chan, wrap / 2);
                for (int i = 0; i < 10; i++)
                {
                    vTaskDelay(pdMS_TO_TICKS(100));
                }               
                pwm_set_chan_level(slice, chan, 0);
                wrap = 0;
            }

            wrap = 125000000 / 4000;

            // Beep rapido intermitente para o sinal amarelo
            for (int i = 0; i < 6 && modoNormalOn; i += 1)
            {
                pwm_set_chan_level(slice, chan, wrap / 2);
                vTaskDelay(pdMS_TO_TICKS(250));
                pwm_set_chan_level(slice, chan, 0);
                vTaskDelay(pdMS_TO_TICKS(250));
            }

            wrap = 125000000 / 2000;

            // Tom contínuo curto para o sinal vermelho
            for (int i = 0; i < 3 && modoNormalOn; ++i)
            {
                pwm_set_chan_level(slice, chan, wrap / 2);
                vTaskDelay(pdMS_TO_TICKS(500));
                pwm_set_chan_level(slice, chan, 0);
                vTaskDelay(pdMS_TO_TICKS(1500));
            }            
        } else
        {
            // Beep lento a cada 2s
            pwm_set_chan_level(slice, chan, wrap / 2);
            vTaskDelay(pdMS_TO_TICKS(100));  
            pwm_set_chan_level(slice, chan, 0);
            vTaskDelay(pdMS_TO_TICKS(1900)); 
        }
    }
}

/* Tarefa para alternar entre os LEDs RGB com PWM */
void vTaskRGB()
{
    while (true)
    {
        pwm_set_gpio_level(LED_PIN_GREEN, 0);
        pwm_set_gpio_level(LED_PIN_RED, 0);
        if (modoNormalOn)
        {
            pwm_set_gpio_level(LED_PIN_GREEN, 255); // Verde, por 6 segundos

            /* O for com varios delays eh necessario para alternar o modo da tarefa enquanto esse bloco
               ainda esta em execucao. */
            for (int i = 0; i < 6000 && modoNormalOn; i += 100)
                vTaskDelay(pdMS_TO_TICKS(100));

            pwm_set_gpio_level(LED_PIN_RED, 255); // Amarelo, por 3 segundos
            for (int i = 0; i < 3000 && modoNormalOn; i += 100)
                vTaskDelay(pdMS_TO_TICKS(100));

            pwm_set_gpio_level(LED_PIN_GREEN, 0); // Vermelho, por 6 segundos
            for (int i = 0; i < 6000 && modoNormalOn; i += 100)
                vTaskDelay(pdMS_TO_TICKS(100));

            pwm_set_gpio_level(LED_PIN_RED, 0); // Desliga o LED vermelho, reinicia o ciclo
        }
        else
        {
            // Modo noturno, luz amarela alternando com PMW lentamente
            for (uint brightness = 0; brightness <= 255 && !modoNormalOn; brightness++)
            {
                pwm_set_gpio_level(LED_PIN_GREEN, brightness);
                pwm_set_gpio_level(LED_PIN_RED, brightness);
                vTaskDelay(pdMS_TO_TICKS(5));
            }
            for (int brightness = 255; brightness >= 0 && !modoNormalOn; brightness--)
            {
                pwm_set_gpio_level(LED_PIN_GREEN, brightness);
                pwm_set_gpio_level(LED_PIN_RED, brightness);
                vTaskDelay(pdMS_TO_TICKS(5));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/*
 * Tarefa para a matriz de LEDs. No modo normal, inicia uma contagem regressiva com a luz de cada LED RGB:
 * 6 ate 1 para o LED verde, 3 ate 1 para o LED amarelo e 6 ate 1 para o LED vermelho, o ciclo reinicia
 * junto com a tarefa dos LEDs RGB. Para o modo noturno, ele simula um PWM com luz amarela junto com o RGB
 * amarelo.
 */
void vTaskMatriz()
{
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &blink_program);
    uint sm = pio_claim_unused_sm(pio, true);
    blink_program_init(pio, sm, offset, WS2812_PIN);

    // Vetor com os numeros que serao exibidos na matriz. Todos eles estao desenhados em lib/numeros.c
    double *numeros[6] = {numero1, numero2, numero3, numero4, numero5, numero6};

    // Numero que sera exibido no momento
    volatile static uint current_numero;

    while (true)
    {
        if (modoNormalOn)
        {
            current_numero = 5;

            // Desenha um 6 na cor verde por 1 segundo
            for (int i = 0; i < NUM_PIXELS; i++)
            {
                double r = 0.0;
                double g = numeros[current_numero][24 - i];
                double b = 0.0;
                unsigned char R = r * 255;
                unsigned char G = g * 255;
                unsigned char B = b * 255;
                uint32_t valor_led = (G << 24) | (R << 16) | (B << 8);
                pio_sm_put_blocking(pio, sm, valor_led);
            }

            for (int i = 0; i < 10 && modoNormalOn; i += 1)
                vTaskDelay(pdMS_TO_TICKS(100));

            current_numero--;

            // Desenha um 5 na cor verde por 1 segundo
            for (int i = 0; i < NUM_PIXELS; i++)
            {
                double r = 0.0;
                double g = numeros[current_numero][24 - i];
                double b = 0.0;
                unsigned char R = r * 255;
                unsigned char G = g * 255;
                unsigned char B = b * 255;
                uint32_t valor_led = (G << 24) | (R << 16) | (B << 8);
                pio_sm_put_blocking(pio, sm, valor_led);
            }

            for (int i = 0; i < 10 && modoNormalOn; i += 1)
                vTaskDelay(pdMS_TO_TICKS(100));

            current_numero--;

            // Desenha um 4 na cor verde por 1 segundo
            for (int i = 0; i < NUM_PIXELS; i++)
            {
                double r = 0.0;
                double g = numeros[current_numero][24 - i];
                double b = 0.0;
                unsigned char R = r * 255;
                unsigned char G = g * 255;
                unsigned char B = b * 255;
                uint32_t valor_led = (G << 24) | (R << 16) | (B << 8);
                pio_sm_put_blocking(pio, sm, valor_led);
            }

            for (int i = 0; i < 10 && modoNormalOn; i += 1)
                vTaskDelay(pdMS_TO_TICKS(100));

            current_numero--;

            // Desenha um 3 na cor verde por 1 segundo
            for (int i = 0; i < NUM_PIXELS; i++)
            {
                double r = 0.0;
                double g = numeros[current_numero][24 - i];
                double b = 0.0;
                unsigned char R = r * 255;
                unsigned char G = g * 255;
                unsigned char B = b * 255;
                uint32_t valor_led = (G << 24) | (R << 16) | (B << 8);
                pio_sm_put_blocking(pio, sm, valor_led);
            }

            for (int i = 0; i < 10 && modoNormalOn; i += 1)
                vTaskDelay(pdMS_TO_TICKS(100));

            current_numero--;

            // Desenha um 2 na cor verde por 1 segundo
            for (int i = 0; i < NUM_PIXELS; i++)
            {
                double r = 0.0;
                double g = numeros[current_numero][24 - i];
                double b = 0.0;
                unsigned char R = r * 255;
                unsigned char G = g * 255;
                unsigned char B = b * 255;
                uint32_t valor_led = (G << 24) | (R << 16) | (B << 8);
                pio_sm_put_blocking(pio, sm, valor_led);
            }

            for (int i = 0; i < 10 && modoNormalOn; i += 1)
                vTaskDelay(pdMS_TO_TICKS(100));

            current_numero--;

            // Desenha um 1 na cor verde por 1 segundo
            for (int i = 0; i < NUM_PIXELS; i++)
            {
                double r = 0.0;
                double g = numeros[current_numero][24 - i];
                double b = 0.0;
                unsigned char R = r * 255;
                unsigned char G = g * 255;
                unsigned char B = b * 255;
                uint32_t valor_led = (G << 24) | (R << 16) | (B << 8);
                pio_sm_put_blocking(pio, sm, valor_led);
            }

            for (int i = 0; i < 10 && modoNormalOn; i += 1)
                vTaskDelay(pdMS_TO_TICKS(100));

            current_numero = 2;

            // Desenha um 3 na cor amarela por 1 segundo
            for (int i = 0; i < NUM_PIXELS; i++)
            {
                double r = numeros[current_numero][24 - i];
                double g = numeros[current_numero][24 - i];
                double b = 0.0;
                unsigned char R = r * 255;
                unsigned char G = g * 255;
                unsigned char B = b * 255;
                uint32_t valor_led = (G << 24) | (R << 16) | (B << 8);
                pio_sm_put_blocking(pio, sm, valor_led);
            }

            for (int i = 0; i < 10 && modoNormalOn; i += 1)
                vTaskDelay(pdMS_TO_TICKS(100));

            current_numero--;

            // Desenha um 2 na cor amarela por 1 segundo
            for (int i = 0; i < NUM_PIXELS; i++)
            {
                double r = numeros[current_numero][24 - i];
                double g = numeros[current_numero][24 - i];
                double b = 0.0;
                unsigned char R = r * 255;
                unsigned char G = g * 255;
                unsigned char B = b * 255;
                uint32_t valor_led = (G << 24) | (R << 16) | (B << 8);
                pio_sm_put_blocking(pio, sm, valor_led);
            }

            for (int i = 0; i < 10 && modoNormalOn; i += 1)
                vTaskDelay(pdMS_TO_TICKS(100));

            current_numero--;

            // Desenha um 1 na cor amarela por 1 segundo
            for (int i = 0; i < NUM_PIXELS; i++)
            {
                double r = numeros[current_numero][24 - i];
                double g = numeros[current_numero][24 - i];
                double b = 0.0;
                unsigned char R = r * 255;
                unsigned char G = g * 255;
                unsigned char B = b * 255;
                uint32_t valor_led = (G << 24) | (R << 16) | (B << 8);
                pio_sm_put_blocking(pio, sm, valor_led);
            }

            for (int i = 0; i < 10 && modoNormalOn; i += 1)
                vTaskDelay(pdMS_TO_TICKS(100));

            current_numero = 5;

            // Desenha um 6 na cor vermelha por 1 segundo
            for (int i = 0; i < NUM_PIXELS; i++)
            {
                double r = numeros[current_numero][24 - i];
                double g = 0.0;
                double b = 0.0;
                unsigned char R = r * 255;
                unsigned char G = g * 255;
                unsigned char B = b * 255;
                uint32_t valor_led = (G << 24) | (R << 16) | (B << 8);
                pio_sm_put_blocking(pio, sm, valor_led);
            }

            for (int i = 0; i < 10 && modoNormalOn; i += 1)
                vTaskDelay(pdMS_TO_TICKS(100));

            current_numero--;

            // Desenha um 5 na cor vermelha por 1 segundo
            for (int i = 0; i < NUM_PIXELS; i++)
            {
                double r = numeros[current_numero][24 - i];
                double g = 0.0;
                double b = 0.0;
                unsigned char R = r * 255;
                unsigned char G = g * 255;
                unsigned char B = b * 255;
                uint32_t valor_led = (G << 24) | (R << 16) | (B << 8);
                pio_sm_put_blocking(pio, sm, valor_led);
            }

            for (int i = 0; i < 10 && modoNormalOn; i += 1)
                vTaskDelay(pdMS_TO_TICKS(100));

            current_numero--;

            // Desenha um 4 na cor vermelha por 1 segundo
            for (int i = 0; i < NUM_PIXELS; i++)
            {
                double r = numeros[current_numero][24 - i];
                double g = 0.0;
                double b = 0.0;
                unsigned char R = r * 255;
                unsigned char G = g * 255;
                unsigned char B = b * 255;
                uint32_t valor_led = (G << 24) | (R << 16) | (B << 8);
                pio_sm_put_blocking(pio, sm, valor_led);
            }

            for (int i = 0; i < 10 && modoNormalOn; i += 1)
                vTaskDelay(pdMS_TO_TICKS(100));

            current_numero--;

            // Desenha um 3 na cor vermelha por 1 segundo
            for (int i = 0; i < NUM_PIXELS; i++)
            {
                double r = numeros[current_numero][24 - i];
                double g = 0.0;
                double b = 0.0;
                unsigned char R = r * 255;
                unsigned char G = g * 255;
                unsigned char B = b * 255;
                uint32_t valor_led = (G << 24) | (R << 16) | (B << 8);
                pio_sm_put_blocking(pio, sm, valor_led);
            }

            for (int i = 0; i < 10 && modoNormalOn; i += 1)
                vTaskDelay(pdMS_TO_TICKS(100));

            current_numero--;

            // Desenha um 2 na cor vermelha por 1 segundo
            for (int i = 0; i < NUM_PIXELS; i++)
            {
                double r = numeros[current_numero][24 - i];
                double g = 0.0;
                double b = 0.0;
                unsigned char R = r * 255;
                unsigned char G = g * 255;
                unsigned char B = b * 255;
                uint32_t valor_led = (G << 24) | (R << 16) | (B << 8);
                pio_sm_put_blocking(pio, sm, valor_led);
            }

            for (int i = 0; i < 10 && modoNormalOn; i += 1)
                vTaskDelay(pdMS_TO_TICKS(100));

            current_numero--;

            // Desenha um 1 na cor vermelha por 1 segundo
            for (int i = 0; i < NUM_PIXELS; i++)
            {
                double r = numeros[current_numero][24 - i];
                double g = 0.0;
                double b = 0.0;
                unsigned char R = r * 255;
                unsigned char G = g * 255;
                unsigned char B = b * 255;
                uint32_t valor_led = (G << 24) | (R << 16) | (B << 8);
                pio_sm_put_blocking(pio, sm, valor_led);
            }

            for (int i = 0; i < 10 && modoNormalOn; i += 1)
                vTaskDelay(pdMS_TO_TICKS(100));
        }
        else
        {
            // Todos os LEDs da matriz brilhando lentamente, simulando o PWM do LED RGB.
            for (float brightness = 0.0; brightness <= 1.0 && !modoNormalOn; brightness += 0.05)
            {
                for (int i = 0; i < NUM_PIXELS; i++)
                {
                    double intensity = modoNoturno[24 - i];
                    unsigned char R = (unsigned char)(intensity * brightness * 255);
                    unsigned char G = (unsigned char)(intensity * brightness * 255);
                    unsigned char B = 0;
                    uint32_t valor_led = (G << 24) | (R << 16) | (B << 8);
                    pio_sm_put_blocking(pio, sm, valor_led);
                }
                vTaskDelay(pdMS_TO_TICKS(63.75));
            }
            for (float brightness = 1.0; brightness >= 0.0 && !modoNormalOn; brightness -= 0.05)
            {
                for (int i = 0; i < NUM_PIXELS; i++)
                {
                    double intensity = modoNoturno[24 - i];
                    unsigned char R = (unsigned char)(intensity * brightness * 255);
                    unsigned char G = (unsigned char)(intensity * brightness * 255);
                    unsigned char B = 0;
                    uint32_t valor_led = (G << 24) | (R << 16) | (B << 8);
                    pio_sm_put_blocking(pio, sm, valor_led);
                }
                vTaskDelay(pdMS_TO_TICKS(63.75));
            }
        }
    }
}

/* Tarefa do botao A, alterna entre o modo normal e o modo noturno.*/
void vTaskBotao()
{
    bool estadoAnterior = true;

    while (true)
    {
        bool estadoAtual = gpio_get(BUTTON_A); // Lê o estado atual do botão

        // Detecta flanco de descida: de 1 para 0
        if (estadoAnterior && !estadoAtual)
        {
            modoNormalOn = !modoNormalOn;           // Alterna o modo
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS)); // Debounce
        }

        estadoAnterior = estadoAtual;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/*
 * Desenha um semaforo no display SSD1306. No modo normal, inicia uma contagem regressiva com a luz
 * de cada LED RGB: 6 ate 1 para o LED verde, 3 ate 1 para o LED amarelo e 6 ate 1 para o LED vermelho,
 * apos isso, o ciclo reinicia junto com a tarefa dos LEDs RGB. Para o modo noturno, ele simula um PWM
 * com luz amarela junto com o RGB amarelo no modo normal, as luzes do semaforo alternam junto com os
 * LEDs RGB: verde, amarelo e vermelho. Para o modo noturno, apenas a luz amarela. O display nao tem
 * cores de fato, eh apenas uma simulacao das posicoes das luzes de um semaforo real.
 */
void vTaskDisplay()
{
    // Configuracao do I2C para o display
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    bool cor = true;
    while (true)
    {
        ssd1306_fill(&ssd, !cor);

        // Desenha a estrutura do semaforo
        ssd1306_line(&ssd, 10, 58, 119, 58, cor);
        ssd1306_line(&ssd, 9, 57, 120, 57, cor);

        for (int i = 0; i < 8; i++)
        {
            ssd1306_line(&ssd, 8, 56 - i, 121, 56 - i, cor);
        }
        for (int i = 0; i < 7; i++)
        {
            ssd1306_line(&ssd, 8, 48 - i, 24 - i, 48 - i, cor);
            ssd1306_line(&ssd, 38 + i, 48 - i, 58 - i, 48 - i, cor);
            ssd1306_line(&ssd, 72 + i, 48 - i, 92 - i, 48 - i, cor);
            ssd1306_line(&ssd, 106 + i, 48 - i, 121, 48 - i, cor);
        }
        for (int i = 0; i < 11; i++)
        {
            ssd1306_line(&ssd, 8, 41 - i, 18, 41 - i, cor);
            ssd1306_line(&ssd, 44, 41 - i, 52, 41 - i, cor);
            ssd1306_line(&ssd, 78, 41 - i, 86, 41 - i, cor);
            ssd1306_line(&ssd, 112, 41 - i, 121, 41 - i, cor);
        }
        for (int i = 0; i < 6; i++)
        {
            ssd1306_line(&ssd, 8, 30 - i, 19 + i, 30 - i, cor);
            ssd1306_line(&ssd, 43 - i, 30 - i, 53 + i, 30 - i, cor);
            ssd1306_line(&ssd, 77 - i, 30 - i, 87 + i, 30 - i, cor);
            ssd1306_line(&ssd, 111 - i, 30 - i, 121, 30 - i, cor);
        }
        for (int i = 0; i < 9; i++)
        {
            ssd1306_line(&ssd, 8, 24 - i, 121, 24 - i, cor);
        }

        ssd1306_line(&ssd, 9, 15, 120, 15, cor);
        ssd1306_line(&ssd, 10, 14, 119, 14, cor);

        for (int i = 0; i < 3; i++)
        {
            ssd1306_line(&ssd, 28, 13 - i, 43, 13 - i, cor);
        }
        for (int i = 0; i < 8; i++)
        {
            ssd1306_line(&ssd, 31, 8 - i, 40, 8 - i, cor);
        }
        for (int i = 0; i < 3; i++)
        {
            ssd1306_line(&ssd, 86, 13 - i, 101, 13 - i, cor);
        }
        for (int i = 0; i < 8; i++)
        {
            ssd1306_line(&ssd, 89, 8 - i, 98, 8 - i, cor);
        }

        if (modoNormalOn)
        {
            // Desliga a luz amarela do semaforo para caso esteja ligada
            for (int i = 0; i < 5; i++)
            {
                ssd1306_line(&ssd, 61 - i, 44 - i, 69 + i, 44 - i, !cor);
            }
            for (int i = 0; i < 7; i++)
            {
                ssd1306_line(&ssd, 57, 39 - i, 73, 39 - i, !cor);
            }
            for (int i = 0; i < 4; i++)
            {
                ssd1306_line(&ssd, 58 + i, 32 - i, 72 - i, 32 - i, !cor);
            }

            // Liga a luz verde do semaforo por 6 segundos
            for (int i = 0; i < 5; i++)
            {
                ssd1306_line(&ssd, 27 - i, 44 - i, 35 + i, 44 - i, cor);
            }
            for (int i = 0; i < 7; i++)
            {
                ssd1306_line(&ssd, 23, 39 - i, 39, 39 - i, cor);
            }
            for (int i = 0; i < 4; i++)
            {
                ssd1306_line(&ssd, 24 + i, 32 - i, 38 - i, 32 - i, cor);
            }

            ssd1306_send_data(&ssd);

            for (int i = 0; i < 6000 && modoNormalOn; i += 100)
                vTaskDelay(pdMS_TO_TICKS(100));

            // Desliga a luz verde do semaforo
            for (int i = 0; i < 5; i++)
            {
                ssd1306_line(&ssd, 27 - i, 44 - i, 35 + i, 44 - i, !cor);
            }
            for (int i = 0; i < 7; i++)
            {
                ssd1306_line(&ssd, 23, 39 - i, 39, 39 - i, !cor);
            }
            for (int i = 0; i < 4; i++)
            {
                ssd1306_line(&ssd, 24 + i, 32 - i, 38 - i, 32 - i, !cor);
            }

            // Liga a luz amarela do semaforo por 3 segundos
            for (int i = 0; i < 5; i++)
            {
                ssd1306_line(&ssd, 61 - i, 44 - i, 69 + i, 44 - i, cor);
            }
            for (int i = 0; i < 7; i++)
            {
                ssd1306_line(&ssd, 57, 39 - i, 73, 39 - i, cor);
            }
            for (int i = 0; i < 4; i++)
            {
                ssd1306_line(&ssd, 58 + i, 32 - i, 72 - i, 32 - i, cor);
            }

            ssd1306_send_data(&ssd);

            for (int i = 0; i < 3000 && modoNormalOn; i += 100)
                vTaskDelay(pdMS_TO_TICKS(100));

            // Desliga a luz amarela do semaforo
            for (int i = 0; i < 5; i++)
            {
                ssd1306_line(&ssd, 61 - i, 44 - i, 69 + i, 44 - i, !cor);
            }
            for (int i = 0; i < 7; i++)
            {
                ssd1306_line(&ssd, 57, 39 - i, 73, 39 - i, !cor);
            }
            for (int i = 0; i < 4; i++)
            {
                ssd1306_line(&ssd, 58 + i, 32 - i, 72 - i, 32 - i, !cor);
            }

            // Liga a luz vermelha do semaforo por 6 segundos
            for (int i = 0; i < 5; i++)
            {
                ssd1306_line(&ssd, 95 - i, 44 - i, 103 + i, 44 - i, cor);
            }
            for (int i = 0; i < 7; i++)
            {
                ssd1306_line(&ssd, 91, 39 - i, 107, 39 - i, cor);
            }
            ssd1306_line(&ssd, 92, 32, 106, 32, cor);
            ssd1306_line(&ssd, 93, 31, 105, 31, cor);
            ssd1306_line(&ssd, 94, 30, 104, 30, cor);
            ssd1306_line(&ssd, 95, 29, 104, 29, cor);

            ssd1306_send_data(&ssd);

            for (int i = 0; i < 6000 && modoNormalOn; i += 100)
                vTaskDelay(pdMS_TO_TICKS(100));

            // Desliga a luz vermelha do semaforo
            for (int i = 0; i < 5; i++)
            {
                ssd1306_line(&ssd, 95 - i, 44 - i, 103 + i, 44 - i, !cor);
            }
            for (int i = 0; i < 7; i++)
            {
                ssd1306_line(&ssd, 91, 39 - i, 107, 39 - i, !cor);
            }
            ssd1306_line(&ssd, 92, 32, 106, 32, !cor);
            ssd1306_line(&ssd, 93, 31, 105, 31, !cor);
            ssd1306_line(&ssd, 94, 30, 104, 30, !cor);
            ssd1306_line(&ssd, 95, 29, 104, 29, !cor);
        }
        else
        {
            // Deixa ligada apenas a luz amarela do semaforo, isso por que apenas o RGB amarelo esta aceso
            for (int i = 0; i < 5; i++)
            {
                ssd1306_line(&ssd, 61 - i, 44 - i, 69 + i, 44 - i, cor);
            }
            for (int i = 0; i < 7; i++)
            {
                ssd1306_line(&ssd, 57, 39 - i, 73, 39 - i, cor);
            }
            for (int i = 0; i < 4; i++)
            {
                ssd1306_line(&ssd, 58 + i, 32 - i, 72 - i, 32 - i, cor);
            }

            ssd1306_send_data(&ssd);

            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

int main()
{
    stdio_init_all();

    gpio_init(LED_PIN_GREEN);
    gpio_set_dir(LED_PIN_GREEN, GPIO_OUT);
    gpio_init(LED_PIN_RED);
    gpio_set_dir(LED_PIN_RED, GPIO_OUT);

    // Inicializando o PWM para o LED verde, LED vermelho e para o buzzer.
    gpio_set_function(LED_PIN_GREEN, GPIO_FUNC_PWM);
    gpio_set_function(LED_PIN_RED, GPIO_FUNC_PWM);
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_led_red = pwm_gpio_to_slice_num(LED_PIN_RED);
    uint slice_led_green = pwm_gpio_to_slice_num(LED_PIN_GREEN);
    uint slice_buzzer = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_clkdiv(slice_led_green, 500.0f);
    pwm_set_wrap(slice_led_green, 255);
    pwm_set_clkdiv(slice_led_red, 500.0f);
    pwm_set_wrap(slice_led_red, 255);
    pwm_set_enabled(slice_led_green, true);
    pwm_set_enabled(slice_led_red, true);
    pwm_set_enabled(slice_buzzer, true);

    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    // Criacao das tarefas
    xTaskCreate(vTaskRGB, "Tarefa dos LEDs", configMINIMAL_STACK_SIZE,
                NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vTaskBotao, "Tarefa do botao A", configMINIMAL_STACK_SIZE,
                NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vTaskDisplay, "Tarefa do display SSD1306", configMINIMAL_STACK_SIZE,
                NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vTaskMatriz, "Tarefa da matriz de LEDs", configMINIMAL_STACK_SIZE,
                NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vTaskBuzzer, "Tarefa do buzzer", configMINIMAL_STACK_SIZE,
                NULL, tskIDLE_PRIORITY, NULL);
    vTaskStartScheduler();
    panic_unsupported();
}