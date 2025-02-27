#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "hardware/pwm.h"

#define BTN_A 5          // Botão A no pino 5
#define BTN_B 6          // Botão B no pino 6
#define BTN_JOYSTICK 22  // Botão Joystick no pino 22
#define LED_R_PIN 13     // Pino do LED RGB Vermelho
#define BUZZER_PIN 10    // Pino do Buzzer (pino PWM)
#define BUZZER_PIN2 21   // Pino do Buzzer 2 (pino PWM)

#define LIMITE 20

volatile int contador = 0;  // Contador de pulsos
volatile bool atualizar_display = true;  // Flag para atualização do display
volatile bool parar_programa = false;  // Flag para parar o programa

ssd1306_t ssd;  // Estrutura do display OLED

// Função para configurar o PWM no buzzer 1
void configurar_pwm_buzzer() {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);  // Configura o pino como PWM
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);  // Obtém o número do slice PWM para o pino
    pwm_set_clkdiv(slice_num, 350.f);  // Define a frequência do PWM (aproximadamente 1420 kHz)
    pwm_set_wrap(slice_num, 350);  // Define o valor de "wrap" para controlar a frequência
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 64);  // 50% de ciclo de trabalho (apito)
    pwm_set_enabled(slice_num, true);  // Habilita o PWM
}


// Função para configurar o PWM no buzzer 2 
void configurar_pwm_buzzer2() {
    gpio_set_function(BUZZER_PIN2, GPIO_FUNC_PWM);  // Configura o pino como PWM
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN2);  // Obtém o número do slice PWM para o pino
    pwm_set_clkdiv(slice_num, 350.f);  // Define a frequência do PWM (aproximadamente 1420 Hz)
    pwm_set_wrap(slice_num, 350);  // Define o valor de "wrap" para controlar a frequência
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 64);  // 50% de ciclo de trabalho (apito)
    pwm_set_enabled(slice_num, true);  // Habilita o PWM
}


// Função para acionar o alarme e LED vermelho
void alerta_limite() {
    configurar_pwm_buzzer();  // Ativa o buzzer 1
    configurar_pwm_buzzer2(); // Ativa o buzzer 2
    gpio_put(LED_R_PIN, 1);   // Liga o LED vermelho
}


// Função para exibir no display
void exibir_no_display(int numero) {
    ssd1306_fill(&ssd, false);  // Limpa o display
    ssd1306_rect(&ssd, 3, 3, 122, 58, 1, 0);  // Desenha um retângulo
    char texto[16];

    if (numero == -1) {
        snprintf(texto, sizeof(texto), "PROGRAMA");
        ssd1306_draw_string(&ssd, texto, 30, 30);  // Exibe a mensagem "PROGRAMA"
        snprintf(texto, sizeof(texto), "PARADO");
        ssd1306_draw_string(&ssd, texto, 30, 45);  // Exibe a mensagem "PARADO"
    } else {
        snprintf(texto, sizeof(texto), "%d", numero);  // Exibe a contagem no display
        ssd1306_draw_string(&ssd, texto, 53, 30);  
    }

    ssd1306_send_data(&ssd);  // Envia os dados para o display
}

// Função de interrupção para os botões (com debounce)
void gpio_irq_handler(uint gpio, uint32_t events) {
    static uint32_t last_interrupt_time = 0;
    uint32_t current_time = to_ms_since_boot(get_absolute_time());

    if (current_time - last_interrupt_time < 300) {
        return;  // Ignora se o intervalo entre cliques for menor que 300 ms
    }

    last_interrupt_time = current_time;

    if (gpio == BTN_A && contador < LIMITE) {
        contador++;
        printf("Botão A pressionado! Contagem: %d\n", contador);
    } else if (gpio == BTN_B) {
        contador++; 
        printf("Botão B pressionado! Contagem: %d\n", contador);
    } else if (gpio == BTN_JOYSTICK) {
        contador++;
        printf("Botão Joystick pressionado! Contagem: %d\n", contador);
    }

    // Verifica se atingiu o limite
    if (contador >= LIMITE) {
        parar_programa = true;  // Ativa a flag para parar o programa
        printf("Programa parado por segurança.\n");
        exibir_no_display(-1);  // Exibe a mensagem no serial monitor
        alerta_limite();        // Chama a função para o alarme
    }

    atualizar_display = true;  // Atualiza o display
}

int main() {
    stdio_init_all();

    // Inicializa o I2C para o display OLED
    i2c_init(i2c1, 400000);
    gpio_set_function(14, GPIO_FUNC_I2C);  // GPIO 14 para SDA
    gpio_set_function(15, GPIO_FUNC_I2C);  // GPIO 15 para SCL
    gpio_pull_up(14);
    gpio_pull_up(15);

    // Inicializa o display OLED
    ssd1306_init(&ssd, 128, 64, false, 0x3C, i2c1);
    ssd1306_fill(&ssd, false);  // Limpa o display
    ssd1306_send_data(&ssd);  // Envia os dados para o display

    // Configuração dos pinos para o LED
    gpio_init(LED_R_PIN);
    gpio_set_dir(LED_R_PIN, GPIO_OUT);

    // Configura os botões com interrupções
    gpio_init(BTN_A);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_pull_up(BTN_A);
    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_B);
    gpio_set_irq_enabled_with_callback(BTN_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    gpio_init(BTN_JOYSTICK);
    gpio_set_dir(BTN_JOYSTICK, GPIO_IN);
    gpio_pull_up(BTN_JOYSTICK);
    gpio_set_irq_enabled_with_callback(BTN_JOYSTICK, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    while (1) {
        // Atualiza o display quando necessário
        if (atualizar_display) {
            if (parar_programa) {
                // Quando o limite for atingido, parar o programa
                break;
            }
            exibir_no_display(contador);  // Atualiza o display com o contador
            atualizar_display = false;  // Reseta a flag
        }

        sleep_ms(100);  // Atraso para não consumir 100% do CPU
    }

    return 0;
}
 