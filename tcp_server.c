#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define SERVER_PORT 10000
#define BUF_SIZE 1024
#define USERS_FILE "users.txt"
#define LOG_FILE "logs.txt"

// Variáveis Globais
time_t start_time;
const char *VERSAO_SERVIDOR = "1.0-Fase1"; // Variável da versão do servidor

// ==========================================
// MÓDULO DE LOGS
// ==========================================
void guardar_log(const char *mensagem, int tipo) {
    FILE *f = fopen(LOG_FILE, "a");
    char data[64];
    if (f) {
        time_t agora = time(NULL);
        struct tm *t = localtime(&agora);
        strftime(data, sizeof(data), "%Y-%m-%d %H:%M:%S", t);
        fprintf(f, "[%s] %s\n", data, mensagem);
        fclose(f);
    }
    
    // Mostra no terminal com cores: 1=Verde(Sucesso), 3=Vermelho(Erro), Outro=Ciano(Info)
    if(tipo == 1) printf(" \033[1;32m[OK]\033[0m    | %s\n", mensagem);
    else if(tipo == 3) printf(" \033[1;31m[ERRO]\033[0m  | %s\n", mensagem);
    else printf(" \033[1;36m[INFO]\033[0m  | %s\n", mensagem);
}

// ==========================================
// INTERFACE DO SERVIDOR (TUI)
// ==========================================
void desenhar_cabecalho_servidor() {
    system("clear");
    printf("\033[1;36m"); // Cor Ciano
    printf("   ____         ____ ___  ____  ____    \n");
    printf("  / ___|       / ___/ _ \\|  _ \\|  _ \\   \n");
    printf(" | |     ____ | |  | | | | |_) | | | |  \n");
    printf(" | |___ |____|| |__| |_| |  _ <| |_| |  \n");
    printf("  \\____|       \\____\\___/|_| \\_\\____/   \n");
    printf("\033[0m\n");
    printf("========================================================================\n");
    printf("                  C-CORD SERVER (Apenas Fase 1 - F3/F4)                 \n");
    printf("========================================================================\n");
    printf(" STATUS: \033[1;32mONLINE\033[0m | PORTO: %d | BD: %s | VERSAO: %s\n", SERVER_PORT, USERS_FILE, VERSAO_SERVIDOR);
    printf("------------------------------------------------------------------------\n");
    printf(" LIVE FEED DE ATIVIDADE:\n");
}

// ==========================================
// LÓGICA DE AUTENTICAÇÃO (F3)
// ==========================================
int check_auth(const char *username, const char *password, char *role) {
    FILE *file = fopen(USERS_FILE, "r");
    if (!file) {
        guardar_log("Ficheiro users.txt nao encontrado!", 3);
        return 0;
    }
    
    char line[256], u[50], p[50], r[20];
    while (fgets(line, sizeof(line), file)) {
        // Lê as 3 partes separadas por dois pontos (:)
        if (sscanf(line, "%[^:]:%[^:]:%s", u, p, r) == 3) {
            if (strcmp(u, username) == 0 && strcmp(p, password) == 0) {
                strcpy(role, r); 
                fclose(file); 
                return 1; // Sucesso
            }
        }
    }
    fclose(file); 
    return 0; // Falhou
}

// ==========================================
// MOTOR DO SERVIDOR (Sequencial)
// ==========================================
int main() {
    int fd, client;
    struct sockaddr_in addr;
    char buffer[BUF_SIZE];

    start_time = time(NULL);
    fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1; 
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    addr.sin_family = AF_INET; 
    addr.sin_addr.s_addr = INADDR_ANY; 
    addr.sin_port = htons(SERVER_PORT);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Erro ao abrir o porto");
        exit(-1);
    }
    listen(fd, 5);
    
    desenhar_cabecalho_servidor();
    guardar_log("Servidor inicializado e a escuta de clientes.", 1);

    // Loop Sequencial / Bloqueante
    while (1) {
        struct sockaddr_in cli_addr; 
        socklen_t size = sizeof(cli_addr);
        client = accept(fd, (struct sockaddr *)&cli_addr, &size);
        
        if (client > 0) {
            memset(buffer, 0, BUF_SIZE);
            if (read(client, buffer, BUF_SIZE - 1) > 0) {
                char response[BUF_SIZE] = "", log_msg[BUF_SIZE] = "";
                int log_type = 0;

                // ----------------------------------------------------
                // F3: AUTENTICAÇÃO
                // ----------------------------------------------------
                if (strncmp(buffer, "AUTH ", 5) == 0) {
                    char u[50], p[50], r[20]; 
                    sscanf(buffer + 5, "%s %s", u, p);
                    
                    if (check_auth(u, p, r) == 1) { 
                        sprintf(response, "AUTH_SUCCESS:%s", r); 
                        sprintf(log_msg, "Login com sucesso: '%s' (%s)", u, r); 
                        log_type = 1; 
                    } else { 
                        strcpy(response, "AUTH_FAIL"); 
                        sprintf(log_msg, "Falha de login: utilizador '%s'", u); 
                        log_type = 3; 
                    }
                }
                
                // ----------------------------------------------------
                // F4: INFO (Uptime e Versão)
                // ----------------------------------------------------
                else if (strcmp(buffer, "GET_INFO") == 0) {
                    int up = (int)difftime(time(NULL), start_time);
                    sprintf(response, "C-Cord Server v%s | Uptime: %02dh:%02dm:%02ds", VERSAO_SERVIDOR, up/3600, (up%3600)/60, up%60);
                    sprintf(log_msg, "Processado pedido GET_INFO (Versao %s)", VERSAO_SERVIDOR);
                }
                
                // ----------------------------------------------------
                // F4: ECHO (Latência/Eco)
                // ----------------------------------------------------
                else if (strncmp(buffer, "ECHO ", 5) == 0) {
                    char msg_recebida[BUF_SIZE];
                    strcpy(msg_recebida, buffer + 5); 
                    
                    sprintf(response, "Servidor Ecoa: %s", msg_recebida); 
                    sprintf(log_msg, "ECHO: Recebeu '%s' e devolveu com sucesso", msg_recebida);
                }
                
                // ----------------------------------------------------
                // COMANDO INVÁLIDO / DESCONHECIDO
                // ----------------------------------------------------
                else {
                    strcpy(response, "CMD_INVALID");
                    strcpy(log_msg, "Comando desconhecido recebido (Bloqueado)"); 
                    log_type = 3;
                }

                // Guarda o registo e responde ao cliente
                guardar_log(log_msg, log_type);
                write(client, response, strlen(response));
            }
            // Modelo Sequencial: A ligação é encerrada imediatamente após a resposta
            close(client); 
        }
    }
    return 0;
}
