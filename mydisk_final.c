/* Marcos P.Camargo
   Vinicius Dornelles*/

/*Bibliotecas*/
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "utils.h"

/*Cores*/
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

/*Tamanhos*/
#define BLOCK_SIZE 512
#define BLOCK_NUMBER 256
#define BLOCK_BOOT 10
#define TAM_NOME 4096
#define MAX_ARGS 30

/*Comparações*/
#define MISMATCH -1
#define TRUE 1
#define FALSE 0
#define MATCH 0

/*Comandos*/
#define AJUDA "ajuda"
#define VERD "verd"
#define VERSET "verset"
#define CRIARDIR "criad"
#define CRIARARQ "criaa"
#define REMOVEDIR "removed"
#define REMOVEARQ "removea"
#define MAPA "mapa"
#define ARVORE "arvore"
#define SAIR "sair"

/*Estrutura do HD*/
typedef struct struct_disk {
  char data[BLOCK_SIZE];
} disk;

disk mydisk[BLOCK_NUMBER];

/*Estrutura de Diretório*/
typedef struct struct_directory {
  char nome[32];
  int posicao_setor;
  struct struct_arquivo *arquivodir;
  struct struct_directory *irmao;
  struct struct_directory *proxnivel;
} dir;

/*Estrutura do Arquivo*/
typedef struct struct_arquivo {
  int setor;
  char nome[32];
  int inicio;
  int tam;
  char date_time[32];
  struct struct_setor *proxsetor;
  struct struct_arquivo *proxarquivo;
} arquivodisk;

/*Estrutura do setor do HD*/
typedef struct struct_setor {
  int setor;
  int inicio;
  int tam;
  struct struct_setor *proxsetor;
} setor;

/*Mapa de bits do disco*/
int mapa_bits[BLOCK_NUMBER]; /*1 - Cheio, 0 - Não cheio*/

/*Vetor utilizado para armazenar os comandos simples*/
char *comando_shell[MAX_ARGS];
char *pastausuario[MAX_ARGS];

/*Declaração das funções*/

/*Comandos*/
void ver_diretorio(dir *mapadiretorio);
void executarcomando(dir *mapadiretorio);
void verd(dir *mapadiretorio);
void ver_setores(dir *mapadiretorio);
void verset(dir *mapadiretorio, char *nome_arquivo);
void print_ajuda();
void criar_diretorio(dir *mapadiretorio);
void criar_arquivo(dir *mapadiretorio);
void removea(dir *mapadiretorio);
void removed(dir *mapadiretorio);

/*Leitura de comandos*/
void lercomando();
int string_tokenizer();

/*Inicialização*/
void config_disco();
int iniciar_mapabits();
int iniciar_diretorio(dir *mapadiretorio);

/*Pesquisar*/
dir *pesquisar_diretorio(dir *mapadiretorio, int nivel, char *nomepasta);
int pesquisar_mapa_bits_diretorio();
int pesquisar_mapa_bits_arquivo(int data_total, arquivodisk *arquivo);

/*Criar*/
void criar_arquivo_disco(dir *mapadiretorio, char *nome, int data);
void adicionar_diretorio(dir *mapadiretorio, char *);

/*Imprimir Shell*/
void imprimir_shell(dir *mapadiretorios);
void imprimir_diretorio();

/*Escrita*/
void escrever_disco_diretorio(int setor, int data);
void escrever_disco_arquivo(int, int, int, arquivodisk *, int);

/*Atualização*/
void atualizar_mapabits();

/*Remoção*/
void remover_diretorio(dir *, dir *);
void remover_arquivo(dir *, dir *, char *);
void remover_arquivo_disco();

/*Solicitações gráficas*/
void exibe_mapa();
void exibe_arvore(dir *mapadiretorio, int rcont);

// Checagem de arquivos duplicados
int checa_arquivo(arquivodisk *arquivo, char *nome);

/*Devolve i -> posiçao da string comando que contem última pasta/arquivo
 * digitado*/
int string_tokenizer() {
  char *token = strtok(comando_shell[1], "\\");
  int i = 0;

  while (token != NULL) {
    pastausuario[i++] = token;
    token = strtok(NULL, "\\");
  }

  return i;
}

/*Comando que imprime os comandos implementados*/
void print_ajuda() {
  fprintf(stderr, "My File System\n");
  fprintf(stderr, "=========================\n");

  fprintf(stderr, "\ncriad [\\caminho]\\dir            ---> criar diretório");
  fprintf(stderr, "\ncriaa [\\caminho] arquivo tamanho ---> criar arquivo");
  fprintf(stderr,
          "\nremoved [\\caminho]\\dir          ---> remover "
          "diretório(se estiver vazio)");
  fprintf(stderr, "\nremovea [\\caminho] arquivo       ---> remover arquivo");
  fprintf(stderr, "\nverd [\\caminho]\\dir             ---> exibir diretório");
  fprintf(stderr,
          "\nverset [\\caminho]\\arquivo       ---> exibir setores "
          "ocupados pelo arquivo");
  fprintf(stderr,
          "\nmapa                              ---> exibir mapa dos setores");
  fprintf(stderr,
          "\narvore                            ---> arvore de diretórios");
  fprintf(stderr,
          "\nsair                              ---> finaliza o sistema\n");
}

/*Imprime a shell*/
/*cd não está implementado*/
void imprimir_shell(dir *mapadiretorio) {
  fprintf(stderr, "%sFS:\\%s %s>", ANSI_COLOR_GREEN, mapadiretorio->nome,
          ANSI_COLOR_RESET);
}

/*Configura o disco para o sistema*/
void config_disco() {
  /*Configurando os 10 primeiros setores*/
  for (int i = 0; i < BLOCK_BOOT; i++) {
    for (int j = 0; j < BLOCK_SIZE; j++) {
      mydisk[i].data[j] = '1';
    }
  }
}

/*Primeira atualização do mapa_bits com base no BLOCK_BOOT*/
int iniciar_mapabits() {
  /*Setores já ocupados*/
  for (int i = 0; i < BLOCK_BOOT; i++) {
    mapa_bits[i] = 1;
  }

  /*Setores livres*/
  for (int i = BLOCK_BOOT; i < BLOCK_NUMBER; i++) {
    mapa_bits[i] = 0;
  }
}

int checa_arquivo(arquivodisk *arquivo, char *nome) {
  arquivodisk *aux = arquivo;

  if (strcmp(aux->nome, nome) == MATCH) return -1;

  while (aux->proxarquivo != NULL) {
    aux = aux->proxarquivo;
    if (strcmp(aux->nome, nome) == MATCH) return -1;
  }

  return 0;
}

void criar_arquivo_disco(dir *mapadiretorio, char *nome, int data) {
  // Estrturas necessárias paga pegar a data e hora

  struct tm *ptm;
  struct timeval tv;

  arquivodisk *arquivo = (arquivodisk *)malloc(sizeof(arquivodisk));
  gettimeofday(&tv, NULL);
  ptm = localtime(&tv.tv_sec);

  if (mapadiretorio->arquivodir == NULL) {
    strncpy(arquivo->nome, nome, sizeof(arquivo->nome));
    strftime(arquivo->date_time, sizeof(arquivo->date_time),
             "%d/%m/%Y %H:%M:%S", ptm);

    mapadiretorio->arquivodir = arquivo;
    arquivo->proxarquivo = NULL;
    arquivo->proxsetor = NULL;
    int result = pesquisar_mapa_bits_arquivo(data, arquivo);
    fprintf(stderr, "Arquivo %s salvo com sucesso 1ºSetor : %d \n",
            arquivo->nome, arquivo->setor);
  } else {
    arquivo = mapadiretorio->arquivodir;
    // Cria o arquivo caso não exista outro com o mesmo nome
    printf("%s, %s \n", arquivo->nome, nome);
    if (checa_arquivo(arquivo, nome) == 0) {
      while (arquivo->proxarquivo != NULL) {
        arquivo = arquivo->proxarquivo;
      }

      arquivodisk *arquivo_disk = (arquivodisk *)malloc(sizeof(arquivodisk));

      strftime(arquivo_disk->date_time, sizeof(arquivo_disk->date_time),
               "%d/%m/%Y %H:%M:%S", ptm);

      strncpy(arquivo_disk->nome, nome, sizeof(arquivo->nome));
      arquivo_disk->proxarquivo = NULL;
      arquivo_disk->proxsetor = NULL;
      arquivo->proxarquivo = arquivo_disk;
      int result = pesquisar_mapa_bits_arquivo(data, arquivo_disk);
      fprintf(stderr, "Arquivo %s salvo com sucesso 1ºSetor : %d \n",
              arquivo_disk->nome, arquivo_disk->setor);
    } else
      fprintf(stderr,
              "ERRO: Não foi possível criar o arquivo\nJá existe um "
              "arquivo com o mesmo nome !\n");
  }

  fprintf(stderr, "%s\n\n", mapadiretorio->nome);
}

void inicializar_diretorio(dir *mapadiretorio) {
  char *pointer = "raiz";
  strncpy(mapadiretorio->nome, pointer, sizeof(mapadiretorio->nome));
  mapadiretorio->posicao_setor = 10;
  mapadiretorio->proxnivel = NULL;
  mapadiretorio->arquivodir = NULL;
  mapadiretorio->irmao = NULL;
  fprintf(stderr, "%s\n", mapadiretorio->nome);
}

void imprimir_diretorio(dir *mapadiretorio) {
  dir *aux = (dir *)malloc(sizeof(dir));
  aux = mapadiretorio;

  while (aux != NULL) {
    fprintf(stderr, "%s\\", aux->nome);
    aux = aux->proxnivel;
  }
  fprintf(stderr, "\n\n");

  /*Testando diretórios irmãos da pasta raiz*/
  aux = mapadiretorio->proxnivel;

  if (aux->irmao != NULL) {
    aux = aux->irmao;
    while (aux != NULL) {
      fprintf(stderr, "%s\n", aux->nome);
      aux = aux->irmao;
    }
  }
}

void exibe_mapa() {
  printf(
      "\n Mapa de setores\n(0 -> Setor possui espaço livre)\n(1 -> Setor "
      "cheio)\n\n");

  int setor_pesquisado = 0;
  for (int lin = 0; lin < 16; lin++) {
    for (int col = 0; col < 16; col++) {
      printf("%d", mapa_bits[setor_pesquisado]);
      setor_pesquisado++;
    }
    printf("\n");
  }

  printf("\n");
}

void exibe_arvore(dir *mapadiretorio, int rcont) {
  // rcont = contador de recursao, para poder colocar os espaçamentos
  // corretos(apenas visual)

  dir *dir_aux = mapadiretorio;

  arquivodisk *arq_aux = (arquivodisk *)malloc(sizeof(arquivodisk));

  /*O algoritmo basicamente percorre todos os diretorios antes dos arquivos
    cada nivel dos diretorios ele avança uma camada na recursão
  */

  while (dir_aux != NULL) {
    arq_aux = dir_aux->arquivodir;
    for (int i = 0; i < rcont; i++) fprintf(stderr, "|  ");
    fprintf(stderr, "%s%s%s\n", ANSI_COLOR_BLUE, dir_aux->nome,
            ANSI_COLOR_RESET);

    if (dir_aux->proxnivel != NULL) {
      exibe_arvore(dir_aux->proxnivel, rcont + 1);
    }

    while (arq_aux != NULL) {
      for (int i = 0; i <= rcont; i++) fprintf(stderr, "|  ");

      fprintf(stderr, "%s\n", arq_aux->nome);
      arq_aux = arq_aux->proxarquivo;
    }

    dir_aux = dir_aux->irmao;
  }
}

void lercomando() {
  for (int i = 0; i < MAX_ARGS; i++) comando_shell[i] = NULL;

  char command[TAM_NOME];
  size_t size;

  fgets(command, TAM_NOME, stdin);

  if (strlen(command) > 1) {
    command[strlen(command) - 1] = '\0';
    // Obtendo todos os argumentos
    int i = 0;

    /*Removendo espaços iniciais*/
    while (isspace(command[i])) {
      i++;
    }
    memmove(command, command + i, strlen(command));

    /*É necessário zerar a variável de controle para o laço abaixo*/
    i = 0;

    /*Obtendo o nome do programa e seus argumentos*/
    char *token = strtok(command, " ");
    while (token != NULL) {
      comando_shell[i++] = token;
      token = strtok(NULL, " ");
    }
    comando_shell[i++] = '\0';
  }
}

void criar_diretorio(dir *mapadiretorio) {
  int nivel = string_tokenizer();

  if (nivel < 2) {
    fprintf(stderr,
            "É necessário criar uma pasta dentro do nível da pasta raiz\n");
    return;
  }

  dir *diretorio_pai = NULL;

  int indice_nome_pasta_a_ser_adicionada = (nivel - 1);
  int indice_nome_pasta_pai = indice_nome_pasta_a_ser_adicionada - 1;

  if (is_string_empty(pastausuario[indice_nome_pasta_a_ser_adicionada])) {
    fprintf(stderr, "Nome da pasta não pode ser vazio\n");
    return;
  }

  diretorio_pai = pesquisar_diretorio(mapadiretorio, nivel - 1,
                                      pastausuario[indice_nome_pasta_pai]);

  if (diretorio_pai == NULL) {
    fprintf(stderr, "Pasta não encontrada\n");
    fprintf(stderr,
            "Dica : Somente é permitido criar diretórios dentro da raiz\n");
    return;
  }

  fprintf(stderr, "Pasta : %s\n", diretorio_pai->nome);

  adicionar_diretorio(diretorio_pai,
                      pastausuario[indice_nome_pasta_a_ser_adicionada]);

  imprimir_diretorio(mapadiretorio);

  /*Finalizando a string*/
  comando_shell[0] = "\0";
  pastausuario[1] = "\0";
}

void ver_diretorio(dir *mapadiretorio) {
  int nivel = string_tokenizer();
  dir *novo = NULL;

  int indice_nome_pasta_procurada = (nivel - 1);

  if (is_string_empty(pastausuario[indice_nome_pasta_procurada])) {
    fprintf(stderr, "Nome da pasta não pode ser vazio\n");
    return;
  }

  novo = pesquisar_diretorio(mapadiretorio, nivel,
                             pastausuario[indice_nome_pasta_procurada]);

  if (novo == NULL) {
    fprintf(stderr, "Pasta inexistente\n");
    return;
  }

  fprintf(stderr, "%s\n", novo->nome);
  verd(novo);
}

void criar_arquivo(dir *mapadiretorio) {
  int indice_nome_arquivo = 2;
  int indice_tamanho_arquivo = 3;

  if (is_string_empty(comando_shell[indice_nome_arquivo]) ||
      is_string_empty(comando_shell[indice_tamanho_arquivo])) {
    fprintf(stderr, "Nome do arquivo ou tamanho do arquivo vazio\n");
    return;
  }

  for (int i = 0; comando_shell[indice_tamanho_arquivo][i] != '\0'; i++) {
    if (!isdigit((unsigned char)comando_shell[indice_tamanho_arquivo][i])) {
      fprintf(stderr, "Tamanho do arquivo inválido\n");
      return;
    }
  }

  int i = string_tokenizer();
  dir *novo = (dir *)malloc(sizeof(dir));
  novo = pesquisar_diretorio(mapadiretorio, i, pastausuario[i - 1]);

  if (novo == NULL) {
    fprintf(stderr, "Pasta inexistente e/ou sintaxe incorreta\n");
    return;
  }

  criar_arquivo_disco(novo, comando_shell[2], atoi(comando_shell[3]));
}

void ver_setores(dir *mapadiretorio) {
  int indice_nome_arquivo = 2;

  if (is_string_empty(comando_shell[indice_nome_arquivo])) {
    fprintf(stderr, "Nome do arquivo vazio\n");
    return;
  }

  int i = string_tokenizer();
  dir *nivel = (dir *)malloc(sizeof(dir));
  nivel = pesquisar_diretorio(mapadiretorio, i, pastausuario[i - 1]);

  if (nivel == NULL) {
    fprintf(stderr, "Sintaxe incorreta e/ou diretório incorreto\n");
  }

  verset(nivel, comando_shell[indice_nome_arquivo]);
}

void removed(dir *mapadiretorio) {
  int i = string_tokenizer();
  char *nome_pasta_a_ser_removida = pastausuario[i - 1];
  char *nome_pasta_pai = pastausuario[i - 2];

  if (is_string_empty(nome_pasta_a_ser_removida)) {
    return;
  }

  if (is_string_empty(nome_pasta_pai)) {
    return;
  }

  dir *nivel = (dir *)malloc(sizeof(dir));
  dir *nivel_anterior = (dir *)malloc(sizeof(dir));
  nivel = pesquisar_diretorio(mapadiretorio, i, nome_pasta_a_ser_removida);

  if (nivel == NULL) {
    fprintf(stderr, "Pasta inexistente: %s\n", nome_pasta_a_ser_removida);
    return;
  }

  nivel_anterior = pesquisar_diretorio(mapadiretorio, i - 1, nome_pasta_pai);

  if (nivel_anterior == NULL) {
    fprintf(stderr, "Pasta inexistente: %s\n", nome_pasta_pai);
    return;
  }

  remover_diretorio(nivel, nivel_anterior);
}

void removea(dir *mapadiretorio) {
  int i = string_tokenizer();
  int indice_nome_arquivo = 2;
  char *nome_arquivo_a_ser_removido = comando_shell[indice_nome_arquivo];

  if (is_string_empty(nome_arquivo_a_ser_removido)) {
    fprintf(stderr, "Nome do arquivo vazio\n");
    return;
  }

  dir *nivel = (dir *)malloc(sizeof(dir));
  dir *nivel_anterior = (dir *)malloc(sizeof(dir));
  nivel = pesquisar_diretorio(mapadiretorio, i, pastausuario[i - 1]);
  nivel_anterior =
      pesquisar_diretorio(mapadiretorio, i - 1, pastausuario[i - 2]);

  if (nivel == NULL) {
    fprintf(stderr, "Pasta inexistente e/ou sintaxe incorreta\n");
    return;
  }

  remover_arquivo(nivel, nivel_anterior, comando_shell[indice_nome_arquivo]);
}

void executarcomando(dir *mapadiretorio) {
  if (strcmp(comando_shell[0], SAIR) == MATCH) {
    exit(0);
  } else if (strcmp(comando_shell[0], AJUDA) == MATCH) {
    print_ajuda();
  } else if (strcmp(comando_shell[0], CRIARDIR) == MATCH) {
    criar_diretorio(mapadiretorio);
  } else if (strcmp(comando_shell[0], VERD) == MATCH) {
    ver_diretorio(mapadiretorio);
  } else if (strcmp(comando_shell[0], CRIARARQ) == MATCH) {
    criar_arquivo(mapadiretorio);
  } else if (strcmp(comando_shell[0], REMOVEDIR) == MATCH) {
    removed(mapadiretorio);
  } else if (strcmp(comando_shell[0], REMOVEARQ) == MATCH) {
    removea(mapadiretorio);
  } else if (strcmp(comando_shell[0], VERSET) == MATCH) {
    ver_setores(mapadiretorio);
  } else if (strcmp(comando_shell[0], MAPA) == MATCH) {
    exibe_mapa();
  } else if (strcmp(comando_shell[0], ARVORE) == MATCH) {
    exibe_arvore(mapadiretorio, 0);
  }

  else {
    fprintf(stderr, "Comando desconhecido\n");
  }
}

void remover_arquivo(dir *pasta_nivel, dir *pasta_nivel_anterior,
                     char *nome_arquivo) {
  arquivodisk *aux = (arquivodisk *)malloc(sizeof(arquivodisk));
  arquivodisk *aux2 = (arquivodisk *)malloc(sizeof(arquivodisk));

  if (pasta_nivel->arquivodir == NULL) {
    fprintf(stderr, "A pasta não possui arquivo\n");
    return;
  }

  aux = pasta_nivel->arquivodir;
  if (strcmp(aux->nome, nome_arquivo) == MATCH) {
    pasta_nivel->arquivodir = aux->proxarquivo;
    remover_arquivo_disco(aux);
    free(aux);
    fprintf(stderr, "Arquivo excluído com sucesso\n");
  } else {
    aux2 = aux;
    aux = aux->proxarquivo;
    while (aux != NULL && strcmp(aux->nome, nome_arquivo)) {
      aux2 = aux->proxarquivo;
      aux = aux->proxarquivo;
    }

    if (aux == NULL) {
      fprintf(stderr, "Arquivo não encontrado\n");
    } else {
      fprintf(stderr, "Arquivo encontrado\n");
      fprintf(stderr, "Arquivo : %s\n", aux->nome);
      remover_arquivo_disco(aux);
      aux2->proxarquivo = aux->proxarquivo;
      free(aux);
    }
  }
}

void remover_arquivo_disco(arquivodisk *arquivo) {
  setor *aux_setor = (setor *)malloc(sizeof(setor));
  setor *aux_setor_depois = (setor *)malloc(sizeof(setor));

  /*Apaga o primeiro setor do arquivo*/
  for (int i = arquivo->inicio; i < (arquivo->inicio + arquivo->tam); i++) {
    mydisk[arquivo->setor].data[i] = 0;
  }
  /*Atualiza o mapa de bits do setor da estrutura arquivo*/
  atualizar_mapabits(arquivo->setor);

  aux_setor = arquivo->proxsetor;

  /*Percorre as estruturas do setores*/
  /*Apaga os dados no disco*/
  while (aux_setor != NULL) {
    /*Apaga o setor*/
    for (int i = aux_setor->inicio; i < (aux_setor->inicio + aux_setor->tam);
         i++) {
      mydisk[aux_setor->setor].data[i] = 0;
    }
    /*Atualiza o mapa de bits do setor que estava o arquivo*/
    atualizar_mapabits(aux_setor->setor);
    aux_setor = aux_setor->proxsetor; /*Avança para o próximo setor*/
  }

  aux_setor = arquivo->proxsetor;
  /*Apaga as estruturas do setor do arquivo, se o arquivo tiver*/
  while (aux_setor != NULL) {
    aux_setor_depois = aux_setor;
    aux_setor = aux_setor->proxsetor;
    free(aux_setor_depois);
  }

  /*Imprime o disco no bloco arquivo->setor*/
  fprintf(stderr, "Imprimindo disco no bloco : %d\n", arquivo->setor);
  for (int i = 0; i < BLOCK_SIZE; i++) {
    fprintf(stderr, "%d", mydisk[arquivo->setor].data[i]);
  }
  fprintf(stderr, "\n");
}

void remover_diretorio(dir *pasta_nivel, dir *pasta_nivel_anterior) {
  if (pasta_nivel->proxnivel != NULL || pasta_nivel->arquivodir != NULL) {
    /*A pasta possui subdiretórios e/ou arquivos e não pode ser apagada*/
    fprintf(stderr, "A pasta não pode ser apagada\n");
    return;
  }

  fprintf(stderr, "A pasta pode ser apagada\n");
  fprintf(stderr, "%s\n", pasta_nivel->nome);
  fprintf(stderr, "%s\n", pasta_nivel_anterior->nome);

  /*Avança para o próximo nível da estrutura auxiliar do nivel anterior*/
  dir **alvo = &pasta_nivel_anterior->proxnivel;
  dir *aux = NULL;

  while (*alvo != NULL && strcmp((*alvo)->nome, pasta_nivel->nome) != MATCH) {
    alvo = &(*alvo)->irmao;
  }

  if (*alvo == NULL) {
    fprintf(stderr, "Pasta não encontrada\n");
    return;
  }

  aux = *alvo;
  *alvo = (*alvo)->irmao;
  free(aux);
}

void adicionar_diretorio(dir *nivel_anterior, char *nome_pasta) {
  dir *direc = (dir *)malloc(sizeof(dir));
  int posicao_setor = pesquisar_mapa_bits_diretorio();

  if (posicao_setor == -1) {
    fprintf(stderr, "Erro: Disco cheio\n");
    return;
  }

  strncpy(direc->nome, nome_pasta, sizeof(direc->nome));
  direc->irmao = NULL;
  direc->proxnivel = NULL;
  direc->arquivodir = NULL;
  direc->posicao_setor = posicao_setor;
  mapa_bits[direc->posicao_setor] = 1;

  dir **alvo = &nivel_anterior->proxnivel;

  /*Existe o próximo nível, então percorre os irmãos*/
  while (*alvo != NULL) {
    alvo = &(*alvo)->irmao;
  }

  /*Atribui para apontar para a nova estrutura*/
  *alvo = direc;

  escrever_disco_diretorio(direc->posicao_setor, 1);
  fprintf(stderr, "Pasta %s criada com sucesso\n", direc->nome);
}

/*
 * Pesquisa o mapa de bits e o disco e grava o arquivo no disco o arquivo nos
 * espaços vazios.
 */
int pesquisar_mapa_bits_arquivo(int data_total, arquivodisk *arquivo) {
  int k = 0;
  int i = BLOCK_BOOT - 1;
  int inicio = 0;
  int bytes_contados = 0;
  int iteracao = 0;

  fprintf(stderr, "Arquivo : %s\n", arquivo->nome);

inicio:
  if (mapa_bits[i] == 0) {
    k = 0;
    inicio = 0;
    bytes_contados = 0;
    while (k < BLOCK_SIZE) {
      inicio = 0;
      bytes_contados = 0;
      if (mydisk[i].data[k] == 0) {
        fprintf(stderr, "Setor : %d Byte : %d\n", i, k);
        inicio = k;
        bytes_contados++;
        k++;

        while (mydisk[i].data[k] == 0 && bytes_contados != data_total &&
               k < BLOCK_SIZE) {
          fprintf(stderr, "Setor : %d Byte : %d\n", i, k);
          bytes_contados++;
          k++;
        }

        fprintf(stderr, "Setor : %d  Inicio : %d Fim : %d\n", i, inicio,
                inicio + bytes_contados - 1);

        data_total = data_total - bytes_contados;

        escrever_disco_arquivo(i, bytes_contados, inicio, arquivo, iteracao);

        iteracao++;

        atualizar_mapabits(i);
        if (data_total == 0) {
          return 0;
        }
      }
      k++;
    }
    i++;
    goto inicio;
  } else {
    if (i < BLOCK_NUMBER) {
      i++;
      goto inicio;
    } else {
      return -1; /*Erro : Disco cheio*/
    }
  }
}

void escrever_disco_arquivo(int bloco, int bytes, int inicio,
                            arquivodisk *arquivo, int iteracao) {
  /*Escrevendo no disco*/
  int i = inicio;
  fprintf(stderr, "I : %d\n", i);
  while (i < (inicio + bytes)) {
    mydisk[bloco].data[i] = 1;
    i++;
  }

  /*Escrevendo na estrutura*/
  if (iteracao == 0) {
    arquivo->setor = bloco;
    arquivo->inicio = inicio;
    arquivo->tam = bytes;
  } else if (iteracao == 1) {
    setor *aux = (setor *)malloc(sizeof(setor));
    aux->setor = bloco;
    aux->tam = bytes;
    aux->inicio = inicio;
    aux->proxsetor = NULL;
    arquivo->proxsetor = aux;
  } else {
    setor *aux = (setor *)malloc(sizeof(setor));
    setor *novo = (setor *)malloc(sizeof(setor));
    aux = arquivo->proxsetor;

    while (aux->proxsetor != NULL) {
      aux = aux->proxsetor;
    }
    novo->setor = bloco;
    novo->tam = bytes;
    novo->inicio = inicio;
    novo->proxsetor = NULL;
    aux->proxsetor = novo;
  }

  /*
      fprintf(stderr,"Imprimindo disco no bloco : %d\n",bloco);
      for(int i = 0;i<BLOCK_SIZE;i++){
        fprintf(stderr,"%d",mydisk[bloco].data[i]);
      }
      fprintf(stderr,"\n");
  */
}
void atualizar_mapabits(int bloco) {
  bool isBlocoCheio = true;
  int i = 0;
  while (i < BLOCK_SIZE) {
    if (mydisk[bloco].data[i] == 0) {
      isBlocoCheio = false;
      break;
    }
    i++;
  }

  if (isBlocoCheio) {
    mapa_bits[bloco] = 1;
  } else {
    mapa_bits[bloco] = 0;
  }
}

void escrever_disco_diretorio(int setor, int data) {
  for (int i = 0; i < BLOCK_SIZE; i++) {
    mydisk[setor].data[i] = data;
  }
}

int pesquisar_mapa_bits_diretorio() {
  int k = 0;
  int i = BLOCK_BOOT;

inicio:
  if (mapa_bits[i] == 0 && i != BLOCK_NUMBER) {
    k = 0;
    while (k != BLOCK_SIZE) {
      if (mydisk[i].data[k] == 1) {
        i++;
        goto inicio;
      }
      k++;
    }
    return i;
  } else {
    if (i != BLOCK_NUMBER) {
      i++;
      goto inicio;
    } else {
      return -1; /*Erro : Disco cheio*/
    }
  }
}

dir *pesquisar_diretorio(dir *mapadiretorio, int nivel, char *nomepasta) {
  dir *aux = mapadiretorio;

  for (int i = 1; i < nivel; i++) {
    if (aux == NULL) {
      return NULL;
    }

    aux = aux->proxnivel;
  }

  if (aux == NULL) {
    return NULL;
  }

  while (strcmp(nomepasta, aux->nome) != MATCH) {
    if (aux->irmao == NULL) {
      return NULL;
    }

    aux = aux->irmao;
  }

  return aux;
}

/*Função para ver os setores do arquivo*/
void verset(dir *diretorio, char *nome_arquivo) {
  arquivodisk *aux = diretorio->arquivodir;

  if (aux == NULL) {
    fprintf(stderr, "A pasta não possui arquivos\n");
    return;
  }

  while (aux != NULL) {
    if (strcmp(aux->nome, nome_arquivo) != MATCH) {
      aux = aux->proxarquivo;
      continue;
    }

    fprintf(stderr, "Setor : %d Inicio : %d Fim : %d \n", aux->setor,
            aux->inicio, aux->inicio + aux->tam - 1);

    setor *aux_setor = aux->proxsetor;

    while (aux_setor != NULL) {
      fprintf(stderr, "Setor : %d Inicio : %d Fim : %d \n", aux_setor->setor,
              aux_setor->inicio, aux_setor->inicio + aux_setor->tam - 1);
      aux_setor = aux_setor->proxsetor;
    }

    return;
  }

  if (aux = NULL) {
    fprintf(stderr, "Arquivo inexistente\n");
  }
}

/*Função ver diretórios*/
void verd(dir *pasta) {
  int quantidade_diretorios = 0;
  int quantidade_arquivos = 0;
  int tam_total_bytes = 0;
  dir *aux = pasta;

  fprintf(stderr, "\n");
  if (aux->proxnivel != NULL) {
    aux = aux->proxnivel;
    fprintf(stderr, "Diretórios\n");

    while (aux != NULL) {
      quantidade_diretorios++;
      fprintf(stderr, "--%s Setor: %d\n", aux->nome, aux->posicao_setor);
      aux = aux->irmao;
    }
  }

  aux = pasta;
  if (aux->arquivodir != NULL) {
    arquivodisk *arquivo_disk = aux->arquivodir;
    fprintf(stderr, "Arquivos\n");

    while (arquivo_disk != NULL) {
      quantidade_arquivos++;
      tam_total_bytes += arquivo_disk->tam;
      fprintf(stderr, "--%s %d bytes %s \n", arquivo_disk->nome,
              arquivo_disk->tam, arquivo_disk->date_time);
      arquivo_disk = arquivo_disk->proxarquivo;
    }
  }

  fprintf(stderr, "\n%d arquivo(s) e um tamanho de: %d byte(s)",
          quantidade_arquivos, tam_total_bytes);
  fprintf(stderr, "\n%d subdiretório(s)\n", quantidade_diretorios);
}

int main() {
  /*Configurando o disco*/
  config_disco();
  iniciar_mapabits();

  /*Inicializando o mapa de diretorios*/
  dir *mapadiretorio = (dir *)malloc(sizeof(dir));
  inicializar_diretorio(mapadiretorio);

  /*Loop infinito até ser detectado o comando sair ser detectado*/
  while (TRUE) {
    imprimir_shell(mapadiretorio);
    lercomando();
    executarcomando(mapadiretorio);
  }
}
