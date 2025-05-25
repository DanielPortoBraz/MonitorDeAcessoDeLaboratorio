/*
 * Autor: Daniel Porto Braz
 * Este código apresenta um sistema de acesso a um laboratório, que exibe o número de usuários em cada instante, suas entradas e saídas. 
 * O laboratório em questão possui capacidade máxima para 11 usuários, logo se houver a entrada de mais usuário além dessa capacidade, 
 * o sistema irá emitir um beep para indicar que o local está lotado. Além disso, o sistema possui um LED RGB para indicar demais estados
 * do laboratório com base no número de usuários, um display para exibir todas as informações, e um botão de reset que simula a saída de
 * todas os usuários do local.
 * 
 * Este código conta com aplicações de tarefas, mutexes, semáforos binários e de contagem, do FreeRTOS. 
 *
 * Este projeto é baseado no repositório de Wilton Lacerda: https://github.com/wiltonlacerda/EmbarcaTechResU1Ex07   
 */


#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

#include "lib/ssd1306.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "stdio.h"

// Semáforos e mutex
static SemaphoreHandle_t xCountUsers; // Semáforo de contagem para o número de usuário
static SemaphoreHandle_t xCountReset; // Semáforo binário para zerar a contagem
static SemaphoreHandle_t xDisplayMutex; // Mutex para uso do display

const uint MAX_USERS = 11; // Número máximo de usuários
static volatile uint num_users = 0; // Número de usuários ativos
char buffer_users[32] = ""; // Buffer para guardar a string com informações dos usuários no display
char buffer_state[32] = ""; // Buffer para guardar a string com informações do estado da sala no display

// LED RGB
#define LRED_PIN 13
#define LGREEN_PIN 11
#define LBLUE_PIN 12

// Display SSD1306
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO 0x3C
ssd1306_t ssd;

// Botões
#define BUTTON_A 5 // Entrada de usuários
#define BUTTON_B 6 // Saída de usuários
#define BUTTON_J 22 // Reset da contagem

// Buzzer
#define BUZZER_PIN 21
// Valores PWM do Buzzer
const uint16_t PERIOD = 59609; // WRAP
const float DIVCLK = 16.0; // Divisor inteiro
static uint slice_21;
const uint16_t dc_values[] = {PERIOD * 0.3, 0}; // Duty Cycle de 30% e 0%


// ----------- CONFIGURAÇÕES ------------

void set_leds(){
    gpio_init(LRED_PIN);
    gpio_set_dir(LRED_PIN, GPIO_OUT);
    gpio_put(LRED_PIN, 0);

    gpio_init(LGREEN_PIN);
    gpio_set_dir(LGREEN_PIN, GPIO_OUT);
    gpio_put(LGREEN_PIN, 0);

    gpio_init(LBLUE_PIN);
    gpio_set_dir(LBLUE_PIN, GPIO_OUT);
    gpio_put(LBLUE_PIN, 0);
}

void set_display(){
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);
}


void set_buttons(){
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
    
    gpio_init(BUTTON_J);
    gpio_set_dir(BUTTON_J, GPIO_IN);
    gpio_pull_up(BUTTON_J);
}

void setup_pwm(){
    
    // PWM do BUZZER
    // Configura para soar 440 Hz
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    slice_21 = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_clkdiv(slice_21, DIVCLK);
    pwm_set_wrap(slice_21, PERIOD);
    pwm_set_gpio_level(BUZZER_PIN, 0);
    pwm_set_enabled(slice_21, true);
}


// ------------ INTERRUPPÇÕES ---------------

// Botão J reseta o número de usuários
void buttonJ_callback(uint gpio, uint32_t events){
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xCountReset, &xHigherPriorityTaskWoken); // Alterna o semáforo binário para 1
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void gpio_irq_handler(uint gpio, uint32_t events)
{
    if (gpio == BUTTON_J) // Botão J pressionado
    {
        buttonJ_callback(gpio, events);
    }
}

//  ------------ TAREFAS -----------------

// Tarefa do LED RGB, exibe uma cor para indicar o estado do sistema
void vLedRgbTask(void *vParams){
    gpio_put(LBLUE_PIN, 1); // No momento inicial, a sala está vazia, portanto o LED azul fica ligado

    while (true){
            
        if (num_users == 0){ // Se não tiver usuários no laboratório, emite sinal azul 
            gpio_put(LRED_PIN, 0);
            gpio_put(LGREEN_PIN, 0);
            gpio_put(LBLUE_PIN, 1);
            sprintf(buffer_state, "Estado: VAZIO");
        }  
        
        else if (num_users <= MAX_USERS - 2){ // Se tiver usuários no laboratório, emite sinal verde        
            gpio_put(LBLUE_PIN, 0);
            gpio_put(LRED_PIN, 0);
            gpio_put(LGREEN_PIN, 1);
            sprintf(buffer_state, "Estado: ");
        }

        else if (num_users == MAX_USERS - 1){ // Se restar apenas uma vaga no laboratório, emite sinal amarelo        
            gpio_put(LBLUE_PIN, 0);
            gpio_put(LGREEN_PIN, 1);
            gpio_put(LRED_PIN, 1);
            sprintf(buffer_state, "Estado: ");
        }

        else{ // Se atingir a capacidade máxima de usuários no laboratório, emite sinal vermelho        
            gpio_put(LGREEN_PIN, 0);
            gpio_put(LBLUE_PIN, 0);
            gpio_put(LRED_PIN, 1);
            sprintf(buffer_state, "Estado: LOTADO");
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Tarefa para a entrada de usuário
void vTaskEntrada(void *vParams){

    while(true){

        if (!gpio_get(BUTTON_A)){
            vTaskDelay(pdMS_TO_TICKS(50));

            if (!gpio_get(BUTTON_A)){
                xSemaphoreGive(xCountUsers); // Incrementa a contagem do semáforo
                num_users = uxSemaphoreGetCount(xCountUsers);
                
                // Beep de aviso, caso a sala esteja lotada e seja solicitada mais uma entrada
                if (num_users == MAX_USERS){
                    pwm_set_gpio_level(BUZZER_PIN, dc_values[0]);
                    vTaskDelay(pdMS_TO_TICKS(200));
                    pwm_set_gpio_level(BUZZER_PIN, dc_values[1]);
                }
            }
        }

        sprintf(buffer_users, "Usuarios: %d", num_users);

        // Desenha na matriz ao tentar solicitar acesso por um mutex
        if (xSemaphoreTake(xDisplayMutex, portMAX_DELAY) == pdTRUE){
            ssd1306_draw_string(&ssd, "             ", 20, 24);
            ssd1306_draw_string(&ssd, "             ", 20, 44);
            ssd1306_draw_string(&ssd, buffer_users, 20, 24);
            ssd1306_draw_string(&ssd, buffer_state, 8, 44);
            ssd1306_send_data(&ssd);
            xSemaphoreGive(xDisplayMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Tarefa para a saída de usuários
void vTaskSaida(void *vParams){

    while(true){

        if (!gpio_get(BUTTON_B)){
            vTaskDelay(pdMS_TO_TICKS(50));

            if (!gpio_get(BUTTON_B) && uxSemaphoreGetCount(xCountUsers) > 0){
                xSemaphoreTake(xCountUsers, portMAX_DELAY); // Decrementa a contagem do semáforo
                num_users = uxSemaphoreGetCount(xCountUsers);
            }
        }

        sprintf(buffer_users, "Usuarios: %d", num_users);

        // Desenha na matriz ao tentar solicitar acesso por um mutex
        if (xSemaphoreTake(xDisplayMutex, portMAX_DELAY) == pdTRUE){
            ssd1306_draw_string(&ssd, "             ", 20, 24);
            ssd1306_draw_string(&ssd, "              ", 8, 44);
            ssd1306_draw_string(&ssd, buffer_users, 20, 24);
            ssd1306_draw_string(&ssd, buffer_state, 8, 44);
            ssd1306_send_data(&ssd);
            xSemaphoreGive(xDisplayMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Tarefa para resetar a contagem de usuários
void vTaskReset(void *vParams){

    while(true){
        
        if (xSemaphoreTake(xCountReset, portMAX_DELAY) == pdTRUE){

            // Beep duplo
            for (int i = 0; i < 2; i++){
                pwm_set_gpio_level(BUZZER_PIN, dc_values[0]);
                vTaskDelay(pdMS_TO_TICKS(100));
                pwm_set_gpio_level(BUZZER_PIN, dc_values[1]);
                vTaskDelay(pdMS_TO_TICKS(100));
            }

            // Para fazer o reset do semáforo, ele é deletado e recriado do zero
            vSemaphoreDelete(xCountUsers);
            xCountUsers = xSemaphoreCreateCounting(MAX_USERS, 0);

            num_users = uxSemaphoreGetCount(xCountUsers);
            
            // Desenha na matriz ao tentar solicitar acesso por um mutex
            if (xSemaphoreTake(xDisplayMutex, portMAX_DELAY) == pdTRUE){
                ssd1306_draw_string(&ssd, "             ", 20, 24);
                ssd1306_draw_string(&ssd, "              ", 8, 44);
                ssd1306_draw_string(&ssd, "Usuarios: 0", 20, 24);
                ssd1306_draw_string(&ssd, "Estado: VAZIO", 8, 44);
                ssd1306_send_data(&ssd);
                xSemaphoreGive(xDisplayMutex);
            }
        }
    }
}


// ------------- MENU PRINCIPAL --------------
int main()
{
    stdio_init_all();

    set_leds();
    set_display();
    set_buttons();
    setup_pwm();

    // Desenha a tela inicial com as informações
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
    ssd1306_rect(&ssd, 63, 127, 4, 63, true, false);
    ssd1306_hline(&ssd, 0, 127, 20, true);
    ssd1306_hline(&ssd, 0, 127, 40, true);
    ssd1306_send_data(&ssd);
    ssd1306_draw_string(&ssd, "LABORATORIO A", 20, 4);
    ssd1306_draw_string(&ssd, buffer_users, 20, 24);
    ssd1306_draw_string(&ssd, buffer_state, 8, 44);

    // Ativação da interrupção do botão J
    gpio_set_irq_enabled_with_callback(BUTTON_J, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    xCountUsers = xSemaphoreCreateCounting(MAX_USERS, 0);
    xCountReset = xSemaphoreCreateBinary();
    xDisplayMutex = xSemaphoreCreateMutex();

    // Criação das tarefas
    xTaskCreate(vLedRgbTask, "RGB Task", 256, NULL, 1, NULL);
    xTaskCreate(vTaskEntrada, "Entrada Task", 256, NULL, 1, NULL);
    xTaskCreate(vTaskSaida, "Saida Task", 256, NULL, 1, NULL);
    xTaskCreate(vTaskReset, "Reset Task", 256, NULL, 1, NULL);

    vTaskStartScheduler();
    panic_unsupported();
}
