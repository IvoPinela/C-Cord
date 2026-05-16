/*
 * ============================================================================
 * SERVIDOR TCP C-CORD — ETAPA 2 (F3, F4, F5, F6, F7, F8)
 * ============================================================================
 *
 * Compilação : gcc tcp_server_v2.c -o server
 * Execução   : ./server
 * Porto      : 10000
 * BD de Utilizadores: users.txt
 * BD de Mensagens    : inbox.txt
 * Ficheiro de Logs   : logs.txt
 *
 * Protocolo suportado:
 *   AUTH <user> <pass>              → AUTH_SUCCESS:ADMIN | AUTH_SUCCESS:USER |
 * AUTH_FAIL GET_INFO                        → versão e uptime ECHO <msg> →
 * devolve a mensagem LIST_ALL                        → lista todos os
 * utilizadores registados CHECK_INBOX <user>              → mostra mensagens na
 * caixa de entrada SEND_MSG <dest> <remetente> <msg> → armazena mensagem para
 * outro utilizador REGISTER <user> <pass>          → regista novo utilizador
 * (estado: PENDING) APPROVE_USER <admin> <user>     → admin aprova utilizador
 * pendente DELETE_USER <admin> <user>      → admin apaga utilizador
 *
 * ============================================================================
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* Configuração do servidor */
#define SERVER_PORT 10000
#define BUF_SIZE 2048
#define USERS_FILE "users.txt"
#define INBOX_FILE "inbox.txt"
#define LOG_FILE "logs.txt"
#define TEMP_FILE \
    "users_tmp.txt" /* Ficheiro temporário para operações de ficheiro */

/* Versão do servidor — actualizar por etapa */
const char* VERSAO_SERVIDOR = "2.0-Fase2";
time_t start_time;

/*
 * ============================================================================
 * FUNÇÃO: guardar_log()
 * ============================================================================
 *
 * O que faz:
 *   Regista uma mensagem no ficheiro de logs (logs.txt) com timestamp, e
 * imprime a mensagem no terminal com cor específica conforme o tipo de registo.
 *
 * Por que é importante:
 *   Permite ao administrador acompanhar a actividade do servidor em tempo real
 *   (logins, erros, operações administrativas) e manter um histórico
 * persistente.
 *
 * Como funciona:
 *   1. Abre ficheiro logs.txt em modo append ("a") para adicionar nova linha
 *   2. Obtém data/hora actual e formata com strftime
 *   3. Escreve [TIMESTAMP] mensagem
 *   4. Fecha ficheiro
 *   5. Imprime no terminal com cores ANSI:
 *      tipo=1: Verde (sucesso)
 *      tipo=3: Vermelho (erro)
 *      outro : Ciano (informação)
 *
 * ============================================================================
 */
void guardar_log(const char* mensagem, int tipo) {
    FILE* f = fopen(LOG_FILE, "a");
    if (f) {
        char data[64];
        time_t agora = time(NULL);
        struct tm* t = localtime(&agora);
        strftime(data, sizeof(data), "%Y-%m-%d %H:%M:%S", t);
        fprintf(f, "[%s] %s\n", data, mensagem);
        fclose(f);
    }
    /* Imprimir no terminal com cores de acordo com o tipo */
    if (tipo == 1)
        printf(" \033[1;32m[OK]\033[0m    | %s\n", mensagem);
    else if (tipo == 3)
        printf(" \033[1;31m[ERRO]\033[0m  | %s\n", mensagem);
    else
        printf(" \033[1;36m[INFO]\033[0m  | %s\n", mensagem);
}

/*
 * ============================================================================
 * FUNÇÃO: desenhar_cabecalho_servidor()
 * ============================================================================
 *
 * O que faz:
 *   Desenha a interface de utilizador (TUI) do servidor no terminal, mostrando
 *   o logo ASCII do C-CORD, estado do servidor, número de porto e versão.
 *
 * Por que é importante:
 *   Oferece feedback visual imediato ao operador do servidor sobre o estado
 *   e configuração do mesmo. Melhora a usabilidade do servidor em execução.
 *
 * Como funciona:
 *   1. Limpa o terminal com system("clear")
 *   2. Imprime logo ASCII em cor ciana (código ANSI 1;36m)
 *   3. Mostra estado (ONLINE), porto, ficheiro de BD e versão
 *   4. Inicializa a secção "LIVE FEED DE ATIVIDADE" para logs
 *
 * ============================================================================
 */
void desenhar_cabecalho_servidor() {
    system("clear");
    printf("\033[1;36m");
    printf("   ____         ____ ___  ____  ____    \n");
    printf("  / ___|       / ___/ _ \\|  _ \\|  _ \\   \n");
    printf(" | |     ____ | |  | | | | |_) | | | |  \n");
    printf(" | |___ |____|| |__| |_| |  _ <| |_| |  \n");
    printf("  \\____|       \\____\\___/|_| \\_\\____/   \n");
    printf("\033[0m\n");
    printf(
        "======================================================================"
        "==\n");
    printf(
        "         C-CORD SERVER (Etapa 2 — F3/F4/F5/F6/F7/F8)                  "
        " \n");
    printf(
        "======================================================================"
        "==\n");
    printf(
        " STATUS: \033[1;32mONLINE\033[0m | PORTO: %d | BD: %s | VERSAO: %s\n",
        SERVER_PORT, USERS_FILE, VERSAO_SERVIDOR);
    printf(
        "----------------------------------------------------------------------"
        "--\n");
    printf(" LIVE FEED DE ATIVIDADE:\n");
}

/*
 * ============================================================================
 * FUNÇÃO: check_auth()
 * ============================================================================
 *
 * O que faz:
 *   Valida um par (username, password) contra a base de dados de utilizadores.
 *   Retorna se o utilizador é ADMIN ou USER, ou rejeita se credenciais
 * inválidas. Impede login de utilizadores PENDING (aguardando aprovação).
 *
 * Por que é importante:
 *   É o núcleo da autenticação (F3). Controla o acesso ao servidor.
 *   Diferencia entre ADMIN e USER para aplicar diferentes permissões.
 *   Protege contas PENDING contra acesso não autorizado.
 *
 * Como funciona:
 *   1. Abre users.txt em modo leitura
 *   2. Lê linha por linha (formato: username:password:ROLE:STATUS)
 *   3. Parse com sscanf usando delimitador ':'
 *   4. Se username e password coincidem:
 *      - Se STATUS é PENDING, retorna -1 (bloqueado)
 *      - Se STATUS é ACTIVE, retorna 1 (sucesso) e copia ROLE para o buffer
 *   5. Se não encontrar, retorna 0 (falha)
 *
 * Retorno:
 *   1  = Autenticação bem-sucedida (ACTIVE)
 *  -1  = Utilizador PENDING (não pode fazer login)
 *   0  = Credenciais inválidas
 *
 * ============================================================================
 */
int check_auth(const char* username, const char* password, char* role) {
    FILE* f = fopen(USERS_FILE, "r");
    if (!f) {
        guardar_log("users.txt nao encontrado!", 3);
        return 0;
    }

    char line[256], id[10], u[50], p[50], r[20], s[20];
    while (fgets(line, sizeof(line), f)) {
        /* Parsear linha: ID:username:password:ROLE:STATUS */
        if (sscanf(line, "%9[^:]:%49[^:]:%49[^:]:%19[^:]:%19s", id, u, p, r, s) == 5) {
            if (strcmp(u, username) == 0 && strcmp(p, password) == 0) {
                fclose(f);
                /* Utilizadores PENDING não podem autenticar */
                if (strcmp(s, "PENDING") == 0) return -1;
                strcpy(role, r);
                return 1;
            }
        }
    }
    fclose(f);
    return 0;
}

/*
 * ============================================================================
 * FUNÇÃO: is_admin()
 * ============================================================================
 *
 * O que faz:
 *   Verifica se um utilizador é ADMINISTRADOR e está ACTIVO.
 *   Usada para proteger operações administrativas (F7, F8).
 *
 * Por que é importante:
 *   Impede que utilizadores normais executem operações críticas como
 *   aprovar utilizadores ou eliminar contas.
 *
 * Como funciona:
 *   1. Abre users.txt
 *   2. Lê linha por linha
 *   3. Se encontrar username com ROLE=ADMIN e STATUS=ACTIVE, retorna 1
 *   4. Caso contrário, retorna 0
 *
 * ============================================================================
 */
int is_admin(const char* username) {
    FILE* f = fopen(USERS_FILE, "r");
    if (!f) return 0;

    char line[256], id[10], u[50], p[50], r[20], s[20];
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%9[^:]:%49[^:]:%49[^:]:%19[^:]:%19s", id, u, p, r, s) == 5) {
            if (strcmp(u, username) == 0 && strcmp(r, "ADMIN") == 0 &&
                strcmp(s, "ACTIVE") == 0) {
                fclose(f);
                return 1;
            }
        }
    }
    fclose(f);
    return 0;
}

/*
 * ============================================================================
 * FUNÇÃO: list_all()
 * ============================================================================
 *
 * O que faz:
 *   Lista todos os utilizadores registados no sistema (F5).
 *   Mostra: username, ROLE (ADMIN/USER), STATUS (ACTIVE/PENDING).
 *   NÃO mostra passwords por razões de segurança.
 *
 * Por que é importante:
 *   Permite que administradores e utilizadores vejam quem está registado.
 *   Útil para fins de descoberta e gestão de conta.
 *
 * Como funciona:
 *   1. Abre users.txt
 *   2. Parse cada linha (username:password:ROLE:STATUS)
 *   3. Constrói string de resposta com formatação numerada
 *   4. Omite password intencionalmente
 *   5. Se BD vazia, devolve mensagem "(sem utilizadores)"
 *
 * ============================================================================
 */
void list_all(char* response) {
    FILE* f = fopen(USERS_FILE, "r");
    if (!f) {
        strcpy(response, "ERRO: Ficheiro de utilizadores nao encontrado.");
        return;
    }

    strcpy(response, "=== UTILIZADORES REGISTADOS ===\n");
    char line[256], id[10], u[50], p[50], r[20], s[20];
    int count = 0;

    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%9[^:]:%49[^:]:%49[^:]:%19[^:]:%19s", id, u, p, r, s) == 5) {
            char entry[128];
            sprintf(entry, " [ID:%s] %s | Papel: %s | Estado: %s\n", id, u, r,
                    s);
            strncat(response, entry, BUF_SIZE - strlen(response) - 1);
        }
    }
    fclose(f);

    if (count == 0)
        strncat(response, " (sem utilizadores)\n",
                BUF_SIZE - strlen(response) - 1);
}

/*
 * ============================================================================
 * FUNÇÃO: check_inbox()
 * ============================================================================
 *
 * O que faz:
 *   Lê todas as mensagens destinadas a um utilizador específico (F5).
 *   As mensagens são armazenadas no inbox.txt em formato:
 *   destinatario:remetente:mensagem
 *
 * Por que é importante:
 *   Implementa sistema de mensageria assíncrona entre utilizadores.
 *   Um utilizador pode deixar mensagem para outro mesmo que offline.
 *
 * Como funciona:
 *   1. Abre inbox.txt
 *   2. Lê linha por linha
 *   3. Parse: destinatario:remetente:mensagem
 *   4. Se destinatario == username, adiciona à resposta
 *   5. Se nenhuma mensagem, devolve "(sem mensagens novas)"
 *
 * ============================================================================
 */
void check_inbox(const char* username, char* response) {
    FILE* f = fopen(INBOX_FILE, "r");
    if (!f) {
        strcpy(response, "A sua caixa de entrada esta vazia.");
        return;
    }

    sprintf(response, "=== CAIXA DE ENTRADA DE %s ===\n", username);
    char line[512], dest[50], from[50], msg[400];
    int count = 0;

    while (fgets(line, sizeof(line), f)) {
        /* Remove newline no fim */
        line[strcspn(line, "\n")] = 0;
        if (sscanf(line, "%49[^:]:%49[^:]:%399[^\n]", dest, from, msg) == 3) {
            if (strcmp(dest, username) == 0) {
                char entry[512];
                sprintf(entry, " [%d] De: %s → %s\n", ++count, from, msg);
                strncat(response, entry, BUF_SIZE - strlen(response) - 1);
            }
        }
    }
    fclose(f);

    if (count == 0)
        strncat(response, " (sem mensagens novas)\n",
                BUF_SIZE - strlen(response) - 1);
}

/*
 * ============================================================================
 * FUNÇÃO: send_msg()
 * ============================================================================
 *
 * O que faz:
 *   Armazena uma mensagem para um utilizador destinatário (F5).
 *   A mensagem é guardada no inbox.txt mesmo que o destinatário esteja offline.
 *
 * Por que é importante:
 *   Implementa um sistema de mensageria persistente e assíncrono.
 *   Não depende de ambos os utilizadores estarem online simultaneamente.
 *
 * Como funciona:
 *   1. Abre inbox.txt em modo append ("a")
 *   2. Escreve linha: destinatario:remetente:mensagem
 *   3. Fecha ficheiro
 *   4. Confirma sucesso ao remetente
 *
 * ============================================================================
 */
void send_msg(const char* dest, const char* from, const char* msg,
              char* response) {
    FILE* f = fopen(INBOX_FILE, "a");
    if (!f) {
        strcpy(response, "ERRO: Nao foi possivel guardar mensagem.");
        return;
    }

    fprintf(f, "%s:%s:%s\n", dest, from, msg);
    fclose(f);
    sprintf(response, "MSG_SENT: Mensagem entregue na caixa de %s.", dest);
}

/*
 * ============================================================================
 * FUNÇÃO: register_user()
 * ============================================================================
 *
 * O que faz:
 *   Regista um novo utilizador no sistema (F6).
 *   A conta começa em estado PENDING, precisa aprovação de admin (F7).
 *
 * Por que é importante:
 *   Implementa workflow de aprovação: novo registo → pendente → aprovado.
 *   Permite controlo de qualidade e prevenção de spam.
 *
 * Como funciona:
 *   1. Verifica se username já existe (evita duplicados)
 *   2. Valida se username e password contêm ':' (corrompe BD) — REJEITA SE
 * CONTIVER
 *   3. Abre users.txt em append
 *   4. Escreve: username:password:USER:PENDING
 *   5. Utilizador não pode fazer login até admin aprovar (status PENDING)
 *
 * SEGURANÇA:
 *   - Rejeita username ou password com ':' pois isso quebra o parsing
 *   - Novas contas começa em PENDING (não ACTIVE)
 *
 * ============================================================================
 */
void register_user(const char* username, const char* password, char* response) {
    /* SEGURANÇA: Impedir caracteres ':' que corrompem a BD */
    if (strchr(username, ':') != NULL) {
        strcpy(response, "REGISTER_FAIL: Username nao pode conter ':'.");
        return;
    }
    if (strchr(password, ':') != NULL) {
        strcpy(response, "REGISTER_FAIL: Password nao pode conter ':'.");
        return;
    }

    /* Verificar se o utilizador já existe */
    FILE* f = fopen(USERS_FILE, "r");
    if (f) {
        char line[256], u[50];
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "%49[^:]", u) == 1) {
                if (strcmp(u, username) == 0) {
                    fclose(f);
                    strcpy(response, "REGISTER_FAIL: Utilizador ja existe.");
                    return;
                }
            }
        }
        fclose(f);
    }

    /* Adicionar novo utilizador com estado PENDING */
    f = fopen(USERS_FILE, "a");
    if (!f) {
        strcpy(response,
               "ERRO: Nao foi possivel aceder ao ficheiro de utilizadores.");
        return;
    }

    fprintf(f, "%s:%s:USER:PENDING\n", username, password);
    fclose(f);
    sprintf(response,
            "REGISTER_OK: Utilizador '%s' registado. Aguarda aprovacao do "
            "administrador.",
            username);
}

/*
 * ============================================================================
 * FUNÇÃO: approve_user()
 * ============================================================================
 *
 * O que faz:
 *   Muda o status de um utilizador de PENDING para ACTIVE (F7).
 *   Apenas admins podem fazer isto.
 *   Depois de aprovado, o utilizador pode fazer login.
 *
 * Por que é importante:
 *   Implementa o workflow de aprovação. Previne acesso de contas não
 * autorizadas.
 *
 * Como funciona (OTIMIZADO):
 *   1. Verifica se quem aprova é realmente admin
 *   2. Abre users.txt e users_tmp.txt
 *   3. Lê users.txt linha a linha
 *   4. Se encontra o utilizador PENDING alvo, escreve com status ACTIVE no temp
 *   5. Outras linhas são copiadas como estão
 *   6. Fecha ambos ficheiros
 *   7. Remove users.txt original (com remove())
 *   8. Renomeia users_tmp.txt para users.txt (com rename())
 *
 * OTIMIZAÇÃO DE MEMÓRIA:
 *   - Antes: char lines[100][256] — limitado a 100 utilizadores
 *   - Agora: Leitura/escrita linha a linha — sem limite de memória
 *
 * ============================================================================
 */
void approve_user(const char* admin_user, const char* target, char* response) {
    /* Verificar se quem aprova é realmente admin */
    if (!is_admin(admin_user)) {
        strcpy(response, "APPROVE_FAIL: Sem permissoes de administrador.");
        return;
    }

    /* Abrir ficheiro original para leitura e temp para escrita */
    FILE* f_read = fopen(USERS_FILE, "r");
    if (!f_read) {
        strcpy(response, "ERRO: Ficheiro de utilizadores nao encontrado.");
        return;
    }

    FILE* f_write = fopen(TEMP_FILE, "w");
    if (!f_write) {
        fclose(f_read);
        strcpy(response, "ERRO: Nao foi possivel criar ficheiro temporario.");
        return;
    }

    /* Ler linha a linha e processar */
    char line[256], id[10], u[50], p[50], r[20], s[20];
    int found = 0;

    while (fgets(line, sizeof(line), f_read)) {
        /* Remover newline */
        line[strcspn(line, "\n")] = 0;

        if (sscanf(line, "%9[^:]:%49[^:]:%49[^:]:%19[^:]:%19s", id, u, p, r, s) == 5) {
            if (strcmp(u, target) == 0 && strcmp(s, "PENDING") == 0) {
                /* Encontrou o utilizador PENDING — reescrever com ACTIVE */
                fprintf(f_write, "%s:%s:%s:%s:ACTIVE\n", id, u, p, r);
                found = 1;
            } else {
                /* Outras linhas — copiar na mesma */
                fprintf(f_write, "%s\n", line);
            }
        } else {
            /* Linha mal formada — preservar na mesma */
            fprintf(f_write, "%s\n", line);
        }
    }

    fclose(f_read);
    fclose(f_write);

    /* Substituir ficheiro original pelo temporário */
    if (found) {
        remove(USERS_FILE);
        rename(TEMP_FILE, USERS_FILE);
        sprintf(response,
                "APPROVE_OK: Utilizador '%s' aprovado e agora pode autenticar.",
                target);
    } else {
        remove(TEMP_FILE);
        sprintf(
            response,
            "APPROVE_FAIL: Utilizador '%s' nao encontrado ou ja esta activo.",
            target);
    }
}

/*
 * ============================================================================
 * FUNÇÃO: delete_user()
 * ============================================================================
 *
 * O que faz:
 *   Remove completamente um utilizador do sistema (F8).
 *   Apenas admins podem fazer isto.
 *   Protecção: impede que um admin se delete a si próprio.
 *
 * Por que é importante:
 *   Permite limpeza de contas inativas, spam ou comprometidas.
 *   Protege a integridade do sistema.
 *
 * Como funciona (OTIMIZADO):
 *   1. Verifica se quem delete é realmente admin
 *   2. Verifica se admin não está a tentar apagar-se a si próprio
 *   3. Abre users.txt para leitura e users_tmp.txt para escrita
 *   4. Lê linha a linha
 *   5. Se encontra utilizador alvo, salta a linha (não escreve)
 *   6. Outras linhas são copiadas
 *   7. Fecha ambos
 *   8. Remove original e renomeia temporário
 *
 * OTIMIZAÇÃO DE MEMÓRIA:
 *   - Antes: char lines[100][256] — limitado a 100 utilizadores
 *   - Agora: Leitura/escrita linha a linha — sem limite de memória
 *
 * ============================================================================
 */
void delete_user(const char* admin_user, const char* target, char* response) {
    /* Verificar se quem apaga é realmente admin */
    if (!is_admin(admin_user)) {
        strcpy(response, "DELETE_FAIL: Sem permissoes de administrador.");
        return;
    }

    /* Protecção: admin não se pode apagar a si próprio */
    if (strcmp(admin_user, target) == 0) {
        strcpy(response,
               "DELETE_FAIL: Nao e possivel apagar a propria conta de "
               "administrador.");
        return;
    }

    /* Abrir ficheiro original para leitura e temp para escrita */
    FILE* f_read = fopen(USERS_FILE, "r");
    if (!f_read) {
        strcpy(response, "ERRO: Ficheiro nao encontrado.");
        return;
    }

    FILE* f_write = fopen(TEMP_FILE, "w");
    if (!f_write) {
        fclose(f_read);
        strcpy(response, "ERRO: Nao foi possivel criar ficheiro temporario.");
        return;
    }

    /* Ler linha a linha e processar */
    char line[256], u[50];
    int found = 0;

    while (fgets(line, sizeof(line), f_read)) {
        /* Remover newline */
        line[strcspn(line, "\n")] = 0;

        if (sscanf(line, "%49[^:]", u) == 1 && strcmp(u, target) == 0) {
            /* Encontrou o utilizador alvo — saltar esta linha (apagar) */
            found = 1;
        } else {
            /* Outras linhas — copiar na mesma */
            fprintf(f_write, "%s\n", line);
        }
    }

    fclose(f_read);
    fclose(f_write);

    /* Substituir ficheiro original pelo temporário */
    if (found) {
        remove(USERS_FILE);
        rename(TEMP_FILE, USERS_FILE);
        sprintf(response, "DELETE_OK: Utilizador '%s' removido do sistema.",
                target);
    } else {
        remove(TEMP_FILE);
        sprintf(response, "DELETE_FAIL: Utilizador '%s' nao encontrado.",
                target);
    }
}

/*
 * ============================================================================
 * FUNÇÃO: main()
 * ============================================================================
 *
 * O que faz:
 *   Motor principal do servidor. Implementa servidor sequencial/bloqueante.
 *   Recebe conexões TCP, processa comandos, devolve respostas.
 *
 * Por que é importante:
 *   O coração da aplicação. Controla o fluxo de execução e handleamento
 *   de clientes.
 *
 * Como funciona:
 *
 *   FASE 1: INICIALIZAÇÃO DO SOCKET
 *   ─────────────────────────────────
 *   1. Criar socket TCP com socket(AF_INET, SOCK_STREAM, 0)
 *      - AF_INET = protocolo IPv4
 *      - SOCK_STREAM = conexão TCP (não UDP)
 *      - 0 = protocolo por defeito
 *
 *   2. Configurar opção SO_REUSEADDR com setsockopt()
 *      - Permite reutilizar porto mesmo após encerrar servidor
 *      - Sem isto, temos que esperar 30-60s antes de poder reabrir
 *
 *   FASE 2: BIND — ASSOCIAR SOCKET A ENDEREÇO/PORTO
 *   ────────────────────────────────────────────────
 *   1. Preencher struct sockaddr_in:
 *      - sin_family = AF_INET (IPv4)
 *      - sin_addr.s_addr = INADDR_ANY (escuta em todas as interfaces)
 *      - sin_port = htons(SERVER_PORT) (porto 10000 em formato rede)
 *
 *   2. Chamar bind() para associar socket a endereço
 *      - Se falhar, significa porto já em uso — sair com erro
 *
 *   FASE 3: LISTEN — COLOCAR SOCKET A ESCUTA
 *   ──────────────────────────────────────────
 *   1. listen(fd, 5) — fila de 5 conexões máximo
 *      - Se mais de 5 clientes tentam conectar simultaneamente,
 *        os excedentes ficam em fila de espera do SO
 *
 *   FASE 4: LOOP PRINCIPAL — ACCEPT E PROCESSA
 *   ──────────────────────────────────────────
 *   1. accept() — bloqueia até chegar cliente
 *   2. read() — recebe buffer do cliente
 *   3. Parse do comando (AUTH, GET_INFO, ECHO, etc.)
 *   4. Executa lógica associada ao comando
 *   5. write() — envia resposta ao cliente
 *   6. close() — encerra conexão (modelo sequencial)
 *   7. Volta a step 1 (volta a accept())
 *
 * NOTAS ARQUITECTÓNICAS:
 *   - Modelo SEQUENCIAL: um cliente de cada vez
 *   - Modelo BLOQUEANTE: accept() fica bloqueado até cliente chegar
 *   - Sem threads ou multiplexing (isso fica para Fase 3)
 *   - Cada cliente tem sua própria conexão TCP
 *   - Depois de responder, conexão é encerrada (sem keep-alive)
 *
 * ============================================================================
 */
int main() {
    int fd, client;
    struct sockaddr_in addr;
    char buffer[BUF_SIZE];

    /* Guardar hora de inicio para calcular uptime */
    start_time = time(NULL);

    /* ====================================================================
     * FASE 1: CRIAR E CONFIGURAR SOCKET TCP
     * ==================================================================== */

    /* Criar socket: AF_INET=IPv4, SOCK_STREAM=TCP, 0=protocolo por defeito */
    fd = socket(AF_INET, SOCK_STREAM, 0);

    /* Permitir reutilizar porto (SO_REUSEADDR) evita "Address already in use"
     */
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* ====================================================================
     * FASE 2: BIND — ASSOCIAR SOCKET A ENDEREÇO E PORTO
     * ==================================================================== */

    /* Preencher estrutura de endereço */
    addr.sin_family = AF_INET; /* Protocolo IPv4 */
    addr.sin_addr.s_addr =
        INADDR_ANY; /* Escuta em todas as interfaces (0.0.0.0) */
    addr.sin_port =
        htons(SERVER_PORT); /* Converter porto para formato rede (big-endian) */

    /* Associar socket ao endereço/porto */
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Erro ao abrir porto");
        exit(-1);
    }

    /* ====================================================================
     * FASE 3: LISTEN — COLOCAR SOCKET À ESCUTA DE CONEXÕES
     * ==================================================================== */

    listen(fd, 5); /* Fila de 5 conexões máximo */

    /* Desenhar interface e registar inicio */
    desenhar_cabecalho_servidor();
    guardar_log("Servidor v2.0 (Etapa 2) iniciado e a escuta.", 1);

    /* ====================================================================
     * FASE 4: LOOP PRINCIPAL — ACCEPT, RECEBER, PROCESSAR, RESPONDER
     * ==================================================================== */

    while (1) {
        struct sockaddr_in cli_addr;
        socklen_t size = sizeof(cli_addr);

        /* ACCEPT: Bloqueia aqui até chegar cliente */
        client = accept(fd, (struct sockaddr*)&cli_addr, &size);

        if (client > 0) {
            /* Limpar buffer antes de ler */
            memset(buffer, 0, BUF_SIZE);

            /* READ: Receber comando do cliente */
            if (read(client, buffer, BUF_SIZE - 1) > 0) {
                char response[BUF_SIZE] = "";
                char log_msg[BUF_SIZE] = "";
                int log_type = 0;

                /* ================================================
                 * F3 — AUTH <user> <pass>
                 * Autentica utilizador contra BD
                 * ================================================ */
                if (strncmp(buffer, "AUTH ", 5) == 0) {
                    char u[50], p[50], r[20];
                    sscanf(buffer + 5, "%49s %49s", u, p);
                    int result = check_auth(u, p, r);

                    if (result == 1) {
                        /* Autenticação bem-sucedida */
                        sprintf(response, "AUTH_SUCCESS:%s", r);
                        sprintf(log_msg, "Login OK: '%s' (%s)", u, r);
                        log_type = 1;
                    } else if (result == -1) {
                        /* Utilizador PENDING (aguardando aprovação) */
                        strcpy(response, "AUTH_PENDING");
                        sprintf(log_msg, "Login bloqueado (PENDING): '%s'", u);
                        log_type = 3;
                    } else {
                        /* Credenciais inválidas */
                        strcpy(response, "AUTH_FAIL");
                        sprintf(log_msg, "Login FALHOU: '%s'", u);
                        log_type = 3;
                    }
                }

                /* ================================================
                 * F4 — GET_INFO
                 * Devolve versão do servidor e uptime
                 * ================================================ */
                else if (strcmp(buffer, "GET_INFO") == 0) {
                    int up = (int)difftime(time(NULL), start_time);
                    sprintf(response,
                            "C-Cord Server v%s | Uptime: %02dh:%02dm:%02ds",
                            VERSAO_SERVIDOR, up / 3600, (up % 3600) / 60,
                            up % 60);
                    sprintf(log_msg, "GET_INFO processado");
                    log_type = 0;
                }

                /* ================================================
                 * F4 — ECHO <msg>
                 * Devolve a mensagem recebida (teste de latência)
                 * ================================================ */
                else if (strncmp(buffer, "ECHO ", 5) == 0) {
                    sprintf(response, "Servidor Ecoa: %s", buffer + 5);
                    sprintf(log_msg, "ECHO: '%s'", buffer + 5);
                    log_type = 0;
                }

                /* ================================================
                 * F5 — LIST_ALL
                 * Lista todos os utilizadores registados
                 * ================================================ */
                else if (strcmp(buffer, "LIST_ALL") == 0) {
                    list_all(response);
                    sprintf(log_msg, "LIST_ALL executado");
                    log_type = 0;
                }

                /* ================================================
                 * F5 — CHECK_INBOX <user>
                 * Mostra mensagens da caixa de entrada
                 * ================================================ */
                else if (strncmp(buffer, "CHECK_INBOX ", 12) == 0) {
                    char user[50];
                    sscanf(buffer + 12, "%49s", user);
                    check_inbox(user, response);
                    sprintf(log_msg, "CHECK_INBOX: '%s'", user);
                    log_type = 0;
                }

                /* ================================================
                 * F5 — SEND_MSG <dest> <from> <msg>
                 * Envia mensagem para outro utilizador
                 * ================================================ */
                else if (strncmp(buffer, "SEND_MSG ", 9) == 0) {
                    char dest[50], from[50], msg[400];
                    sscanf(buffer + 9, "%49s %49s %399[^\n]", dest, from, msg);
                    send_msg(dest, from, msg, response);
                    sprintf(log_msg, "SEND_MSG: de '%s' para '%s'", from, dest);
                    log_type = 1;
                }

                /* ================================================
                 * F6 — REGISTER <user> <pass>
                 * Regista novo utilizador (começa em PENDING)
                 * ================================================ */
                else if (strncmp(buffer, "REGISTER ", 9) == 0) {
                    char u[50], p[50];
                    sscanf(buffer + 9, "%49s %49s", u, p);
                    register_user(u, p, response);
                    sprintf(log_msg, "REGISTER: tentativa para '%s'", u);
                    log_type = 1;
                }

                /* ================================================
                 * F7 — APPROVE_USER <admin> <user>
                 * Admin aprova utilizador PENDING para ACTIVE
                 * ================================================ */
                else if (strncmp(buffer, "APPROVE_USER ", 13) == 0) {
                    char admin[50], target[50];
                    sscanf(buffer + 13, "%49s %49s", admin, target);
                    approve_user(admin, target, response);
                    sprintf(log_msg, "APPROVE_USER: '%s' por '%s'", target,
                            admin);
                    log_type = 1;
                }

                /* ================================================
                 * F8 — DELETE_USER <admin> <user>
                 * Admin remove utilizador do sistema
                 * ================================================ */
                else if (strncmp(buffer, "DELETE_USER ", 12) == 0) {
                    char admin[50], target[50];
                    sscanf(buffer + 12, "%49s %49s", admin, target);
                    delete_user(admin, target, response);
                    sprintf(log_msg, "DELETE_USER: '%s' por '%s'", target,
                            admin);
                    log_type = 1;
                }

                /* ================================================
                 * COMANDO DESCONHECIDO
                 * ================================================ */
                else {
                    strcpy(response, "CMD_INVALID");
                    strcpy(log_msg, "Comando desconhecido recebido");
                    log_type = 3;
                }

                /* Guardar registo de actividade e responder ao cliente */
                guardar_log(log_msg, log_type);
                write(client, response, strlen(response));
            }

            /* Modelo sequencial/bloqueante: fecha após cada resposta */
            close(client);
        }
    }

    return 0;
}
