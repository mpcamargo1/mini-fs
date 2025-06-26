#ifndef UTILS_H
#define UTILS_H

#define BLOCK_SIZE 512
#define BLOCK_NUMBER 256
#define BLOCK_BOOT 10

/*Estrutura do HD*/
typedef struct struct_disk {
  char data[BLOCK_SIZE];
  /*Mapa de bits do disco*/
  int mapa_bits[BLOCK_NUMBER]; /*1 - Cheio, 0 - Não cheio*/
} disk;

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

bool is_string_empty(const char *str);

#endif