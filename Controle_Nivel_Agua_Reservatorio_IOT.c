#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ssd1306.h"
#include "font.h"
#include <stdio.h>

#define RELE_PIN 16
// #define LED_PIN 12
#define BOTAO_A 5
#define BOTAO_JOY 22
#define POTENCIOMETRO_BOIA 28

#define WIFI_SSID "Leonardo"
#define WIFI_PASS "00695470PI"

#define I2C_PORT_DISP i2c1
#define I2C_SDA_DISP 14
#define I2C_SCL_DISP 15
#define endereco 0x3C

// Definições para o potenciômetro de boia
#define ADC_MIN_LEITURA 22   // Valor mínimo lido do potenciômetro (quando o reservatório está vazio)
#define ADC_MAX_LEITURA 4070 // Valor máximo lido do potenciômetro (quando o reservatório está cheio)

volatile static int limite_minimo_nivel_agua = 10;
volatile static int limite_maximo_nivel_agua = 90;


int map_to_percentage(uint16_t adc_value) {
    // Garante que o valor não extrapole os limites definidos
    if (adc_value < ADC_MIN_LEITURA) {
        adc_value = ADC_MIN_LEITURA;
    }
    if (adc_value > ADC_MAX_LEITURA) {
        adc_value = ADC_MAX_LEITURA;
    }

    // Calcula a porcentagem
    // Use float para a divisão para obter um resultado mais preciso e depois converta para int
    int porcentagem = (int)(((float)(adc_value - ADC_MIN_LEITURA) / (ADC_MAX_LEITURA - ADC_MIN_LEITURA)) * 100.0f);

    // Garante que a porcentagem esteja entre 0 e 100
    if (porcentagem < 0) {
        porcentagem = 0;
    }
    if (porcentagem > 100) {
        porcentagem = 100;
    }
    return porcentagem;
}

int aciona_bomba_com_base_no_nivel_agua(int nivel_agua){
    if (nivel_agua <= limite_minimo_nivel_agua){
        gpio_put(RELE_PIN,0); // ativa bomba com rele
    }
    else if (nivel_agua >= limite_maximo_nivel_agua){
        gpio_put(RELE_PIN,1); // desativa bomba com rele
    }
    
    
}

// Explicação: Esta é uma string constante que contém todo o código HTML, CSS e JavaScript da sua página web. 
//Quando um cliente (navegador) acessa o endereço IP do seu RP2040, este código HTML é enviado de volta, permitindo que o navegador renderize a interface.
// HTML: Define a estrutura da página (títulos, parágrafos, botões).
// CSS (<style>): Define o estilo visual da página (cores, tamanhos, layout).
// JavaScript (<script>): Contém a lógica interativa da página:
// sendCommand(cmd): Envia comandos para o RP2040 (ex: /led/on, /led/off) usando fetch.
// atualizar(): Busca o estado atual do LED, joystick e botões do RP2040 via /estado e atualiza a interface (texto e barras de progresso).
// setInterval(atualizar, 500): Chama a função atualizar a cada 500 milissegundos para manter a interface atualizada.
const char HTML_BODY[] =
    "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Controle do LED</title>"
    "<style>"
    "body { font-family: sans-serif; text-align: center; padding: 10px; margin: 0; background:rgb(0, 0, 0); }"
    ".botao { font-size: 20px; padding: 10px 30px; margin: 10px; border: none; border-radius: 8px; }"
    ".on { background: #4CAF50; color: white; }"
    ".off { background: #f44336; color: white; }"
    ".barra { width: 30%; background: #ddd; border-radius: 6px; overflow: hidden; margin: 0 auto 15px auto; height: 20px; }"

    ".preenchimento { height: 100%; transition: width 0.3s ease; }"
    "#barra_x { background: #2196F3; }"
    ".label { font-weight: bold; margin-bottom: 5px; display: block; }"
    ".bolinha { width: 20px; height: 20px; border-radius: 50%; display: inline-block; margin-left: 10px; background: #ccc; transition: background 0.3s ease; }"
    "@media (max-width: 600px) { .botao { width: 80%; font-size: 18px; } }"
    "</style>"
    "<script>"
    "function sendCommand(cmd) { fetch('/led/' + cmd); }"
    "function atualizar() {"
    "  fetch('/estado').then(res => res.json()).then(data => {"
    "    document.getElementById('estado').innerText = data.bomba_rele ? 'Ligado' : 'Desligado';"
    "    document.getElementById('valor_potenciometro').innerText = data.nivel_agua;"
    "    document.getElementById('botao').innerText = data.botao ? 'Pressionado' : 'Solto';"
    "    document.getElementById('joy').innerText = data.joy ? 'Pressionado' : 'Solto';"
    "    document.getElementById('bolinha_a').style.background = data.botao ? '#2126F3' : '#ccc';"
    "    document.getElementById('bolinha_joy').style.background = data.joy ? '#4C7F50' : '#ccc';"
    "    document.getElementById('barra_x').style.width = data.nivel_agua + '%';"
    "  });"
    "}"
    "setInterval(atualizar, 500);"
    "</script></head><body>"

    "<h1>Controle do Nivel de água no Reservatório</h1>"

    "<p>Estado da Bomba de água: <span id='estado'>--</span></p>"

    "<p class='label'>Nivel de água: <span id='valor_potenciometro'>--</span>%</p>"
    "<div class='barra'><div id='barra_x' class='preenchimento'></div></div>"


    "<p class='label'>Botão A: <span id='botao'>--</span> <span id='bolinha_a' class='bolinha'></span></p>"
    "<p class='label'>Botão do Joystick: <span id='joy'>--</span> <span id='bolinha_joy' class='bolinha'></span></p>"

    "<button class='botao on' onclick=\"sendCommand('on')\">Aumentar nível</button>"
    "<button class='botao off' onclick=\"sendCommand('off')\">Diminuir nível</button>"

    "</body></html>";


// Explicação: Esta estrutura é usada para gerenciar o estado de cada conexão HTTP. Cada vez que um navegador 
// faz uma requisição, uma instância desta estrutura é criada para armazenar a resposta que o Pico vai enviar 
// de volta e controlar quantos bytes já foram enviados. 
struct http_state
{
    char response[4096]; // Buffer para armazenar a resposta HTTP a ser enviada
    size_t len;          // Tamanho da resposta em bytes
    size_t sent;         // Quantidade de bytes já enviados
};



// Explicação: Esta função é um callback (função que é chamada quando um evento acontece) do lwIP. 
// Ela é invocada sempre que uma parte dos dados da resposta HTTP é enviada com sucesso. 
// A função verifica se todos os dados foram transmitidos; se sim, ela fecha a conexão TCP e libera a memória alocada para o estado da requisição.

static err_t http_sent(void *arg, struct tcp_pcb *tpcb, u16_t len){
    struct http_state *hs = (struct http_state *)arg; // Recupera o estado da requisição
    hs->sent += len;                                  // Adiciona o número de bytes recém-enviados
    if (hs->sent >= hs->len)                          // Se todos os bytes da resposta foram enviados
    {
        tcp_close(tpcb);                              // Fecha a conexão TCP
        free(hs);                                     // Libera a memória alocada para o estado da requisição
    }  
    return ERR_OK;                                    // Retorna OK
}



// Explicação: Esta é a função central do servidor HTTP. 
// Ela é um callback que é chamada sempre que o Pico recebe dados em uma conexão TCP (ou seja, uma requisição HTTP).
// Ele primeiro verifica se a conexão foi encerrada.
// Aloca memória para http_state para gerenciar a resposta.
// Analisa a requisição recebida (ex: GET /led/on, GET /estado, GET /).
// Com base na requisição, ele executa a ação correspondente (ligar/desligar LED, ler joystick/botões).
// Monta a resposta HTTP (HTTP/1.1 200 OK, Content-Type, Content-Length e o corpo da resposta, que pode ser texto simples, JSON ou HTML).
// Usa tcp_write e tcp_output para enviar a resposta de volta ao cliente.
// Libera a memória alocada para a requisição.




static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (!p) // Se não houver payload (conexão encerrada pelo cliente)
    {
        tcp_close(tpcb); // Fecha a conexão TCP
        return ERR_OK;
    }

    char *req = (char *)p->payload; // Pega o conteúdo da requisição HTTP (ex: "GET /led/on HTTP/1.1")
    struct http_state *hs = malloc(sizeof(struct http_state)); // Aloca memória para o estado da requisição
    if (!hs) // Se a alocação falhar
    {
        pbuf_free(p); // Libera o buffer da requisição
        tcp_close(tpcb);  // Fecha a conexão
        return ERR_MEM; // Retorna erro de memória
    }
    hs->sent = 0; // Inicializa o contador de bytes enviados para esta requisição

    // Lógica para lidar com as diferentes URLs/requisições
    // if (strstr(req, "GET /led/on")){ // Se a requisição for para ligar o LED
    //     gpio_put(LED_PIN, 1); // Liga o led
    //     const char *txt = "Ligado";
    //     hs->len = snprintf(hs->response, sizeof(hs->response),
    //                        // Cabeçalhos HTTP para resposta de texto puro
    //                        "HTTP/1.1 200 OK\r\n"
    //                        "Content-Type: text/plain\r\n"
    //                        "Content-Length: %d\r\n"
    //                        "Connection: close\r\n"
    //                        "\r\n"
    //                        "%s",
    //                        (int)strlen(txt), txt); // Formata a resposta com o texto "Ligado"
    // }
    // else if (strstr(req, "GET /led/off")){ // Se a requisição for para desligar o LED
    //     gpio_put(LED_PIN, 0); // Desliga o led
    //     const char *txt = "Desligado";
    //     hs->len = snprintf(hs->response, sizeof(hs->response),
    //                        // Cabeçalhos HTTP para resposta de texto puro
    //                        "HTTP/1.1 200 OK\r\n"
    //                        "Content-Type: text/plain\r\n"
    //                        "Content-Length: %d\r\n"
    //                        "Connection: close\r\n"
    //                        "\r\n"
    //                        "%s",
    //                        (int)strlen(txt), txt); // Formata a resposta com o texto "Desligado"
    // }
    if (strstr(req, "GET /estado")){  // Se a requisição for para obter o estado dos sensores/LED
        adc_select_input(2); // <--- MUDE AQUI! Seleciona o canal ADC 2 para o potenciômetro da boia
        uint16_t valor_potenciometro_lido = adc_read(); // Armazena o valor do potenciômetro

        // Converte para porcentagem
        int nivel_porcentagem = map_to_percentage(valor_potenciometro_lido);

        int botao = !gpio_get(BOTAO_A); // Lê o estado do Botão A (negado, pois pull-up)
        int joy = !gpio_get(BOTAO_JOY); // Lê o estado do Botão do Joystick (negado, pois pull-up)

        char json_payload[96]; // Buffer para a string JSON
        // Envia o valor_bruto_potenciometro como 'x' E o nivel_porcentagem como um novo campo 'nivel_agua'
        // Ou, para simplificar e aproveitar o que já está na página, você pode enviar direto o nível_porcentagem em 'x'
        int json_len = snprintf(json_payload, sizeof(json_payload),
                                 "{\"led\":%d,\"botao\":%d,\"joy\":%d,\"nivel_agua\":%d}\r\n",
                                 !gpio_get(RELE_PIN), botao, joy, nivel_porcentagem); // Enviando como 'nivel_agua'

        printf("[DEBUG] JSON: %s\n", json_payload); // Imprime o JSON no console para depuração

        hs->len = snprintf(hs->response, sizeof(hs->response),
                            // Cabeçalhos HTTP para resposta JSON
                           "HTTP/1.1 200 OK\r\n"
                           "Content-Type: application/json\r\n"
                           "Content-Length: %d\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "%s",
                           json_len, json_payload); // Formata a resposta com o JSON
    }
    else{ // Para qualquer outra requisição (geralmente a raiz "/")
        hs->len = snprintf(hs->response, sizeof(hs->response),
                            // Cabeçalhos HTTP para resposta HTML
                           "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/html\r\n"
                           "Content-Length: %d\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "%s",
                           (int)strlen(HTML_BODY), HTML_BODY); // Retorna a página HTML completa
    }

    tcp_arg(tpcb, hs);// Associa o estado da requisição (hs) à conexão TCP
    tcp_sent(tpcb, http_sent); // Registra o callback a ser chamado quando os dados forem enviados

    tcp_write(tpcb, hs->response, hs->len, TCP_WRITE_FLAG_COPY); // Escreve a resposta no buffer TCP
    tcp_output(tpcb); // Envia os dados do buffer TCP

    pbuf_free(p); // Libera o buffer da requisição recebida
    return ERR_OK;
}


// Explicação: Esta é outra função de callback do lwIP. Ela é chamada sempre que uma nova conexão TCP é estabelecida no servidor. 
// Aqui, ela apenas registra a função http_recv para ser chamada quando dados forem recebidos nessa nova conexão.
static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, http_recv); // Registra o callback http_recv para lidar com os dados recebidos nesta nova conexão
    return ERR_OK;
}


// Explicação: Esta função inicializa o servidor HTTP. Ela:
// Cria um novo "Protocol Control Block" (PCB), que é uma estrutura de dados do lwIP que representa um endpoint de conexão TCP.
// Associa o PCB à porta 80 (porta padrão para HTTP).
// Coloca o PCB em modo de "escuta", esperando por conexões de entrada.
// Registra a função connection_callback para ser chamada sempre que um cliente tentar se conectar.

static void start_http_server(void)
{
    struct tcp_pcb *pcb = tcp_new();// Cria um novo bloco de controle de protocolo TCP
    if (!pcb)
    {
        printf("Erro ao criar PCB TCP\n");// Erro se não conseguir criar
        return;
    }
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK) // Liga o servidor na porta 80 (porta padrão HTTP)
    {
        printf("Erro ao ligar o servidor na porta 80\n");  // Erro se não conseguir ligar
        return;
    }
    pcb = tcp_listen(pcb);  // Coloca o PCB no modo de escuta (listening)
    tcp_accept(pcb, connection_callback); // Registra o callback para novas conexões
    printf("Servidor HTTP rodando na porta 80...\n"); // Mensagem de sucesso
}



#include "pico/bootrom.h"
#define BOTAO_B 6
void gpio_irq_handler(uint gpio, uint32_t events)
{
    reset_usb_boot(0, 0);
}

int main()
{
    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_pull_up(BOTAO_B);
    gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    stdio_init_all();
    sleep_ms(2000);

    // gpio_init(LED_PIN);
    // gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(RELE_PIN);
    gpio_set_dir(RELE_PIN, GPIO_OUT);
    gpio_put(RELE_PIN, 1);

    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);

    gpio_init(BOTAO_JOY);
    gpio_set_dir(BOTAO_JOY, GPIO_IN);
    gpio_pull_up(BOTAO_JOY);

    adc_init();
    adc_gpio_init(POTENCIOMETRO_BOIA);

    i2c_init(I2C_PORT_DISP, 400 * 1000);
    gpio_set_function(I2C_SDA_DISP, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_DISP, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_DISP);
    gpio_pull_up(I2C_SCL_DISP);

    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT_DISP);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "Iniciando Wi-Fi", 0, 0);
    ssd1306_draw_string(&ssd, "Aguarde...", 0, 30);    
    ssd1306_send_data(&ssd);

    if (cyw43_arch_init()) // Tenta inicializar o módulo Wi-Fi
    {
        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, "WiFi => FALHA", 0, 0);
        ssd1306_send_data(&ssd);
        return 1; // Retorna erro se falhar
    }

    cyw43_arch_enable_sta_mode(); // Habilita o modo estação (cliente) Wi-Fi
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 30000)) // Tenta conectar à rede Wi-Fi com timeout de 10 segundos
    {
        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, "WiFi => ERRO", 0, 0);
        ssd1306_send_data(&ssd);
        return 1; // Retorna erro se falhar a conexão
    }

    uint8_t *ip = (uint8_t *)&(cyw43_state.netif[0].ip_addr.addr); // Obtém e exibe o endereço IP
    char ip_str[24];
    snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]); // Formata o IP como string    

    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "WiFi => OK", 0, 0);
    ssd1306_draw_string(&ssd, ip_str, 0, 10); // Exibe o IP no display
    ssd1306_send_data(&ssd);

    start_http_server(); // Inicia o servidor HTTP
    char str_nivel[5]; // Buffer para armazenar a string
    bool cor = true;
    while (true)
    {
        cyw43_arch_poll();

        // Leitura dos valores analógicos
        adc_select_input(2);
        uint16_t adc_value_potenciometro = adc_read();

        // Converte para porcentagem para o display OLED
        int nivel_porcentagem_display = map_to_percentage(adc_value_potenciometro);
        sprintf(str_nivel, "%d%%", nivel_porcentagem_display); // Formata com '%'

        ssd1306_fill(&ssd, !cor);                     // Limpa o display
        ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor); // Desenha um retângulo
        ssd1306_line(&ssd, 3, 25, 123, 25, cor);      // Desenha uma linha
        ssd1306_line(&ssd, 3, 37, 123, 37, cor);      // Desenha uma linha

        ssd1306_draw_string(&ssd, "CEPEDI   TIC37", 8, 6); // Desenha uma string
        ssd1306_draw_string(&ssd, "EMBARCATECH", 20, 16);  // Desenha uma string
        ssd1306_draw_string(&ssd, ip_str, 10, 28);
        ssd1306_draw_string(&ssd, "Nivel", 8, 41);           // Desenha uma string
        ssd1306_draw_string(&ssd, str_nivel, 8, 52);                     // Desenha uma string
        ssd1306_rect(&ssd, 52, 90, 8, 8, cor, !gpio_get(BOTAO_JOY)); // Desenha um retângulo
        ssd1306_rect(&ssd, 52, 102, 8, 8, cor, !gpio_get(BOTAO_A));  // Desenha um retângulo
        ssd1306_rect(&ssd, 52, 114, 8, 8, cor, !cor);                // Desenha um retângulo
        ssd1306_send_data(&ssd);                                     // Atualiza o display
        aciona_bomba_com_base_no_nivel_agua(nivel_porcentagem_display);
        sleep_ms(200);
    }

    cyw43_arch_deinit();
    return 0;
}

