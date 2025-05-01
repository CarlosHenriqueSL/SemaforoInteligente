#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "hardware/uart.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "blink.pio.h"
#include "font.h"
#include "ssd1306.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO 0x3C

#define BUTTON_A 5
#define LED_PIN_GREEN 11
#define LED_PIN_RED 13

#define BUZZER_PIN 21

volatile bool modoNormalOn = true;

void init_buzzer()
{
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_enabled(slice, true);
}

void tocarBuzzer(uint frequencia, uint duracao_ms)
{
    uint slice = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint wrap = 125000000 / frequencia;
    pwm_set_wrap(slice, wrap);
    pwm_set_chan_level(slice, pwm_gpio_to_channel(BUZZER_PIN), wrap / 2);
    vTaskDelay(pdMS_TO_TICKS(duracao_ms));
    pwm_set_chan_level(slice, pwm_gpio_to_channel(BUZZER_PIN), 0);
}

void vModoNormal()
{
    gpio_init(LED_PIN_GREEN);
    gpio_set_dir(LED_PIN_GREEN, GPIO_OUT);
    gpio_init(LED_PIN_RED);
    gpio_set_dir(LED_PIN_RED, GPIO_OUT);

    init_buzzer();
    while (true)
    {
        if (modoNormalOn)
        {
            gpio_put(LED_PIN_GREEN, true);
            tocarBuzzer(3500, 1000);
            vTaskDelay(pdMS_TO_TICKS(6000));
            gpio_put(LED_PIN_RED, true);
            while (true)
            {
                tocarBuzzer(4000, 250);
                vTaskDelay(pdMS_TO_TICKS(250));
                tocarBuzzer(4000, 250);
                vTaskDelay(pdMS_TO_TICKS(250));
                tocarBuzzer(4000, 250);
                vTaskDelay(pdMS_TO_TICKS(250));
                tocarBuzzer(4000, 250);
                vTaskDelay(pdMS_TO_TICKS(250));
                tocarBuzzer(4000, 250);
                vTaskDelay(pdMS_TO_TICKS(250));
                tocarBuzzer(4000, 250);
                vTaskDelay(pdMS_TO_TICKS(250));
                tocarBuzzer(4000, 250);
                vTaskDelay(pdMS_TO_TICKS(250));
                tocarBuzzer(4000, 250);
                vTaskDelay(pdMS_TO_TICKS(250));
                tocarBuzzer(4000, 250);
                vTaskDelay(pdMS_TO_TICKS(250));
                tocarBuzzer(4000, 250);
                vTaskDelay(pdMS_TO_TICKS(250));
                tocarBuzzer(4000, 250);
                vTaskDelay(pdMS_TO_TICKS(250));
                tocarBuzzer(4000, 250);
                vTaskDelay(pdMS_TO_TICKS(250));
                break;
            }
            gpio_put(LED_PIN_GREEN, false);
            gpio_put(LED_PIN_RED, true);
            while (true)
            {
                tocarBuzzer(2000, 500);
                vTaskDelay(pdMS_TO_TICKS(1500));
                tocarBuzzer(2000, 500);
                vTaskDelay(pdMS_TO_TICKS(1500));
                tocarBuzzer(2000, 500);
                vTaskDelay(pdMS_TO_TICKS(1500));
                tocarBuzzer(2000, 500);
                vTaskDelay(pdMS_TO_TICKS(1500));
                tocarBuzzer(2000, 500);
                vTaskDelay(pdMS_TO_TICKS(1500));
                break;
            }
            gpio_put(LED_PIN_RED, false);
            break;
        }
    }
}

void vTaskBotao()
{
    while (true)
    {
        modoNormalOn = gpio_get(BUTTON_A);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void vDisplayTask()
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
        ssd1306_line(&ssd, 15, 21, 15, 41, cor);
        ssd1306_line(&ssd, 16, 20, 16, 42, cor);
        ssd1306_line(&ssd, 17, 19, 17, 43, cor);
        ssd1306_line(&ssd, 18, 18, 18, 44, cor);
        ssd1306_line(&ssd, 19, 17, 19, 45, cor);
        ssd1306_line(&ssd, 20, 16, 20, 46, cor);
        ssd1306_line(&ssd, 21, 16, 21, 46, cor);
        ssd1306_line(&ssd, 22, 16, 22, 46, cor);
        ssd1306_line(&ssd, 23, 16, 23, 27, cor);
        ssd1306_line(&ssd, 23, 35, 23, 46, cor);
        ssd1306_line(&ssd, 24, 16, 24, 26, cor);
        ssd1306_line(&ssd, 24, 36, 24, 46, cor);
        ssd1306_line(&ssd, 25, 16, 25, 25, cor);
        ssd1306_line(&ssd, 25, 37, 25, 46, cor);
        ssd1306_line(&ssd, 26, 16, 26, 24, cor);
        ssd1306_line(&ssd, 26, 38, 26, 46, cor);
        ssd1306_line(&ssd, 27, 16, 27, 23, cor);
        ssd1306_line(&ssd, 27, 39, 27, 46, cor);
        ssd1306_line(&ssd, 28, 16, 28, 23, cor);
        ssd1306_line(&ssd, 28, 39, 28, 46, cor);
        ssd1306_line(&ssd, 29, 16, 29, 23, cor);
        ssd1306_line(&ssd, 29, 39, 29, 46, cor);
        ssd1306_line(&ssd, 30, 16, 30, 23, cor);
        ssd1306_line(&ssd, 30, 39, 30, 46, cor);
        ssd1306_line(&ssd, 31, 16, 31, 23, cor);
        ssd1306_line(&ssd, 31, 39, 31, 46, cor);
        ssd1306_line(&ssd, 32, 16, 32, 23, cor);
        ssd1306_line(&ssd, 32, 39, 32, 46, cor);
        ssd1306_line(&ssd, 33, 16, 33, 23, cor);
        ssd1306_line(&ssd, 33, 39, 33, 46, cor);
        ssd1306_send_data(&ssd);
        sleep_ms(735);
    }
}

#define UART_ID uart1
#define BAUD_RATE 115200

#define UART_TX_PIN 4
#define UART_RX_PIN 5

int main()
{
    stdio_init_all();

    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    PIO pio = pio0;
    uint offset = pio_add_program(pio, &blink_program);
    printf("Loaded program at %d\n", offset);

    uart_init(UART_ID, BAUD_RATE);

    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    uart_puts(UART_ID, " Hello, UART!\n");


   /* xTaskCreate(vDisplayTask, "Modo normal", configMINIMAL_STACK_SIZE,
                NULL, tskIDLE_PRIORITY, NULL);*/
    xTaskCreate(vTaskBotao, "Botao", configMINIMAL_STACK_SIZE,
                NULL, tskIDLE_PRIORITY, NULL);
    vTaskStartScheduler();
    panic_unsupported();
}
