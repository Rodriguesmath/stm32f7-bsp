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
│   ├── BspUart/           # Driver de comunicacao serial (UART/USART) + DMA TX/RX IDLE
│   ├── BspAdc/            # Driver de aquisicao analogica (ADC) + DMA Streaming
│   ├── BspDac/            # Driver de conversao digital-analogica (DAC) + DMA Waveforms
│   ├── BspSync/           # Sincronismo e controle fiel ao CMSIS-RTOS v2
│   └── BspDma/            # Gerenciador de D-Cache (Cortex-M7), alinhamento 32B e monitoramento
├── Bsp/                   # Camada unificada de hardware (Secao 4.12 da Norma)
│   ├── Bsp.h              # Ponto de acesso unico da aplicacao (Bsp_Init)
│   └── Bsp.c              # Inicializacoes de baixo nivel e despacho de ISRs
└── Examples/              # Exemplos de uso pratico para a turma
    └── main_example.c     # Integracao demonstrativa de UART + ADC + DAC
```

---

## 3. Como Importar e Usar em Projetos STM32CubeMX / CubeIDE

Siga o passo a passo abaixo para importar esta biblioteca em qualquer projeto recém-gerado pelo STM32CubeMX no STM32CubeIDE:

### Fase 1: Baixar a biblioteca para o projeto
Abra o terminal na pasta raiz do seu projeto STM32 e execute:

```bash
# 1. Cria a pasta Libs (se ainda nao existir)
mkdir -p Libs

# 2. Adiciona a biblioteca como submodulo Git (Recomendado)
git submodule add https://github.com/Rodriguesmath/stm32f7-bsp.git Libs/stm32f7-bsp

# OU clone diretamente se nao estiver usando controle de versao no projeto:
# git clone https://github.com/Rodriguesmath/stm32f7-bsp.git Libs/stm32f7-bsp
```

---

### Fase 2: Configurar o STM32CubeIDE (Apenas 2 ajustes)

No **STM32CubeIDE**, clique com o botão direito no nome do projeto (na árvore à esquerda) e selecione **Properties**:

1. **Configurar o "Source Location" (Para compilar os arquivos `.c`):**
   * Vá em **C/C++ General** > **Paths and Symbols**;
   * Clique na aba **Source Location**;
   * Clique no botão **Add Folder...**;
   * Selecione a pasta **`Libs`** e confirme em **OK**.
   *(Isso avisa ao compilador GCC para incluir e compilar todos os fontes dentro de `Libs`).*

2. **Configurar os "Include Paths" (Para localizar os arquivos `.h`):**
   * Na mesma janela de *Paths and Symbols*, clique na aba **Includes**;
   * Selecione a linguagem **GNU C**;
   * Clique em **Add...** > depois clique em **Workspace...**;
   * Adicione os caminhos dos módulos desejados:
     * `/${ProjName}/Libs/stm32f7-bsp/Common`
     * `/${ProjName}/Libs/stm32f7-bsp/Drivers/BspUart`
     * `/${ProjName}/Libs/stm32f7-bsp/Drivers/BspAdc`
     * `/${ProjName}/Libs/stm32f7-bsp/Drivers/BspDac`
     * `/${ProjName}/Libs/stm32f7-bsp/Drivers/BspDma`
     * `/${ProjName}/Libs/stm32f7-bsp/Drivers/BspSync`
   * Clique em **Apply and Close**.

---

### Fase 3: Usar no código (`Core/Src/main.c`)

No arquivo `main.c` gerado pelo CubeMX, insira as chamadas nas tags de usuário reservadas:

```c
/* USER CODE BEGIN Includes */
#include "BspUart.h"
#include "BspAdc.h"
#include "BspDac.h"
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
bspUart_t console;
bspAdc_t  sensor;
/* USER CODE END PV */

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  /* Inicializacoes de baixo nivel geradas pelo CubeMX */
  MX_GPIO_Init();
  MX_USART3_UART_Init();
  MX_ADC1_Init();

  /* USER CODE BEGIN 2 */
  // Conecte os handles gerados pelo CubeMX as nossas bibliotecas:
  BspUart_Init(&console, &huart3);
  BspAdc_Init(&sensor, &hadc1, 3.3f);

  BspUart_SendString(&console, "Biblioteca BSP importada com sucesso!\r\n");
  /* USER CODE END 2 */

  while (1)
  {
    /* USER CODE BEGIN 3 */
    f32 tensao = 0.0f;
    if (BspAdc_ReadFilteredVoltage(&sensor, &tensao) == eSTATUS_OK)
    {
        BspUart_Printf(&console, "Tensao lida: %.2f V\r\n", tensao);
    }
    HAL_Delay(500);
    /* USER CODE END 3 */
  }
}
```

---

### Fase 4: Atualizações em Grupo (Git)

Quando você ou qualquer colega adicionar novas funções ou melhorias ao repositório central:
```bash
git submodule update --remote
```
Todos os projetos vinculados recebem a versão atualizada instantaneamente!
