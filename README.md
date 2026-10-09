# STM32F7 Generic BSP Libraries

Biblioteca genérica de drivers de alto nível para microcontroladores **STM32F7** (focada na placa **NUCLEO-F767ZI**), desenvolvida sobre a camada STM32 HAL sem acoplamento entre periféricos.

Este repositório foi projetado para ser consumido como um **Git Submodule** dentro de projetos configurados via **STM32CubeMX** e **STM32CubeIDE**.

---

## 1. Diretrizes de Desenvolvimento (Normas Adotadas)

Todo o código deste repositório segue estritamente as convenções de engenharia de software embarcado para robustez, legibilidade e manutenibilidade:

* **Idioma:**
  * Nomes de identificadores (funções, variáveis, `#defines`, tipos): **Inglês**.
  * Comentários e documentação Doxygen: **Português** (sem caracteres especiais/acentos).
* **Encoding e Indentação:**
  * Arquivos em **UTF-8**, quebra de linha LF (`\n`).
  * Indentação com **4 espaços** (nunca TABs).
  * Abertura de chaves sempre na linha seguinte (**estilo Allman**).
  * Proibido o uso de `if` de uma linha; sempre utilizar `{ }`.
  * Comparações booleanas explícitas (`if(status == true)`).
* **Nomenclatura:**
  * Arquivos: `UpperCamelCase` (ex: `BspUart.c`, `BspUart.h`).
  * Funções: `<NomeArquivo>_<NomeFuncao>` (ex: `BspUart_Init()`).
  * Tipos (`typedef`): `lowerCamelCase` com sufixo `_t` (ex: `bspUart_t`).
  * Defines públicos (`.h`): `d<NOME_ARQUIVO>_<NOME>` (ex: `dBSP_UART_TX_BUFFER_SIZE`).
  * Defines locais (`.c`): `d<NOME>` (ex: `dTIMEOUT_MS`).
  * Enumerações: Prefixo `e` maiúsculo/minúsculo conforme regra: `e<NOME>` (ex: `eSTATUS_OK`).
  * Variáveis e ponteiros: `lowerCamelCase` (ex: `rawData`, `adValue`).
* **Estrutura de Seções:**
  * Os arquivos `.h` e `.c` devem conter **obrigatoriamente** as seções padrão delineadas em `Templates/LibModel.h` e `Templates/LibModel.c`.
  * Variáveis de escopo interno do arquivo `.c` devem ser agrupadas dentro de uma `static struct { ... } <nomeDoModulo>;`.

---

## 2. Estrutura de Pastas

```text
stm32f7-bsp/
├── .editorconfig          # Configuracao automatica de formatacao (4 espacos, UTF-8)
├── .clang-format          # Formatador de codigo C (estilo Allman, sem tab)
├── .gitignore             # Filtro de arquivos de compilacao e temporarios
├── Doxyfile               # Configuracao para geracao automatica de documentacao HTML
├── Makefile               # Script de compilacao e validacao (make, make test, make doc)
├── README.md              # Este guia de uso
├── Common/                # Tipos e estruturas compartilhadas
│   ├── BspTypes.h         # Tipos u8, u16, u32, s8, s16, s32, bool, dTRUE, dFALSE
│   └── BspVersion.h       # Estrutura semantica de versionamento
├── Templates/             # Modelos oficiais para novos modulos
│   ├── LibModel.h         # Template base de header (.h)
│   └── LibModel.c         # Template base de source (.c)
├── Drivers/               # Drivers modulares desacoplados (HAL Wrappers)
│   ├── BspUart/           # Driver de comunicacao serial (UART/USART)
│   ├── BspAdc/            # Driver de aquisicao analogica (ADC)
│   ├── BspDac/            # Driver de conversao digital-analogica (DAC)
│   └── BspSync/           # Sincronismo e controle fiel ao CMSIS-RTOS v2
├── Bsp/                   # Camada unificada de hardware (Secao 4.12 da Norma)
│   ├── Bsp.h              # Ponto de acesso unico da aplicacao (Bsp_Init)
│   └── Bsp.c              # Inicializacoes de baixo nivel e despacho de ISRs
└── Examples/              # Exemplos de uso pratico para a turma
    └── main_example.c     # Integracao demonstrativa de UART + ADC + DAC
```

---

## 3. Como Integrar em Projetos STM32CubeMX / CubeIDE

Para usar estas bibliotecas em qualquer projeto de firmware da turma:

1. **Adicionar como Submódulo Git:**
   Dentro da pasta raiz do seu projeto STM32, crie a pasta `Libs` e vincule este repositório:
   ```bash
   git submodule add <URL_DO_REPOSITORIO> Libs/stm32f7-bsp
   ```

2. **Configurar no STM32CubeIDE:**
   * Clique com botão direito no projeto > **Properties** > **C/C++ General** > **Paths and Symbols**:
     * **Source Location:** Adicione a pasta `Libs/stm32f7-bsp`.
     * **Includes (GNU C):** Adicione:
       * `Libs/stm32f7-bsp/Common`
       * `Libs/stm32f7-bsp/Drivers/<ModuloDesejado>`

3. **Atualização em Grupo:**
   Quando correções ou novos drivers forem publicados:
   ```bash
   git submodule update --remote
   ```
