# mini-fs

🗂️ Um mini sistema de arquivos simulado em C, com estrutura de diretórios, arquivos, alocação em blocos e visualização de mapa de setores.

## 📌 Sobre o projeto

Este projeto é uma simulação de um sistema de arquivos implementado em linguagem C. Ele permite:

- Criar diretórios e arquivos
- Visualizar a estrutura em árvore
- Mapear os setores do "disco"
- Remover arquivos e diretórios
- Visualizar quais setores cada arquivo ocupa

O sistema é baseado em estruturas de dados simples (como listas encadeadas) e simula o armazenamento usando arrays em memória.

---

## ⚙️ Funcionalidades principais

| Comando       | Descrição |
|---------------|-----------|
| `criad`       | Cria um diretório no caminho informado |
| `criaa`       | Cria um arquivo com tamanho especificado |
| `removed`     | Remove um diretório vazio |
| `removea`     | Remove um arquivo |
| `verd`        | Exibe o conteúdo de um diretório |
| `verset`      | Mostra os setores ocupados por um arquivo |
| `mapa`        | Exibe o mapa de setores (bitmap) |
| `arvore`      | Exibe a estrutura em árvore dos diretórios |
| `sair`        | Encerra o sistema |
| `ajuda`       | Mostra a ajuda com todos os comandos disponíveis |

---

## 🛠️ Como compilar e executar

```bash
gcc -o mini-fs main.c
./mini-fs

## 🧪 Exemplo de uso

```bash
criad \raiz\documentos
criaa \raiz\documentos arquivo1.txt 150
verd \raiz\documentos
verset \raiz\documentos arquivo1.txt
mapa
arvore

