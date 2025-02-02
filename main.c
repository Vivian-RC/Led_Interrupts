// código feito por Vivian Rodrigues Castro.
// 
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

#define IS_RGBW false  // Define se os LEDs WS2812 possuem canal branco
#define NUM_PIXELS 25 // Número total de LEDs na matriz
#define WS2812_PIN 7  // Pino GPIO onde os LEDs WS2812 estão conectados
#define RED_LED_PIN 13 // Pino GPIO do LED vermelho
#define tempo 100 // 5 Hz para o LED vermelho

const uint button_A = 5;  // Pino GPIO do botão A
const uint button_B = 6; // Pino GPIO do botão B

static volatile uint8_t current_number = 0; // Número exibido na matriz de LEDs
static volatile uint32_t last_time = 0; // Tempo da última interrupção para debounce

static void gpio_irq_handler(uint gpio, uint32_t events); // Protótipo da função de interrupção para os botões

bool led_buffer[NUM_PIXELS]; // Buffer que armazena o estado dos LEDs

// Padrões de números (0-9) para exibição na matriz WS2812
bool number_patterns[10][NUM_PIXELS] = {
    // número 0
    { 
        0, 1, 1, 1, 0,
        0, 1, 0, 1, 0,
        0, 1, 0, 1, 0,
        0, 1, 0, 1, 0,
        0, 1, 1, 1, 0
    },
    // Número 1
    { 
        0, 1, 1, 1, 0,
        0, 0, 1, 0, 0,
        0, 0, 1, 0, 0,
        0, 1, 1, 0, 0,
        0, 0, 1, 0, 0
    },
    // Número 2
    { 
        0, 1, 1, 1, 0,
        0, 1, 0, 0, 0,
        0, 1, 1, 1, 0,
        0, 0, 0, 1, 0,
        0, 1, 1, 1, 0
    },
    // Número 3
    { 
        0, 1, 1, 1, 0,
        0, 0, 0, 1, 0,
        0, 1, 1, 1, 0,
        0, 0, 0, 1, 0,
        0, 1, 1, 1, 0
    },
    // Número 4
    { 
        0, 1, 0, 0, 0,
        0, 0, 0, 1, 0,
        0, 1, 1, 1, 0,
        0, 1, 0, 1, 0,
        0, 1, 0, 1, 0
    },
    // Número 5
    { 
        0, 1, 1, 1, 0,
        0, 0, 0, 1, 0,
        0, 1, 1, 1, 0,
        0, 1, 0, 0, 0,
        0, 1, 1, 1, 0
    },
    // Número 6
    { 
        0, 1, 1, 1, 0,
        0, 1, 0, 1, 0,
        0, 1, 1, 1, 0,
        0, 1, 0, 0, 0,
        0, 1, 1, 1, 0
    },
    // Número 7
    { 
        0, 1, 0, 0, 0,
        0, 0, 0, 1, 0,
        0, 1, 0, 0, 0,
        0, 1, 0, 1, 0,
        0, 1, 1, 1, 0
    },
    // Número 8
    { 
        0, 1, 1, 1, 0,
        0, 1, 0, 1, 0,
        0, 1, 1, 1, 0,
        0, 1, 0, 1, 0,
        0, 1, 1, 1, 0
    },
    // Número 9 corrigido
    { 
        0, 1, 1, 1, 0,
        0, 0, 0, 1, 0,
        0, 1, 1, 1, 0,
        0, 1, 0, 1, 0,
        0, 1, 1, 1, 0
    }
};

static inline void put_pixel(uint32_t pixel_grb) { // Envia um valor de cor para o LED WS2812 através do PIO
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u); // O deslocamento de 8 bits garante a formatação correta dos dados
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    // Converte valores de cor RGB para um único valor de 32 bits no formato GRB
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

void set_number(uint8_t number) {
    // Atualiza o buffer de LEDs com o padrão do número a ser exibido
    for (int i = 0; i < NUM_PIXELS; i++) {
        led_buffer[i] = number_patterns[number][i];
    }
}

void display_number() { 
     // Define a cor azul para os LEDs ativos
    uint32_t color = urgb_u32(0, 0, 20);
    // Atualiza a matriz de LEDs com a cor definida
    for (int i = 0; i < NUM_PIXELS; i++) {
        put_pixel(led_buffer[i] ? color : 0);
    }
}

void gpio_irq_handler(uint gpio, uint32_t events) {
     // Obtém o tempo atual em microssegundos
    uint32_t current_time = to_us_since_boot(get_absolute_time());
    // Implementação de debounce: verifica se passou tempo suficiente desde a última pressão
    if (current_time - last_time > 400000) {
        // Incrementa ou decrementa o número exibido dependendo do botão pressionado
        last_time = current_time;
        current_number = (gpio == button_A) ? (current_number + 1) % 10 : (current_number == 0) ? 9 : current_number - 1;
        set_number(current_number); // Atualiza o padrão do número exibido na matriz de LEDs
    }
}

int main() {
    stdio_init_all();
    gpio_init(button_A);
    gpio_set_dir(button_A, GPIO_IN);
    gpio_pull_up(button_A);
    gpio_init(button_B);
    gpio_set_dir(button_B, GPIO_IN);
    gpio_pull_up(button_B);
    gpio_init(RED_LED_PIN);
    gpio_set_dir(RED_LED_PIN, GPIO_OUT);
    gpio_set_irq_enabled_with_callback(button_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(button_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);
    while (1) {
        gpio_put(RED_LED_PIN, true);
        sleep_ms(tempo);
        gpio_put(RED_LED_PIN, false);
        sleep_ms(tempo);
        display_number();
    }
    return 0;
}
