#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")
#define BUF_SIZE 1024

// Variáveis Globais de Estado
int is_admin = 0;
char current_user[50] = "";
struct sockaddr_in addr;

// Limpa o buffer do teclado para evitar loops infinitos no scanf
void clear_buffer() { 
    int c; 
    while ((c = getchar()) != '\n' && c != EOF); 
}

// F1: Interface Dinâmica (TUI) com 3 Estados Visuais
void draw_header() {
    system("cls");
    
    // ESTADO 1: VISITANTE (Branco)
    if (current_user[0] == '\0') {
        printf("\033[1;37m"); 
        printf("  _____        _____              _ \n");
        printf(" / ____|      / ____|            | |\n");
        printf("| |     _____| |     ___  _ __ __| |\n");
        printf("| |    |_____| |    / _ \\| '__/ _` |\n");
        printf("| |____      | |___| (_) | | | (_| |\n");
        printf(" \\_____|      \\_____\\___/|_|  \\__,_|\n");
        printf("\033[0m\n");
        printf("====================================================\n");
        printf("         \033[1;37m[?] C-CORD (MODO VISITANTE)\033[0m                 \n");
        printf("====================================================\n");
        printf(" USER: [GUEST] | STATUS: Aguardando Login...\n");
    } 
    // ESTADO 2: ADMINISTRADOR (Vermelho)
    else if (is_admin) {
        printf("\033[1;31m"); 
        printf("  _____        _____              _ \n");
        printf(" / ____|      / ____|            | |\n");
        printf("| |     _____| |     ___  _ __ __| |\n");
        printf("| |    |_____| |    / _ \\| '__/ _` |\n");
        printf("| |____      | |___| (_) | | | (_| |\n");
        printf(" \\_____|      \\_____\\___/|_|  \\__,_|\n");
        printf("\033[0m\n");
        printf("====================================================\n");
        printf("         \033[1;31m[!] MODO ADMINISTRADOR ATIVO\033[0m               \n");
        printf("====================================================\n");
        printf(" USER: [\033[1;33m%s\033[0m] | ROLE: ADMIN\n", current_user);
    } 
    // ESTADO 3: UTILIZADOR NORMAL (Ciano)
    else {
        printf("\033[1;36m"); 
        printf("  _____        _____              _ \n");
        printf(" / ____|      / ____|            | |\n");
        printf("| |     _____| |     ___  _ __ __| |\n");
        printf("| |    |_____| |    / _ \\| '__/ _` |\n");
        printf("| |____      | |___| (_) | | | (_| |\n");
        printf(" \\_____|      \\_____\\___/|_|  \\__,_|\n");
        printf("\033[0m\n");
        printf("====================================================\n");
        printf("         \033[1;36m[~] MODO UTILIZADOR NORMAL\033[0m                 \n");
        printf("====================================================\n");
        printf(" USER: [\033[1;33m%s\033[0m] | ROLE: USER\n", current_user);
    }
    printf("----------------------------------------------------\n");
}

// Função utilitária para enviar comandos e receber resposta sequencial
void call_server(char* cmd) {
    SOCKET fd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
        send(fd, cmd, (int)strlen(cmd), 0);
        char res[BUF_SIZE] = {0};
        recv(fd, res, BUF_SIZE, 0);
        printf("\n\033[1;32m[SERVER]:\033[0m %s\n", res);
    } else {
        printf("\n\033[1;31m[ERRO] Servidor offline ou inacessivel.\033[0m\n");
    }
    closesocket(fd);
    printf("\n Pressione ENTER para voltar ao menu...");
    getchar();
}

int main(int argc, char *argv[]) {
    // Inicialização do Winsock
    WSADATA wsa; 
    WSAStartup(MAKEWORD(2,2), &wsa);
    struct hostent *hp = gethostbyname(argv[1]);
    
    if (argc < 3 || !hp) { 
        printf("Uso: client.exe <IP_DO_SERVIDOR> <PORTO>\n"); 
        return -1; 
    }
    
    addr.sin_family = AF_INET; 
    addr.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
    addr.sin_port = htons(atoi(argv[2]));

    // ==========================================
    // VERIFICAÇÃO INICIAL DO SERVIDOR (Ping de Teste)
    // ==========================================
    system("cls");
    printf("\n A verificar se o servidor esta online no porto %s...\n", argv[2]);
    SOCKET test_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(test_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        printf("\n \033[1;31m[ERRO CRITICO]\033[0m Nao foi possivel contactar o servidor.\n");
        printf(" Verifica se o servidor Linux esta a correr e se o IP/Porto estao corretos.\n\n");
        closesocket(test_fd);
        WSACleanup();
        system("pause");
        return -1;
    }
    closesocket(test_fd);
    printf(" \033[1;32m[SUCESSO]\033[0m Ligacao estabelecida! A iniciar o C-Cord...\n");
    Sleep(1500);

    // ==========================================
    // LOOP PRINCIPAL DO CLIENTE
    // ==========================================
    while(1) {
        draw_header();
        
        // Menu de Visitante
        if (!current_user[0]) {
            printf(" [ 1 ] F3: Fazer Login Seguro\n");
            printf(" [ 0 ] Sair da Aplicacao\n\n Escolha: ");
        } 
        // Menu de Utilizador Autenticado (Exatamente como pediste)
        else {
            printf(" [ 1 ] F4: Ver Estado do Servidor (GET_INFO)\n");
            printf(" [ 2 ] F4: Testar Latencia (ECHO)\n");
            printf(" [ 3 ] Terminar Sessao\n");
            printf(" [ 0 ] Sair da Aplicacao\n\n Escolha: ");
        }

        int opt; 
        if (scanf("%d", &opt) != 1) { 
            clear_buffer(); 
            continue; 
        }
        clear_buffer();

        if (opt == 0) break;

        // ==========================================
        // LÓGICA DAS OPÇÕES
        // ==========================================
        
        // --- FLUXO DO VISITANTE ---
        if (!current_user[0]) {
            if (opt == 1) {
                char u[50], p[50], pl[100]; 
                printf(" Username: "); scanf("%s", u); 
                printf(" Password: "); scanf("%s", p); 
                clear_buffer();
                
                sprintf(pl, "AUTH %s %s", u, p);
                
                SOCKET fd = socket(AF_INET, SOCK_STREAM, 0); 
                if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
                    send(fd, pl, strlen(pl), 0); 
                    char res[100]={0}; 
                    recv(fd, res, 100, 0); 
                    closesocket(fd);
                    
                    if (strncmp(res, "AUTH_SUCCESS", 12) == 0) { 
                        strcpy(current_user, u); 
                        is_admin = (strstr(res, "ADMIN") != NULL); 
                    } else {
                        printf("\n\033[1;31mCredenciais invalidas!\033[0m\n"); 
                        system("pause");
                    }
                } else {
                    printf("\n\033[1;31m[ERRO] Servidor offline.\033[0m\n");
                    system("pause");
                }
            }
        }
        
        // --- FLUXO DO UTILIZADOR AUTENTICADO ---
        else {
            // F4 (GET_INFO)
            if (opt == 1) {
                call_server("GET_INFO");
            }
            // F4 (ECHO)
            else if (opt == 2) {
                char msg[500], pl[600]; 
                printf(" Mensagem: "); 
                fgets(msg, 500, stdin); 
                msg[strcspn(msg, "\n")] = 0; 
                sprintf(pl, "ECHO %s", msg); 
                call_server(pl);
            }
            // LOGOUT
            else if (opt == 3) {
                current_user[0] = '\0'; 
                is_admin = 0; 
            }
        }
    }
    
    WSACleanup(); 
    return 0;
}