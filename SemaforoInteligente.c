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

volatile bool modoNormalOn = true;

static TaskHandle_t xTaskRGBHandle = NULL;
static uint32_t last_irq_time_a = 0;

void iniciar_pwm()
{
    gpio_init(LED_PIN_GREEN);
    gpio_set_dir(LED_PIN_GREEN, GPIO_OUT);
    gpio_init(LED_PIN_RED);
    gpio_set_dir(LED_PIN_RED, GPIO_OUT);

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
}

void vTaskBuzzer(uint frequencia, uint duracao_ms)
{
    uint slice = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint wrap = 0 / frequencia;
    pwm_set_wrap(slice, wrap);
    pwm_set_chan_level(slice, pwm_gpio_to_channel(BUZZER_PIN), wrap / 2);
    vTaskDelay(pdMS_TO_TICKS(duracao_ms));
    pwm_set_chan_level(slice, pwm_gpio_to_channel(BUZZER_PIN), 0);
}

void vTaskRGB(void *pvParameters)
{
    while (true)
    {
        pwm_set_gpio_level(LED_PIN_GREEN, 0);
        pwm_set_gpio_level(LED_PIN_RED, 0);
        if (modoNormalOn)
        {
            pwm_set_gpio_level(LED_PIN_GREEN, 255);
            for (int i = 0; i < 6000 && modoNormalOn; i += 100)
                vTaskDelay(pdMS_TO_TICKS(100));

            pwm_set_gpio_level(LED_PIN_RED, 255);
            for (int i = 0; i < 3000 && modoNormalOn; i += 100)
                vTaskDelay(pdMS_TO_TICKS(100));

            pwm_set_gpio_level(LED_PIN_GREEN, 0);
            for (int i = 0; i < 6000 && modoNormalOn; i += 100)
                vTaskDelay(pdMS_TO_TICKS(100));

            pwm_set_gpio_level(LED_PIN_RED, 0);
        }
        else
        {
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
uint32_t matrix_rgb(double r, double g, double b)
{
    unsigned char R = r * 255, G = g * 255, B = b * 255;
    return (G << 24) | (R << 16) | (B << 8);
}

void desenho_pio(double *desenho, uint32_t valor_led, PIO pio, uint sm, double r, double g, double b)
{
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        uint32_t valor_led = matrix_rgb(desenho[24 - i] * r, desenho[24 - i] * g, desenho[24 - i] * b);
        pio_sm_put_blocking(pio, sm, valor_led);
    }
}

void vTaskMatriz(void *pvParameters)
{
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &blink_program);
    uint sm = pio_claim_unused_sm(pio, true);
    blink_program_init(pio, sm, offset, WS2812_PIN);

    double *numeros[10] = {numero1, numero2, numero3, numero4, numero5, numero6};

    volatile static uint current_numero;

    while (true)
    {
        if (modoNormalOn)
        {
            current_numero = 5;            
            desenho_pio(numeros[current_numero], 0, pio, sm, 0.0, 1.0, 0.0);  
            vTaskDelay(pdMS_TO_TICKS(1000));                              
            current_numero--;
            desenho_pio(numeros[current_numero], 0, pio, sm, 0.0, 1.0, 0.0);  
            vTaskDelay(pdMS_TO_TICKS(1000));                              
            current_numero--;
            desenho_pio(numeros[current_numero], 0, pio, sm, 0.0, 1.0, 0.0);  
            vTaskDelay(pdMS_TO_TICKS(1000));                              
            current_numero--;
            desenho_pio(numeros[current_numero], 0, pio, sm, 0.0, 1.0, 0.0);  
            vTaskDelay(pdMS_TO_TICKS(1000));                              
            current_numero--;
            desenho_pio(numeros[current_numero], 0, pio, sm, 0.0, 1.0, 0.0);  
            vTaskDelay(pdMS_TO_TICKS(1000));                              
            current_numero--;
            desenho_pio(numeros[current_numero], 0, pio, sm, 0.0, 1.0, 0.0);  
            vTaskDelay(pdMS_TO_TICKS(1000));                              
            current_numero = 2;   
            desenho_pio(numeros[current_numero], 0, pio, sm, 1.0, 1.0, 0.0);  
            vTaskDelay(pdMS_TO_TICKS(1000));   
            current_numero--;
            desenho_pio(numeros[current_numero], 0, pio, sm, 1.0, 1.0, 0.0);  
            vTaskDelay(pdMS_TO_TICKS(1000));               
            current_numero--;
            desenho_pio(numeros[current_numero], 0, pio, sm, 1.0, 1.0, 0.0);  
            vTaskDelay(pdMS_TO_TICKS(1000));   
            current_numero = 5;
            desenho_pio(numeros[current_numero], 0, pio, sm, 1.0, 0.0, 0.0);  
            vTaskDelay(pdMS_TO_TICKS(1000));                           
            current_numero--;
            desenho_pio(numeros[current_numero], 0, pio, sm, 1.0, 0.0, 0.0);  
            vTaskDelay(pdMS_TO_TICKS(1000));                              
            current_numero--;
            desenho_pio(numeros[current_numero], 0, pio, sm, 1.0, 0.0, 0.0);  
            vTaskDelay(pdMS_TO_TICKS(1000));                              
            current_numero--;
            desenho_pio(numeros[current_numero], 0, pio, sm, 1.0, 0.0, 0.0);  
            vTaskDelay(pdMS_TO_TICKS(1000));                              
            current_numero--;
            desenho_pio(numeros[current_numero], 0, pio, sm, 1.0, 0.0, 0.0);  
            vTaskDelay(pdMS_TO_TICKS(1000));                              
            current_numero--;
            desenho_pio(numeros[current_numero], 0, pio, sm, 1.0, 0.0, 0.0);  
            vTaskDelay(pdMS_TO_TICKS(1000));            
        }
    }   
}

void vTaskBotao(void *pvParameters)
{
    bool estadoAnterior = true; // Começa com true porque o botão tem pull-up (não pressionado)

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
        vTaskDelay(pdMS_TO_TICKS(10)); // Pequeno delay para leitura
    }
}

void vTaskDisplay()
{
    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA);                                        // Pull up the data line
    gpio_pull_up(I2C_SCL);                                        // Pull up the clock line
    ssd1306_t ssd;                                                // Inicializa a estrutura do display
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd);                                         // Configura o display
    ssd1306_send_data(&ssd);                                      // Envia os dados para o display
    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    bool cor = true;
    while (true)
    {
        ssd1306_fill(&ssd, !cor);
        ssd1306_line(&ssd, 10, 58, 119, 58, cor);
        ssd1306_line(&ssd, 9, 57, 120, 57, cor);
        ssd1306_line(&ssd, 8, 56, 121, 56, cor);
        ssd1306_line(&ssd, 8, 55, 121, 55, cor);
        ssd1306_line(&ssd, 8, 54, 121, 54, cor);
        ssd1306_line(&ssd, 8, 53, 121, 53, cor);
        ssd1306_line(&ssd, 8, 52, 121, 52, cor);
        ssd1306_line(&ssd, 8, 51, 121, 51, cor);
        ssd1306_line(&ssd, 8, 50, 121, 50, cor);
        ssd1306_line(&ssd, 8, 49, 121, 49, cor);
        ssd1306_line(&ssd, 8, 48, 24, 48, cor);
        ssd1306_line(&ssd, 38, 48, 58, 48, cor);
        ssd1306_line(&ssd, 72, 48, 92, 48, cor);
        ssd1306_line(&ssd, 106, 48, 121, 48, cor);
        ssd1306_line(&ssd, 8, 47, 23, 47, cor);
        ssd1306_line(&ssd, 39, 47, 57, 47, cor);
        ssd1306_line(&ssd, 73, 47, 91, 47, cor);
        ssd1306_line(&ssd, 107, 47, 121, 47, cor);
        ssd1306_line(&ssd, 8, 46, 22, 46, cor);
        ssd1306_line(&ssd, 40, 46, 56, 46, cor);
        ssd1306_line(&ssd, 74, 46, 90, 46, cor);
        ssd1306_line(&ssd, 108, 46, 121, 46, cor);
        ssd1306_line(&ssd, 8, 45, 21, 45, cor);
        ssd1306_line(&ssd, 41, 45, 55, 45, cor);
        ssd1306_line(&ssd, 75, 45, 89, 45, cor);
        ssd1306_line(&ssd, 109, 45, 121, 45, cor);
        ssd1306_line(&ssd, 8, 44, 20, 44, cor);
        ssd1306_line(&ssd, 42, 44, 54, 44, cor);
        ssd1306_line(&ssd, 76, 44, 88, 44, cor);
        ssd1306_line(&ssd, 110, 44, 121, 44, cor);
        ssd1306_line(&ssd, 8, 43, 19, 43, cor);
        ssd1306_line(&ssd, 43, 43, 53, 43, cor);
        ssd1306_line(&ssd, 77, 43, 87, 43, cor);
        ssd1306_line(&ssd, 111, 43, 121, 43, cor);
        ssd1306_line(&ssd, 8, 42, 18, 42, cor);
        ssd1306_line(&ssd, 44, 42, 52, 42, cor);
        ssd1306_line(&ssd, 78, 42, 86, 42, cor);
        ssd1306_line(&ssd, 112, 42, 121, 42, cor);
        ssd1306_line(&ssd, 8, 41, 18, 41, cor);
        ssd1306_line(&ssd, 44, 41, 52, 41, cor);
        ssd1306_line(&ssd, 78, 41, 86, 41, cor);
        ssd1306_line(&ssd, 112, 41, 121, 41, cor);
        ssd1306_line(&ssd, 8, 40, 18, 40, cor);
        ssd1306_line(&ssd, 44, 40, 52, 40, cor);
        ssd1306_line(&ssd, 78, 40, 86, 40, cor);
        ssd1306_line(&ssd, 112, 40, 121, 40, cor);
        ssd1306_line(&ssd, 8, 39, 18, 39, cor);
        ssd1306_line(&ssd, 44, 39, 52, 39, cor);
        ssd1306_line(&ssd, 78, 39, 86, 39, cor);
        ssd1306_line(&ssd, 112, 39, 121, 39, cor);
        ssd1306_line(&ssd, 8, 38, 18, 38, cor);
        ssd1306_line(&ssd, 44, 38, 52, 38, cor);
        ssd1306_line(&ssd, 78, 38, 86, 38, cor);
        ssd1306_line(&ssd, 112, 38, 121, 38, cor);
        ssd1306_line(&ssd, 8, 37, 18, 37, cor);
        ssd1306_line(&ssd, 44, 37, 52, 37, cor);
        ssd1306_line(&ssd, 78, 37, 86, 37, cor);
        ssd1306_line(&ssd, 112, 37, 121, 37, cor);
        ssd1306_line(&ssd, 8, 36, 18, 36, cor);
        ssd1306_line(&ssd, 44, 36, 52, 36, cor);
        ssd1306_line(&ssd, 78, 36, 86, 36, cor);
        ssd1306_line(&ssd, 112, 36, 121, 36, cor);
        ssd1306_line(&ssd, 8, 35, 18, 35, cor);
        ssd1306_line(&ssd, 44, 35, 52, 35, cor);
        ssd1306_line(&ssd, 78, 35, 86, 35, cor);
        ssd1306_line(&ssd, 112, 35, 121, 35, cor);
        ssd1306_line(&ssd, 8, 34, 18, 34, cor);
        ssd1306_line(&ssd, 44, 34, 52, 34, cor);
        ssd1306_line(&ssd, 78, 34, 86, 34, cor);
        ssd1306_line(&ssd, 112, 34, 121, 34, cor);
        ssd1306_line(&ssd, 8, 33, 18, 33, cor);
        ssd1306_line(&ssd, 44, 33, 52, 33, cor);
        ssd1306_line(&ssd, 78, 33, 86, 33, cor);
        ssd1306_line(&ssd, 112, 33, 121, 33, cor);
        ssd1306_line(&ssd, 8, 32, 18, 32, cor);
        ssd1306_line(&ssd, 44, 32, 52, 32, cor);
        ssd1306_line(&ssd, 78, 32, 86, 32, cor);
        ssd1306_line(&ssd, 112, 32, 121, 32, cor);
        ssd1306_line(&ssd, 8, 31, 18, 31, cor);
        ssd1306_line(&ssd, 44, 31, 52, 31, cor);
        ssd1306_line(&ssd, 78, 31, 86, 31, cor);
        ssd1306_line(&ssd, 112, 31, 121, 31, cor);
        ssd1306_line(&ssd, 8, 30, 19, 30, cor);
        ssd1306_line(&ssd, 43, 30, 53, 30, cor);
        ssd1306_line(&ssd, 77, 30, 87, 30, cor);
        ssd1306_line(&ssd, 111, 30, 121, 30, cor);
        ssd1306_line(&ssd, 8, 29, 20, 29, cor);
        ssd1306_line(&ssd, 42, 29, 54, 29, cor);
        ssd1306_line(&ssd, 76, 29, 88, 29, cor);
        ssd1306_line(&ssd, 110, 29, 121, 29, cor);
        ssd1306_line(&ssd, 8, 28, 21, 28, cor);
        ssd1306_line(&ssd, 41, 28, 55, 28, cor);
        ssd1306_line(&ssd, 75, 28, 89, 28, cor);
        ssd1306_line(&ssd, 109, 28, 121, 28, cor);
        ssd1306_line(&ssd, 8, 27, 22, 27, cor);
        ssd1306_line(&ssd, 40, 27, 56, 27, cor);
        ssd1306_line(&ssd, 74, 27, 90, 27, cor);
        ssd1306_line(&ssd, 108, 27, 121, 27, cor);
        ssd1306_line(&ssd, 8, 26, 23, 26, cor);
        ssd1306_line(&ssd, 39, 26, 57, 26, cor);
        ssd1306_line(&ssd, 73, 26, 91, 26, cor);
        ssd1306_line(&ssd, 107, 26, 121, 26, cor);
        ssd1306_line(&ssd, 8, 25, 24, 25, cor);
        ssd1306_line(&ssd, 38, 25, 58, 25, cor);
        ssd1306_line(&ssd, 72, 25, 92, 25, cor);
        ssd1306_line(&ssd, 106, 25, 121, 25, cor);
        ssd1306_line(&ssd, 8, 24, 121, 24, cor);
        ssd1306_line(&ssd, 8, 23, 121, 23, cor);
        ssd1306_line(&ssd, 8, 22, 121, 22, cor);
        ssd1306_line(&ssd, 8, 21, 121, 21, cor);
        ssd1306_line(&ssd, 8, 20, 121, 20, cor);
        ssd1306_line(&ssd, 8, 19, 121, 19, cor);
        ssd1306_line(&ssd, 8, 18, 121, 18, cor);
        ssd1306_line(&ssd, 8, 17, 121, 17, cor);
        ssd1306_line(&ssd, 8, 16, 121, 16, cor);
        ssd1306_line(&ssd, 9, 15, 120, 15, cor);
        ssd1306_line(&ssd, 10, 14, 119, 14, cor);
        ssd1306_line(&ssd, 28, 13, 43, 13, cor);
        ssd1306_line(&ssd, 28, 12, 43, 12, cor);
        ssd1306_line(&ssd, 28, 11, 43, 11, cor);
        ssd1306_line(&ssd, 28, 10, 43, 10, cor);
        ssd1306_line(&ssd, 31, 8, 40, 8, cor);
        ssd1306_line(&ssd, 31, 7, 40, 7, cor);
        ssd1306_line(&ssd, 31, 6, 40, 6, cor);
        ssd1306_line(&ssd, 31, 5, 40, 5, cor);
        ssd1306_line(&ssd, 31, 4, 40, 4, cor);
        ssd1306_line(&ssd, 31, 3, 40, 3, cor);
        ssd1306_line(&ssd, 31, 2, 40, 2, cor);
        ssd1306_line(&ssd, 31, 1, 40, 1, cor);
        ssd1306_line(&ssd, 86, 13, 101, 13, cor);
        ssd1306_line(&ssd, 86, 12, 101, 12, cor);
        ssd1306_line(&ssd, 86, 11, 101, 11, cor);
        ssd1306_line(&ssd, 86, 10, 101, 10, cor);
        ssd1306_line(&ssd, 89, 8, 98, 8, cor);
        ssd1306_line(&ssd, 89, 7, 98, 7, cor);
        ssd1306_line(&ssd, 89, 6, 98, 6, cor);
        ssd1306_line(&ssd, 89, 5, 98, 5, cor);
        ssd1306_line(&ssd, 89, 4, 98, 4, cor);
        ssd1306_line(&ssd, 89, 3, 98, 3, cor);
        ssd1306_line(&ssd, 89, 2, 98, 2, cor);
        ssd1306_line(&ssd, 89, 1, 98, 1, cor);

        ssd1306_send_data(&ssd);

        if (modoNormalOn)
        {
            ssd1306_line(&ssd, 27, 44, 35, 44, cor);
            ssd1306_line(&ssd, 26, 43, 36, 43, cor);
            ssd1306_line(&ssd, 25, 42, 37, 42, cor);
            ssd1306_line(&ssd, 24, 41, 38, 41, cor);
            ssd1306_line(&ssd, 23, 40, 39, 40, cor);
            ssd1306_line(&ssd, 23, 39, 39, 39, cor);
            ssd1306_line(&ssd, 23, 38, 39, 38, cor);
            ssd1306_line(&ssd, 23, 37, 39, 37, cor);
            ssd1306_line(&ssd, 23, 36, 39, 36, cor);
            ssd1306_line(&ssd, 23, 35, 39, 35, cor);
            ssd1306_line(&ssd, 23, 34, 39, 34, cor);
            ssd1306_line(&ssd, 23, 33, 39, 33, cor);
            ssd1306_line(&ssd, 24, 32, 38, 32, cor);
            ssd1306_line(&ssd, 25, 31, 37, 31, cor);
            ssd1306_line(&ssd, 26, 30, 36, 30, cor);
            ssd1306_line(&ssd, 27, 29, 35, 29, cor);

            ssd1306_send_data(&ssd);

            for (int i = 0; i < 6000 && modoNormalOn; i += 100)
                vTaskDelay(pdMS_TO_TICKS(100));

            ssd1306_line(&ssd, 26, 43, 36, 43, !cor);
            ssd1306_line(&ssd, 27, 44, 35, 44, !cor);
            ssd1306_line(&ssd, 25, 42, 37, 42, !cor);
            ssd1306_line(&ssd, 24, 41, 38, 41, !cor);
            ssd1306_line(&ssd, 23, 40, 39, 40, !cor);
            ssd1306_line(&ssd, 23, 39, 39, 39, !cor);
            ssd1306_line(&ssd, 23, 38, 39, 38, !cor);
            ssd1306_line(&ssd, 23, 37, 39, 37, !cor);
            ssd1306_line(&ssd, 23, 36, 39, 36, !cor);
            ssd1306_line(&ssd, 23, 35, 39, 35, !cor);
            ssd1306_line(&ssd, 23, 34, 39, 34, !cor);
            ssd1306_line(&ssd, 23, 33, 39, 33, !cor);
            ssd1306_line(&ssd, 24, 32, 38, 32, !cor);
            ssd1306_line(&ssd, 25, 31, 37, 31, !cor);
            ssd1306_line(&ssd, 26, 30, 36, 30, !cor);
            ssd1306_line(&ssd, 27, 29, 35, 29, !cor);

            ssd1306_line(&ssd, 61, 44, 69, 44, cor);
            ssd1306_line(&ssd, 60, 43, 70, 43, cor);
            ssd1306_line(&ssd, 59, 42, 71, 42, cor);
            ssd1306_line(&ssd, 58, 41, 72, 41, cor);
            ssd1306_line(&ssd, 57, 40, 73, 40, cor);
            ssd1306_line(&ssd, 57, 39, 73, 39, cor);
            ssd1306_line(&ssd, 57, 38, 73, 38, cor);
            ssd1306_line(&ssd, 57, 37, 73, 37, cor);
            ssd1306_line(&ssd, 57, 36, 73, 36, cor);
            ssd1306_line(&ssd, 57, 35, 73, 35, cor);
            ssd1306_line(&ssd, 57, 34, 73, 34, cor);
            ssd1306_line(&ssd, 57, 33, 73, 33, cor);
            ssd1306_line(&ssd, 58, 32, 72, 32, cor);
            ssd1306_line(&ssd, 59, 31, 71, 31, cor);
            ssd1306_line(&ssd, 60, 30, 70, 30, cor);
            ssd1306_line(&ssd, 61, 29, 69, 29, cor);

            ssd1306_send_data(&ssd);

            for (int i = 0; i < 3000 && modoNormalOn; i += 100)
                vTaskDelay(pdMS_TO_TICKS(100));

            ssd1306_line(&ssd, 61, 44, 69, 44, !cor);
            ssd1306_line(&ssd, 60, 43, 70, 43, !cor);
            ssd1306_line(&ssd, 59, 42, 71, 42, !cor);
            ssd1306_line(&ssd, 58, 41, 72, 41, !cor);
            ssd1306_line(&ssd, 57, 40, 73, 40, !cor);
            ssd1306_line(&ssd, 57, 39, 73, 39, !cor);
            ssd1306_line(&ssd, 57, 38, 73, 38, !cor);
            ssd1306_line(&ssd, 57, 37, 73, 37, !cor);
            ssd1306_line(&ssd, 57, 36, 73, 36, !cor);
            ssd1306_line(&ssd, 57, 35, 73, 35, !cor);
            ssd1306_line(&ssd, 57, 34, 73, 34, !cor);
            ssd1306_line(&ssd, 57, 33, 73, 33, !cor);
            ssd1306_line(&ssd, 58, 32, 72, 32, !cor);
            ssd1306_line(&ssd, 59, 31, 71, 31, !cor);
            ssd1306_line(&ssd, 60, 30, 70, 30, !cor);
            ssd1306_line(&ssd, 61, 29, 69, 29, !cor);

            ssd1306_line(&ssd, 95, 44, 103, 44, cor);
            ssd1306_line(&ssd, 94, 43, 104, 43, cor);
            ssd1306_line(&ssd, 93, 42, 105, 42, cor);
            ssd1306_line(&ssd, 92, 41, 106, 41, cor);
            ssd1306_line(&ssd, 91, 40, 107, 40, cor);
            ssd1306_line(&ssd, 91, 39, 107, 39, cor);
            ssd1306_line(&ssd, 91, 38, 107, 38, cor);
            ssd1306_line(&ssd, 91, 37, 107, 37, cor);
            ssd1306_line(&ssd, 91, 36, 107, 36, cor);
            ssd1306_line(&ssd, 91, 35, 107, 35, cor);
            ssd1306_line(&ssd, 91, 34, 107, 34, cor);
            ssd1306_line(&ssd, 91, 33, 107, 33, cor);
            ssd1306_line(&ssd, 92, 32, 106, 32, cor);
            ssd1306_line(&ssd, 93, 31, 105, 31, cor);
            ssd1306_line(&ssd, 94, 30, 104, 30, cor);
            ssd1306_line(&ssd, 95, 29, 104, 29, cor);

            ssd1306_send_data(&ssd);

            for (int i = 0; i < 6000 && modoNormalOn; i += 100)
                vTaskDelay(pdMS_TO_TICKS(100));

            ssd1306_line(&ssd, 95, 44, 103, 44, !cor);
            ssd1306_line(&ssd, 94, 43, 104, 43, !cor);
            ssd1306_line(&ssd, 93, 42, 105, 42, !cor);
            ssd1306_line(&ssd, 92, 41, 106, 41, !cor);
            ssd1306_line(&ssd, 91, 40, 107, 40, !cor);
            ssd1306_line(&ssd, 91, 39, 107, 39, !cor);
            ssd1306_line(&ssd, 91, 38, 107, 38, !cor);
            ssd1306_line(&ssd, 91, 37, 107, 37, !cor);
            ssd1306_line(&ssd, 91, 36, 107, 36, !cor);
            ssd1306_line(&ssd, 91, 35, 107, 35, !cor);
            ssd1306_line(&ssd, 91, 34, 107, 34, !cor);
            ssd1306_line(&ssd, 91, 33, 107, 33, !cor);
            ssd1306_line(&ssd, 92, 32, 106, 32, !cor);
            ssd1306_line(&ssd, 93, 31, 105, 31, !cor);
            ssd1306_line(&ssd, 94, 30, 104, 30, !cor);
            ssd1306_line(&ssd, 95, 29, 104, 29, !cor);
        }
        else
        {
        }
        ssd1306_send_data(&ssd);
    }
}

int main()
{
    stdio_init_all();

    iniciar_pwm();

    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    xTaskCreate(vTaskRGB, "Modo normal", configMINIMAL_STACK_SIZE,
                NULL, tskIDLE_PRIORITY, &xTaskRGBHandle);
    xTaskCreate(vTaskBotao, "Botao", configMINIMAL_STACK_SIZE,
                NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vTaskDisplay, "Task display", configMINIMAL_STACK_SIZE,
                NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vTaskMatriz, "Matriz", configMINIMAL_STACK_SIZE,
                NULL, tskIDLE_PRIORITY, NULL);
    vTaskStartScheduler();
    panic_unsupported();
}