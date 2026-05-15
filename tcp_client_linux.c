/*
 * ============================================================================
 * CLIENTE TCP LINUX PARA C-CORD (Clone simplificado do Discord)
 * ============================================================================
 * 
 * Programa: tcp_client_linux.c
 * Descrição: Cliente de rede para conectar a um servidor TCP usando sockets POSIX
 * 
 * Este programa demonstra:
 *   - Uso de sockets POSIX (sys/socket.h, netinet/in.h, etc.)
 *   - Conexão TCP a um servidor remoto
 *   - Autenticação de utilizadores
 *   - Interface de utilizador em terminal (TUI) com cores ANSI
 *   - Comunicação cliente-servidor com protocolo simples
 * 
 * Compilação: gcc -o tcp_client_linux tcp_client_linux.c
 * Utilização: ./tcp_client_linux <IP_SERVIDOR> <PORTO>
 * ============================================================================
 */

/* Cabeçalhos padrão POSIX para rede em Linux */
#include <arpa/inet.h>      /* Conversões de endereços (inet_aton, htons, etc.) */
#include <netdb.h>          /* Resolução de nomes (gethostbyname) */
#include <netinet/in.h>     /* Estruturas de endereço de rede (sockaddr_in) */
#include <stdio.h>          /* Entrada/saída padrão (printf, scanf) */
#include <stdlib.h>         /* Funções gerais (malloc, atoi, exit) */
#include <string.h>         /* Manipulação de strings (strcpy, sprintf, strlen) */
#include <sys/socket.h>     /* API de sockets (socket, connect, send, recv) */
#include <unistd.h>         /* Funções POSIX (sleep, close) */

/* Constante para o tamanho máximo do buffer de comunicação */
#define BUF_SIZE 1024

/*
 * ============================================================================
 * VARIÁVEIS GLOBAIS DE ESTADO
 * ============================================================================
 * Estas variáveis mantêm o estado da sessão do utilizador em toda a execução.
 */

int is_admin = 0;                    /* Inteiro que guarda se utilizador é administrador (0=não, 1=sim) */
char current_user[50] = "";          /* Cadeia de caracteres com o nome do utilizador autenticado */
struct sockaddr_in addr;             /* Estrutura que guarda o endereço do servidor (IP e porto) */


/*
 * ============================================================================
 * FUNÇÃO: clear_buffer()
 * ============================================================================
 * 
 * O que esta função faz:
 *   Limpa o buffer de entrada do teclado para evitar que caracteres antigos
 *   fiquem no buffer e causem problemas na próxima leitura com scanf().
 * 
 * Por quê é importante?
 *   Quando o utilizador digita algo e depois pressiona ENTER, o scanf() pode
 *   deixar o '\n' no buffer. Se não limparmos, o próximo scanf() lê esse '\n'
 *   e ignora a entrada do utilizador.
 * 
 * Como funciona?
 *   - Faz um loop enquanto houver caracteres no buffer
 *   - Lê cada carácter com getchar()
 *   - Para quando encontra '\n' (Enter) ou EOF (fim do ficheiro)
 * 
 * ============================================================================
 */
void clear_buffer() { 
    int c;                          /* Variável para guardar cada carácter lido */
    while ((c = getchar()) != '\n' && c != EOF);  /* Loop até encontrar nova linha ou fim */
}


/*
 * ============================================================================
 * FUNÇÃO: draw_header()
 * ============================================================================
 * 
 * O que esta função faz:
 *   Desenha o cabeçalho da interface do utilizador (menu) com cores ANSI e
 *   altera a aparência conforme o estado de autenticação (3 modos diferentes).
 * 
 * Os 3 modos de interface:
 * 
 *   1. MODO VISITANTE (Branco - código 1;37m):
 *      - Utilizador não está autenticado (current_user vazio)
 *      - Mostra apenas opção de fazer login
 * 
 *   2. MODO ADMINISTRADOR (Vermelho - código 1;31m):
 *      - Utilizador fez login com conta de admin (is_admin = 1)
 *      - Mostra menus específicos de administrador
 * 
 *   3. MODO UTILIZADOR NORMAL (Ciano - código 1;36m):
 *      - Utilizador fez login com conta normal (is_admin = 0)
 *      - Mostra menus para utilizadores normais
 * 
 * Como funciona?
 *   - Limpa a tela com system("clear")
 *   - Mostra o logo ASCII do C-Cord
 *   - Usa cores ANSI (\033[...m) para colorir o texto
 *   - Verifica o estado do utilizador e mostra a interface apropriada
 * 
 * Cores ANSI explicadas:
 *   \033[1;37m = Branco brilhante (VISITANTE)
 *   \033[1;31m = Vermelho brilhante (ADMINISTRADOR)
 *   \033[1;36m = Ciano brilhante (UTILIZADOR)
 *   \033[0m = Reposição de cor (volta ao normal)
 * 
 * ============================================================================
 */
void draw_header() {
    system("clear");    /* Limpa toda a tela antes de desenhar */
    
    /* ESTADO 1: MODO VISITANTE (sem autenticação) */
    if (current_user[0] == '\0') {
        printf("\033[1;37m");  /* Ativa cor branca brilhante */
        printf("  _____        _____              _ \n");
        printf(" / ____|      / ____|            | |\n");
        printf("| |     _____| |     ___  _ __ __| |\n");
        printf("| |    |_____| |    / _ \\| '__/ _` |\n");
        printf("| |____      | |___| (_) | | | (_| |\n");
        printf(" \\_____|      \\_____\\___/|_|  \\__,_|\n");
        printf("\033[0m\n");                      /* Reposição de cor */
        printf("====================================================\n");
        printf("         \033[1;37m[?] C-CORD (MODO VISITANTE)\033[0m                   \n");
        printf("====================================================\n");
        printf(" UTILIZADOR: [VISITANTE] | ESTADO: À espera de autenticação...\n");
    } 
    /* ESTADO 2: MODO ADMINISTRADOR (utilizador com privilégios) */
    else if (is_admin) {
        printf("\033[1;31m");  /* Ativa cor vermelha brilhante */
        printf("  _____        _____              _ \n");
        printf(" / ____|      / ____|            | |\n");
        printf("| |     _____| |     ___  _ __ __| |\n");
        printf("| |    |_____| |    / _ \\| '__/ _` |\n");
        printf("| |____      | |___| (_) | | | (_| |\n");
        printf(" \\_____|      \\_____\\___/|_|  \\__,_|\n");
        printf("\033[0m\n");                      /* Reposição de cor */
        printf("====================================================\n");
        printf("         \033[1;31m[!] MODO ADMINISTRADOR ACTIVO\033[0m             \n");
        printf("====================================================\n");
        printf(" UTILIZADOR: [\033[1;33m%s\033[0m] | PAPEL: ADMINISTRADOR\n", current_user);
    } 
    /* ESTADO 3: MODO UTILIZADOR NORMAL (utilizador autenticado sem privilégios) */
    else {
        printf("\033[1;36m");  /* Ativa cor ciana brilhante */
        printf("  _____        _____              _ \n");
        printf(" / ____|      / ____|            | |\n");
        printf("| |     _____| |     ___  _ __ __| |\n");
        printf("| |    |_____| |    / _ \\| '__/ _` |\n");
        printf("| |____      | |___| (_) | | | (_| |\n");
        printf(" \\_____|      \\_____\\___/|_|  \\__,_|\n");
        printf("\033[0m\n");                      /* Reposição de cor */
        printf("====================================================\n");
        printf("         \033[1;36m[~] MODO UTILIZADOR NORMAL\033[0m                 \n");
        printf("====================================================\n");
        printf(" UTILIZADOR: [\033[1;33m%s\033[0m] | PAPEL: UTILIZADOR\n", current_user);
    }
    printf("----------------------------------------------------\n");
}


/*
 * ============================================================================
 * FUNÇÃO: call_server(char* cmd)
 * ============================================================================
 * 
 * O que esta função faz:
 *   Envia um comando ao servidor TCP e recebe a resposta de forma sequencial.
 *   Esta é uma função utilitária para simplificar a comunicação com o servidor.
 * 
 * Parâmetros:
 *   cmd - Cadeia de caracteres contendo o comando a enviar (ex: "GET_INFO", "ECHO mensagem")
 * 
 * Processo de rede passo a passo:
 *   
 *   1. CRIAR SOCKET:
 *      - socket(AF_INET, SOCK_STREAM, 0) cria um socket TCP/IP novo
 *      - AF_INET = protocolo IPv4
 *      - SOCK_STREAM = conexão TCP (garantida, ordenada)
 *      - Verificamos se foi criado com sucesso (fd >= 0)
 * 
 *   2. CONECTAR AO SERVIDOR:
 *      - connect() estabelece ligação TCP ao servidor no endereço "addr"
 *      - sizeof(addr) = tamanho da estrutura de endereço
 *      - Se == 0, ligação bem-sucedida
 *      - Se != 0, ligação falhou (servidor offline?)
 * 
 *   3. ENVIAR COMANDO:
 *      - send() envia os dados do comando através do socket
 *      - strlen(cmd) calcula o número de bytes a enviar
 *      - Último parâmetro (0) = sem flags especiais
 * 
 *   4. RECEBER RESPOSTA:
 *      - recv() aguarda dados do servidor
 *      - Guarda até BUF_SIZE bytes em "res"
 *      - Bloqueia até receber dados (ou timeout)
 * 
 *   5. FECHAR CONEXÃO:
 *      - close(fd) fecha o socket e liberta o descritor
 * 
 * ============================================================================
 */
void call_server(char* cmd) {
    /* PASSO 1: Criar um novo socket para comunicação TCP */
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    
    /* Verificar se o socket foi criado com sucesso */
    if (fd < 0) {
        printf("\n\033[1;31m[ERRO] Não foi possível criar socket.\033[0m\n");
        printf("\n Pressione ENTER para voltar ao menu...");
        getchar();
        return;
    }

    /* PASSO 2: Conectar ao servidor usando o endereço guardado em "addr" */
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
        /* Ligação bem-sucedida! Proceder com comunicação */
        
        /* PASSO 3: Enviar o comando ao servidor */
        send(fd, cmd, strlen(cmd), 0);
        
        /* PASSO 4: Receber a resposta do servidor */
        char res[BUF_SIZE] = {0};  /* Buffer inicializado com zeros */
        recv(fd, res, BUF_SIZE, 0);
        
        /* Mostrar a resposta ao utilizador em verde */
        printf("\n\033[1;32m[SERVIDOR]:\033[0m %s\n", res);
    } else {
        /* Ligação falhou - servidor está offline ou inacessível */
        printf("\n\033[1;31m[ERRO] Servidor desconectado ou inacessível.\033[0m\n");
    }
    
    /* PASSO 5: Fechar o socket para libertar recursos */
    close(fd);
    
    printf("\n Pressione ENTER para voltar ao menu...");
    getchar();
}


/*
 * ============================================================================
 * FUNÇÃO: main(int argc, char *argv[])
 * ============================================================================
 * 
 * O que esta função faz:
 *   É o ponto de entrada do programa. Trata da inicialização, ligação ao
 *   servidor e controla o loop principal do cliente.
 * 
 * Parâmetros de linha de comando:
 *   argc - Número de argumentos (deve ser 3: programa, IP, porto)
 *   argv[0] - Nome do programa
 *   argv[1] - IP ou nome do servidor (ex: "127.0.0.1" ou "localhost")
 *   argv[2] - Número do porto (ex: "10000")
 * 
 * Fluxo geral:
 *   1. Verificar argumentos de linha de comando
 *   2. Resolver o nome do servidor (DNS) para obter endereço IP
 *   3. Preparar estrutura de endereço do servidor (IP + porto)
 *   4. Fazer teste de ligação inicial ao servidor
 *   5. Loop principal: mostrar menu, receber escolha, processar ação
 *   6. Finalizar programa quando utilizador escolhe sair
 * 
 * ============================================================================
 */
int main(int argc, char *argv[]) {
    
    /*
     * ========================================
     * FASE 1: VALIDAÇÃO DE ARGUMENTOS
     * ========================================
     */
    
    /* Verificar se o utilizador forneceu os 3 argumentos necessários */
    if (argc < 3) { 
        printf("Utilização: client <IP_DO_SERVIDOR> <PORTO>\n"); 
        printf("Exemplo: client 127.0.0.1 10000\n");
        return -1;  /* Sair com código de erro */
    }

    /*
     * ========================================
     * FASE 2: RESOLUÇÃO DE NOME (DNS)
     * ========================================
     * Converte nome do servidor (ex: "localhost") em endereço IP
     */
    
    /* gethostbyname() tenta resolver o nome para endereço IP */
    struct hostent *hp = gethostbyname(argv[1]);
    
    if (!hp) {
        /* Se falhar, mostrar erro */
        printf("Utilização: client <IP_DO_SERVIDOR> <PORTO>\n");
        return -1;
    }

    /*
     * ========================================
     * FASE 3: PREPARAÇÃO DO ENDEREÇO DO SERVIDOR
     * ========================================
     * Preencher a estrutura sockaddr_in com IP e porto
     */
    
    addr.sin_family = AF_INET;      /* Usar protocolo IPv4 */
    /* Copiar endereço IP resolvido para a estrutura usando memcpy (mais seguro) */
    memcpy(&addr.sin_addr, hp->h_addr_list[0], hp->h_length);
    /* Converter porto de número para formato de rede (big-endian) com htons() */
    addr.sin_port = htons(atoi(argv[2]));

    /*
     * ========================================
     * FASE 4: TESTE INICIAL DE LIGAÇÃO AO SERVIDOR
     * ========================================
     * Verificar se o servidor está online antes de começar
     */
    
    system("clear");    /* Limpar tela */
    printf("\n A verificar se o servidor está online no porto %s...\n", argv[2]);
    
    /* Criar um socket temporário para teste */
    int test_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (test_fd < 0) {
        printf("\n \033[1;31m[ERRO CRÍTICO]\033[0m Não foi possível criar socket.\n");
        return -1;
    }

    /* Tentar conectar ao servidor */
    if (connect(test_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        /* Ligação falhou */
        printf("\n \033[1;31m[ERRO CRÍTICO]\033[0m Não foi possível contactar o servidor.\n");
        printf(" Por favor, verifique se o servidor está em execução e se o IP/Porto estão correctos.\n\n");
        close(test_fd);
        return -1;
    }
    
    /* Ligação bem-sucedida! */
    close(test_fd);
    printf(" \033[1;32m[SUCESSO]\033[0m Ligação estabelecida! A iniciar C-Cord...\n");
    sleep(1);   /* Aguardar 1 segundo para o utilizador ver a mensagem */

    /*
     * ========================================
     * FASE 5: LOOP PRINCIPAL DO CLIENTE
     * ========================================
     * Este loop continua até o utilizador escolher sair
     */
    
    while(1) {
        /* Mostrar a interface com o estado actual */
        draw_header();
        
        /* MENU DO VISITANTE (sem autenticação) */
        if (!current_user[0]) {
            printf(" [ 1 ] F3: Autenticação Segura\n");
            printf(" [ 0 ] Sair da Aplicação\n\n Escolha: ");
        } 
        /* MENU DO UTILIZADOR AUTENTICADO */
        else {
            printf(" [ 1 ] F4: Obter Info do Servidor (GET_INFO)\n");
            printf(" [ 2 ] F4: Testar Latência (ECHO)\n");
            printf(" [ 3 ] Terminar Sessão\n");
            printf(" [ 0 ] Sair da Aplicação\n\n Escolha: ");
        }

        /* Ler a opção do utilizador */
        int opt; 
        if (scanf("%d", &opt) != 1) { 
            /* Se scanf falhar (entrada inválida), limpar buffer e tentar novamente */
            clear_buffer(); 
            continue; 
        }
        clear_buffer();  /* Limpar '\n' deixado por scanf */

        /* Se utilizador escolher 0, sair do loop */
        if (opt == 0) break;

        /*
         * ==========================================
         * PROCESSAMENTO DAS OPÇÕES DO MENU
         * ==========================================
         */
        
        /* FLUXO DO VISITANTE (sem autenticação) */
        if (!current_user[0]) {
            if (opt == 1) {
                /* Opção de Login */
                
                /* Declarar variáveis para guardar dados de autenticação */
                char u[50], p[50], pl[150];  /* u=utilizador, p=password, pl=payload (aumentado) */
                
                /* Pedir nome de utilizador */
                printf(" Nome de utilizador: "); 
                scanf("%s", u); 
                
                /* Pedir palavra-passe */
                printf(" Palavra-passe: "); 
                scanf("%s", p); 
                clear_buffer();  /* Limpar buffer após scanf */

                /* Construir comando AUTH com formato: "AUTH <utilizador> <password>" */
                sprintf(pl, "AUTH %s %s", u, p);

                /* 
                 * PASSO 1: Criar socket para comunicação com servidor
                 * AF_INET = IPv4
                 * SOCK_STREAM = TCP (conexão garantida)
                 * 0 = protocolo por defeito
                 */
                int fd = socket(AF_INET, SOCK_STREAM, 0);
                if (fd < 0) {
                    printf("\n\033[1;31m[ERRO] Não foi possível criar socket.\033[0m\n");
                    sleep(1);
                    continue;
                }

                /* 
                 * PASSO 2: Conectar ao servidor
                 * addr = estrutura com IP e porto do servidor
                 * sizeof(addr) = tamanho da estrutura
                 * Retorna 0 se bem-sucedido, -1 se falhou
                 */
                if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
                    /* 
                     * PASSO 3: Enviar o comando de autenticação
                     * send(file_descriptor, dados, quantidade_bytes, flags)
                     */
                    send(fd, pl, strlen(pl), 0); 
                    
                    /* 
                     * PASSO 4: Receber resposta do servidor
                     * recv(file_descriptor, buffer, tamanho_máximo, flags)
                     * Bloqueia até receber dados
                     */
                    char res[100] = {0}; 
                    recv(fd, res, 100, 0); 
                    
                    /* PASSO 5: Fechar socket para libertar recursos */
                    close(fd);

                    /* 
                     * PASSO 6: Processar resposta do servidor
                     * Esperamos: "AUTH_SUCCESS:ADMIN", "AUTH_SUCCESS:USER" ou "AUTH_FAIL"
                     */
                    if (strncmp(res, "AUTH_SUCCESS", 12) == 0) { 
                        /* Autenticação bem-sucedida! */
                        strcpy(current_user, u);  /* Guardar nome do utilizador */
                        /* Verificar se é administrador (procurar "ADMIN" na resposta) */
                        is_admin = (strstr(res, "ADMIN") != NULL); 
                    } else {
                        /* Autenticação falhou */
                        printf("\n\033[1;31mCredenciais inválidas!\033[0m\n"); 
                        sleep(1);
                    }
                } else {
                    /* Erro ao conectar ao servidor */
                    printf("\n\033[1;31m[ERRO] Servidor desconectado.\033[0m\n");
                    sleep(1);
                }
            }
        }
        
        /* FLUXO DO UTILIZADOR AUTENTICADO */
        else {
            /* Opção 1: Obter informações do servidor (GET_INFO) */
            if (opt == 1) {
                call_server("GET_INFO");
            }
            /* Opção 2: Testar latência com ECHO */
            else if (opt == 2) {
                char msg[500], pl[600];  /* msg=mensagem, pl=payload com comando */
                
                /* Pedir mensagem ao utilizador */
                printf(" Mensagem: "); 
                fgets(msg, 500, stdin);  /* Usar fgets para permitir espaços */
                msg[strcspn(msg, "\n")] = 0;  /* Remover o '\n' do fim da string */
                
                /* Construir comando ECHO com formato: "ECHO <mensagem>" */
                sprintf(pl, "ECHO %s", msg); 
                call_server(pl);  /* Enviar ao servidor e processar resposta */
            }
            /* Opção 3: Terminar sessão (logout) */
            else if (opt == 3) {
                current_user[0] = '\0';  /* Limpar nome de utilizador (vazio) */
                is_admin = 0;            /* Remover privilégios de admin */
            }
        }
    }
    
    return 0;  /* Sair do programa com sucesso */
}
