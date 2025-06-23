/* Marcos P.Camargo
   Vinicius Dornelles*/

/*Bibliotecas*/
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

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

/*Estrutura Disco*/
typedef struct struct_disk {
  char data[BLOCK_SIZE];
} disk;

disk mydisk[BLOCK_NUMBER];

/*Estrutura Diretorio*/
typedef struct struct_directory {
  char nome[32];
  int posicao_setor;
  struct struct_arquivo *arquivodir;
  struct struct_directory *irmao;
  struct struct_directory *proxnivel;
} dir;

/*Estrutura Arquivo*/
typedef struct struct_arquivo {
  int setor;
  char nome[32];
  int inicio;
  int tam;
  char date_time[32];
  struct struct_setor *proxsetor;
  struct struct_arquivo *proxarquivo;
} arquivodisk;

/*Estrutura setor*/
typedef struct struct_setor {
  int setor;
  int inicio;
  int tam;
  struct struct_setor *proxsetor;
} setor;

typedef struct struct_tabela {
  char setor;
  int num_blocos;
} tab_blocos;

tab_blocos tabela[BLOCK_NUMBER];

int mapa_bits[BLOCK_NUMBER]; /*1 - Cheio, 0 - Não cheio*/

/*Vetor utilizado para armazenar os comandos simples*/
char *comando_shell[MAX_ARGS];
char *pastausuario[MAX_ARGS];

/*Declaração das funções*/

/*Comandos*/
void executarcomando(dir *mapadiretorio);
void verd(dir *mapadiretorio);
void verset(dir *mapadiretorio, char *nome_arquivo);
void print_ajuda();

/*Leitura de comandos*/
void lercomando();
int string_tokenizer();

/*Inicialização*/
void config_disco();
int iniciar_mapabits();
int iniciar_diretorio(dir *mapadiretorio);

/*Pesquisar*/
dir *pesquisar_diretorio(dir *mapadiretorio, int nivel);
int pesquisar_mapa_bits_diretorio();
int pesquisar_mapa_bits_arquivo(int data_total, arquivodisk *arquivo);

/*Criar*/
void criar_arquivo(dir *mapadiretorio, char *nome, int data);
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
  ;
  int i = 0;
  while (token != NULL) {
    pastausuario[i++] = token;
    token = strtok(NULL, "\\");
  }
  for (int k = 0; k < i; k++) {
    fprintf(stderr, "%s\n", pastausuario[k]);
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
  /*Comando de criação
  fprintf(stderr,"Criar diretório  ");
    fprintf(stderr,"Sintaxe : criad \\caminhocompleto\\novapasta\n");
  fprintf(stderr,"Criar arquivo\n");
    fprintf(stderr,"--Sintaxe : criaa \\caminhocompleto novoarquivo\n");
  fprintf(stderr,"Remover diretório\n");
    fprintf(stderr,"--Sintaxe : removed \\caminhocompleto\\diretorioexcluir\n");
  fprintf(stderr,"Remover arquivo\n");
    fprintf(stderr,"--Sintaxe : removea \\caminhocompleto arquivo\n");  */

  /*Comandos de visualização
  fprintf(stderr,"Ver informações do diretório\n");
    fprintf(stderr,"--Sintaxe : verd \\diretoriocompleto\n");*/
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
  arquivodisk *aux = (arquivodisk *)malloc(sizeof(arquivodisk));
  aux = arquivo;

  if (strcmp(aux->nome, nome) == MATCH) return -1;

  while (aux->proxarquivo != NULL) {
    aux = aux->proxarquivo;
    if (strcmp(aux->nome, nome) == MATCH) return -1;
  }
  return 0;
}

void criar_arquivo(dir *mapadiretorio, char *nome, int data) {
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

  int cont = 0;
  for (int col = 0; col < 16; col++) {
    for (int lin = 0; lin < 16; lin++) {
      printf("%d", mapa_bits[cont]);
      cont++;
    }
    printf("\n");
  }

  printf("\n");
}

void exibe_arvore(dir *mapadiretorio, int rcont) {
  // rcont = contador de recursao, para poder colocar os espaçamentos
  // corretos(apenas visual)

  dir *dir_aux = (dir *)malloc(sizeof(dir));
  dir_aux = mapadiretorio;

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

void executarcomando(dir *mapadiretorio) {
  /*Caso o comando for "sair"*/
  if (strcmp(comando_shell[0], SAIR) == MATCH) {
    exit(0);
  } else if (strcmp(comando_shell[0], AJUDA) == MATCH) {
    print_ajuda();
  } else if (strcmp(comando_shell[0], CRIARDIR) == MATCH) {
    /*Separa as pasta pela \*/

    int i = string_tokenizer();
    dir *novo = (dir *)malloc(sizeof(dir));
    novo = pesquisar_diretorio(mapadiretorio, i - 1);

    if (novo != NULL) {
      fprintf(stderr, "Pasta : %s\n", novo->nome);
      adicionar_diretorio(novo, pastausuario[i - 1]);
      imprimir_diretorio(mapadiretorio);
    } else {
      fprintf(stderr, "Pasta não encontrada\n");
      fprintf(stderr,
              "Dica : Somente é permitido criar diretórios dentro da raiz\n");
    }
    /*Finalizando a string*/
    comando_shell[0] = "\0";
    pastausuario[1] = "\0";
  } else if (strcmp(comando_shell[0], VERD) == MATCH) {
    /*Separa as pasta pela \*/
    int i = string_tokenizer();
    dir *novo = (dir *)malloc(sizeof(dir));
    novo = pesquisar_diretorio(mapadiretorio, i);

    if (novo != NULL) {
      fprintf(stderr, "%s\n", novo->nome);
      verd(novo);
    } else {
      fprintf(stderr, "Pasta inexistente\n");
    }
  } else if (strcmp(comando_shell[0], CRIARARQ) == MATCH) {
    int i = string_tokenizer();
    dir *novo = (dir *)malloc(sizeof(dir));
    novo = pesquisar_diretorio(mapadiretorio, i);

    if (novo != NULL && comando_shell[2] != NULL && comando_shell[3] != NULL) {
      criar_arquivo(novo, comando_shell[2], atoi(comando_shell[3]));
    } else {
      fprintf(stderr, "Pasta inexistente e/ou sintaxe incorreta\n");
    }
  } else if (strcmp(comando_shell[0], REMOVEDIR) == MATCH) {
    int i = string_tokenizer();
    dir *nivel = (dir *)malloc(sizeof(dir));
    dir *nivel_anterior = (dir *)malloc(sizeof(dir));
    nivel = pesquisar_diretorio(mapadiretorio, i);
    nivel_anterior = pesquisar_diretorio(mapadiretorio, i - 1);

    if (nivel != NULL) {
      remover_diretorio(nivel, nivel_anterior);
    } else {
      fprintf(stderr, "Pasta inexistente\n");
    }
  } else if (strcmp(comando_shell[0], REMOVEARQ) == MATCH) {
    int i = string_tokenizer();
    dir *nivel = (dir *)malloc(sizeof(dir));
    dir *nivel_anterior = (dir *)malloc(sizeof(dir));
    nivel = pesquisar_diretorio(mapadiretorio, i);
    nivel_anterior = pesquisar_diretorio(mapadiretorio, i - 1);

    if (nivel != NULL && comando_shell[2]) {
      remover_arquivo(nivel, nivel_anterior, comando_shell[2]);
    } else {
      fprintf(stderr, "Pasta inexistente e/ou sintaxe incorreta\n");
    }
  } else if (strcmp(comando_shell[0], VERSET) == MATCH) {
    int i = string_tokenizer();
    dir *nivel = (dir *)malloc(sizeof(dir));
    nivel = pesquisar_diretorio(mapadiretorio, i);

    if (nivel != NULL && comando_shell[2] != NULL) {
      verset(nivel, comando_shell[2]);
    } else {
      fprintf(stderr, "Sintaxe incorreta e/ou diretório incorreto\n");
    }
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
  } else {
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
    atualizar_mapabits(
        aux_setor
            ->setor); /*Atualiza o mapa de bits do setor que estava o arquivo*/
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
  /*Alocando memória para as estruturas auxiliares que percorrem o mapa de
   * diretórios*/
  dir *aux = (dir *)malloc(sizeof(dir));
  dir *aux2 = (dir *)malloc(sizeof(dir));

  /*Checando se a pasta não contem subdiretórios e arquivos*/
  if (pasta_nivel->proxnivel == NULL && pasta_nivel->arquivodir == NULL) {
    fprintf(stderr, "A pasta pode ser apagada\n");
    fprintf(stderr, "%s\n", pasta_nivel->nome);
    fprintf(stderr, "%s\n", pasta_nivel_anterior->nome);

    /*Avança para o próximo nível da estrutura auxiliar do nivel anterior*/
    aux = pasta_nivel_anterior->proxnivel;
    /*Verifica se ocorre o match do nome da pasta aux com a pasta_nivel*/
    if (strcmp(aux->nome, pasta_nivel->nome) == MATCH) {
      /* Atualiza o mapa de bits daquele setor */
      mapa_bits[aux->posicao_setor] = 0;
      /*Escreve no disco o diretório*/
      escrever_disco_diretorio(aux->posicao_setor, 0);

      /*A pasta do nivel anterior aponta para o irmao da pasta deletada*/
      pasta_nivel_anterior->proxnivel = aux->irmao;
      /*Libera recursos da estrutura aux*/
      free(aux);
    } else {
      /*Se não percorre os irmãos até encontrar a pasta*/
      aux2 = aux;
      aux = aux->irmao;
      while (strcmp(aux->nome, pasta_nivel->nome)) {
        aux2 = aux2->irmao;
        aux = aux->irmao;
      }

      fprintf(stderr, "Achei %s\n", aux->nome);
      /*Atualiza o mapa de bits daquele setor*/
      mapa_bits[aux->posicao_setor] = 0;
      /*Escreve no disco o diretório*/
      escrever_disco_diretorio(aux->posicao_setor, 0);
      aux2->irmao = aux->irmao;
      /*Libera recursos da estrutura aux*/
      free(aux);
    }
  } else {
    /*A pasta possui subdiretórios e/ou arquivos e não pode ser apagada*/
    fprintf(stderr, "A pasta não pode ser apagada\n");
  }
}

void adicionar_diretorio(dir *aux, char *nome_pasta) {
  dir *direc = (dir *)malloc(sizeof(dir));
  dir *aux2 = (dir *)malloc(sizeof(dir));

  if (aux->proxnivel == NULL) {
    strncpy(direc->nome, nome_pasta, sizeof(direc->nome));
    direc->irmao = NULL;
    direc->proxnivel = NULL;
    direc->arquivodir = NULL;
    direc->posicao_setor = pesquisar_mapa_bits_diretorio();
    mapa_bits[direc->posicao_setor] = 1;
    aux->proxnivel = direc;
    escrever_disco_diretorio(direc->posicao_setor, 1);
    fprintf(stderr, "Pasta %s criada com sucesso\n", direc->nome);
  } else {
    aux = aux->proxnivel;
    if (aux->irmao == NULL) {
      strncpy(direc->nome, nome_pasta, sizeof(direc->nome));
      direc->irmao = NULL;
      direc->proxnivel = NULL;
      direc->arquivodir = NULL;
      direc->posicao_setor = pesquisar_mapa_bits_diretorio();
      mapa_bits[direc->posicao_setor] = 1;
      aux->irmao = direc;
      escrever_disco_diretorio(direc->posicao_setor, 1);
      fprintf(stderr, "Adicionado %s\n", direc->nome);
    } else {
      aux2 = aux->irmao;
      while (aux2 != NULL) {
        aux2 = aux2->irmao;
        aux = aux->irmao;
      }
      strncpy(direc->nome, nome_pasta, sizeof(direc->nome));
      direc->posicao_setor = pesquisar_mapa_bits_diretorio();
      direc->irmao = NULL;
      mapa_bits[direc->posicao_setor] = 1;
      direc->proxnivel = NULL;
      direc->arquivodir = NULL;
      escrever_disco_diretorio(direc->posicao_setor, 1);
      fprintf(stderr, "Parou %s\n", aux->nome);
      aux->irmao = direc;
    }
  }
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
  int i = 0;
  int update = 0;
  while (i < BLOCK_SIZE) {
    if (mydisk[bloco].data[i] == 0) {
      update = 1;
      mapa_bits[bloco] = 0;
      break;
    }
    i++;
  }
  if (update == 0) {
    mapa_bits[bloco] = 1;
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
      return 0;
    }
  }
}

dir *pesquisar_diretorio(dir *mapadiretorio, int nivel) {
  dir *aux = (dir *)malloc(sizeof(dir));
  aux = mapadiretorio;
  for (int i = 0; i < nivel; i++) {
    fprintf(stderr, "Pasta %s\n", aux->nome);
    if (aux != NULL) {
      if (strcmp(pastausuario[i], aux->nome) == MATCH) {
        if (i == nivel - 1) {
          fprintf(stderr, "Pasta encontrada \n");
          return aux;
        } else {
          fprintf(stderr, "Pasta encontrada %s\n", aux->nome);
          aux = aux->proxnivel;
        }
      } else {
        if (aux->irmao != NULL) {
          aux = aux->irmao;
          while (aux != NULL) {
            if (strcmp(pastausuario[i], aux->nome) == MATCH) {
              if (i == nivel - 1) {
                fprintf(stderr, "Pasta encontrada\n");
                return aux;
              } else {
                fprintf(stderr, "Pasta encontrada %s\n", aux->nome);
                aux = aux->proxnivel;
                break;
              }
            } else {
              aux = aux->irmao;
            }
          }
        } else {
          fprintf(stderr, "Pasta não encontrada\n");
          return NULL;
        }
      }
    } else
      return NULL;
  }
}

/*Função para ver os setores do arquivo*/
void verset(dir *diretorio, char *nome_arquivo) {
  arquivodisk *aux = (arquivodisk *)malloc(sizeof(arquivodisk));

  if (diretorio->arquivodir == NULL) {
    fprintf(stderr, "A pasta não possui arquivos\n");
  } else {
    aux = diretorio->arquivodir;

    while (aux != NULL) {
      if (strcmp(aux->nome, nome_arquivo) == MATCH) {
        fprintf(stderr, "Setor : %d Inicio : %d Fim : %d \n", aux->setor,
                aux->inicio, aux->inicio + aux->tam - 1);

        setor *aux_setor = (setor *)malloc(sizeof(setor));
        aux_setor = aux->proxsetor;

        while (aux_setor != NULL) {
          fprintf(stderr, "Setor : %d Inicio : %d Fim : %d \n",
                  aux_setor->setor, aux_setor->inicio,
                  aux_setor->inicio + aux_setor->tam - 1);
          aux_setor = aux_setor->proxsetor;
        }

        break;
      } else {
        aux = aux->proxarquivo;
      }
    }
    if (aux = NULL) {
      fprintf(stderr, "Arquivo inexistente\n");
    }
  }
}

/*Função ver diretórios*/
void verd(dir *pasta) {
  int dir_cont = 0;
  int arq_cont = 0;
  int tam_total = 0;
  dir *aux = (dir *)malloc(sizeof(dir));
  aux = pasta;

  fprintf(stderr, "\n");
  if (aux->proxnivel != NULL) {
    aux = aux->proxnivel;
    fprintf(stderr, "Diretórios\n");
    while (aux != NULL) {
      dir_cont++;
      fprintf(stderr, "--%s Setor : %d\n", aux->nome, aux->posicao_setor);
      aux = aux->irmao;
    }
  }

  aux = pasta;
  if (aux->arquivodir != NULL) {
    arquivodisk *arquivo_disk = (arquivodisk *)malloc(sizeof(arquivodisk));
    arquivo_disk = aux->arquivodir;
    fprintf(stderr, "Arquivos\n");

    while (arquivo_disk != NULL) {
      arq_cont++;
      tam_total += arquivo_disk->tam;
      fprintf(stderr, "--%s %d bytes %s \n", arquivo_disk->nome,
              arquivo_disk->tam, arquivo_disk->date_time);
      arquivo_disk = arquivo_disk->proxarquivo;
    }
  }

  fprintf(stderr, "\n%d arquivo(s) e um tamanho de: %d byte(s)", arq_cont,
          tam_total);
  fprintf(stderr, "\n%d subdiretório(s)\n", dir_cont);
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
