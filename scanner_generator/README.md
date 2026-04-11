# Scanner Generator (C++): fluxo completo

Este projeto gera um scanner a partir de regras de tokens (regex), serializa o AFD minimizado em JSON e exporta uma lista de tokens em JSON a partir de um codigo Racket.

## Visao geral do pipeline

1. Ler regras lexicas (`TOKEN REGEX`) de um arquivo `.txt`.
2. Construir AFN global, converter para AFD e minimizar.
3. Serializar o AFD minimizado em JSON.
4. Gerar codigo de scanner em C++.
5. Executar scanner no arquivo Racket de entrada.
6. Exportar tokens para `token_list.json`.

## Arquivos e partes do projeto

### Entradas principais

- `racket_specs.txt`: regras lexicas (tokens e regex).
- `racket_input.rkt`: codigo Racket que sera tokenizado.

### Nucleo do gerador

- `main.cpp`: executavel principal com 2 modos:
  - serializar AFD (`<spec> [saida_json]`)
  - gerar scanner (`generate-scanner <spec> <output_dir>`)
- `scanner_manager.hpp`: leitura de specs, parse de regex e construcao de AFN/AFD.
- `regex_tokenizer.hpp` e `regex_engine.hpp`: tokenizer/parser de regex.
- `mythompson.hpp`: construcao de AFN por Thompson.
- `subset_construction.hpp`: AFN -> AFD.
- `state_minimizator.hpp`: minimizacao do AFD.
- `afd_serializer.hpp`: escrita do AFD minimizado em JSON.

### Geracao de scanner

- `generate_scanner_code.hpp` / `generate_scanner_code.cpp`: geram codigo C++ do scanner.
- `out_scanner/scanner_gerado.hpp`: interface do scanner gerado.
- `out_scanner/scanner_gerado.cpp`: implementacao do scanner gerado.

### Exportacao de tokens em JSON

- `export_tokens_json.cpp`: programa fixo que:
  - le `racket_input.rkt`
  - chama `scan(...)` do scanner gerado
  - grava `token_list.json`

### Build e debug

- `Makefile`: alvos de compilacao e execucao.
- `debug_main.cpp`: imprime estruturas intermediarias (AFN/AFD) para depuracao.

### Arquivos de saida esperados

- `racket_afd.json`: AFD minimizado serializado.
- `out_scanner/scanner_gerado.hpp` e `out_scanner/scanner_gerado.cpp`: scanner gerado.
- `scanner_tokens_json`: executavel que exporta tokens.
- `token_list.json`: lista final de tokens para parser.

## Pre-requisitos

- Linux/macOS com `g++` e `make`
- Compilador com suporte a C++17

## Como rodar tudo do zero

Execute os comandos abaixo na raiz do projeto:

```bash
cd /home/amaro/scanner_generator/scanner_generator
```

### 1) Limpar binarios antigos

```bash
make clean
```

### 2) Compilar o gerador principal

```bash
make scanner
```

### 3) Serializar AFD minimizado em JSON

```bash
make export-afd SPEC=racket_specs.txt AFD_OUT=racket_afd.json
```

### 4) Gerar scanner C++ a partir das regras

```bash
./scanner generate-scanner racket_specs.txt ./out_scanner
```

### 5) Compilar e rodar exportador de tokens

```bash
make export-tokens
```

Esse passo gera/atualiza `token_list.json` usando `racket_input.rkt`.

### 6) Ver o resultado no terminal

```bash
cat token_list.json
```

## Formato das regras lexicas

Cada linha do arquivo de specs segue:

```txt
TOKEN REGEX
```

Exemplo:

```txt
LPAREN \(
RPAREN \)
DEFINE define
IF if
NUMBER (\-[0-9]+)|([0-9]+)
SYMBOL [a-zA-Z_][a-zA-Z0-9_]*
```

## Comando unico (pipeline completo)

```bash
cd /home/amaro/scanner_generator/scanner_generator && \
make clean && \
make scanner && \
make export-afd SPEC=racket_specs.txt AFD_OUT=racket_afd.json && \
./scanner generate-scanner racket_specs.txt ./out_scanner && \
make export-tokens && \
cat token_list.json
```

## Erros comuns

- `No rule to make target out_scanner/scanner_gerado.cpp`:
  - Rode antes: `./scanner generate-scanner racket_specs.txt ./out_scanner`
- `regexes_ex.txt` nao encontrado:
  - Passe `SPEC=...` explicitamente (ex.: `SPEC=racket_specs.txt`)
- `token_list.json` nao atualiza:
  - Verifique se `racket_input.rkt` foi salvo antes de `make export-tokens`
