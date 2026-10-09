/******************************************************************************/
/**
* @file BspUart.h
* @addtogroup BSP_UART
* @{
******************************************************************************/
#ifndef _BSP_UART_H_
#define _BSP_UART_H_

/*******************************************************************************
* INCLUDES NECESSARIOS
******************************************************************************/
#include "BspTypes.h"

#if defined(__has_include)
#if __has_include("stm32f7xx_hal.h")
#include "stm32f7xx_hal.h"
#endif
#endif

// Declaracao antecipada para permitir compilacao desacoplada
#ifndef HAL_UART_MODULE_ENABLED
typedef struct __UART_HandleTypeDef UART_HandleTypeDef;
#endif

/*******************************************************************************
* CONFIGURACOES
******************************************************************************/
/// Tamanho do buffer circular de recepcao serial por software
#define dBSP_UART_RX_BUFFER_SIZE            128                                 // [bytes]

/// Timeout padrao de transmissao bloqueante em milissegundos
#define dBSP_UART_DEFAULT_TIMEOUT_MS        1000                                // [ms]

/// Habilita ou desabilita a funcao BspUart_Printf
#define dBSP_UART_ENABLE_PRINTF             dTRUE                               // [dTRUE ou dFALSE]

/*******************************************************************************
* DEFINES PUBLICOS
******************************************************************************/

/*******************************************************************************
* TIPOS DE DADOS PUBLICOS
******************************************************************************/
/// Estrutura de controle da instancia da porta UART
typedef struct
{
    /// Ponteiro para o handle gerado pelo CubeMX
    UART_HandleTypeDef *huart;
    /// Buffer de recepcao circular interno
    u8 rxBuffer[dBSP_UART_RX_BUFFER_SIZE];
    /// Posicao de insercao do buffer circular (alimentado na interrupcao)
    volatile u16 rxHead;
    /// Posicao de retirada do buffer circular (consumido pela aplicacao)
    volatile u16 rxTail;
    /// Quantidade de bytes disponiveis para leitura
    volatile u16 rxCount;
    /// Flag de estado de inicializacao da porta
    bool isInitialized;
} bspUart_t;

/*******************************************************************************
* PROTOTIPOS PUBLICOS
******************************************************************************/
/******************************************************************************/
/** @brief Inicializa a instancia da UART vinculando ao handle do CubeMX.
* @param dev: ponteiro para a estrutura de dados da UART.
* @param huart: ponteiro para o handle HAL gerado pelo CubeMX (ex: &huart3).
* @retval eSTATUS_OK se sucesso, ou eSTATUS_INVALID_PARAM se ponteiro nulo.
* @details A funcao zera os indices do buffer circular de recepcao, associa o
* handle do periférico e marca a instancia como ativa.
*
* Exemplo de uso:
* @code
* bspUart_t consoleUart;
* BspUart_Init(&consoleUart, &huart3);
* @endcode
******************************************************************************/
status_t BspUart_Init(bspUart_t *dev, UART_HandleTypeDef *huart);

/******************************************************************************/
/** @brief Envia um unico byte pela UART.
* @param dev: ponteiro para a estrutura da UART.
* @param byte: valor de 8 bits a ser transmitido.
* @retval eSTATUS_OK se transmitido, eSTATUS_TIMEOUT ou eSTATUS_ERROR caso falha.
* @details A funcao invoca a transmissao da HAL aguardando a liberacao do registrador
* de saida ate o timeout configurado.
*
* Exemplo de uso:
* @code
* BspUart_SendByte(&consoleUart, 'A');
* @endcode
******************************************************************************/
status_t BspUart_SendByte(bspUart_t *dev, u8 byte);

/******************************************************************************/
/** @brief Transmite um buffer de bytes pela UART.
* @param dev: ponteiro para a estrutura da UART.
* @param buffer: ponteiro para o array de dados a serem enviados.
* @param size: quantidade de bytes a transmitir.
* @retval eSTATUS_OK se sucesso, eSTATUS_TIMEOUT ou eSTATUS_INVALID_PARAM se erro.
* @details Realiza o envio em bloco de um conjunto de dados utilizando a camada
* HAL com protecao contra ponteiros nulos ou tamanho zerado.
*
* Exemplo de uso:
* @code
* u8 pacote[4] = {0xAA, 0x01, 0x02, 0x55};
* BspUart_SendBuffer(&consoleUart, pacote, 4);
* @endcode
******************************************************************************/
status_t BspUart_SendBuffer(bspUart_t *dev, const u8 *buffer, u16 size);

/******************************************************************************/
/** @brief Envia uma string terminada em zero (null-terminated).
* @param dev: ponteiro para a estrutura da UART.
* @param str: string de texto constante a ser enviada.
* @retval eSTATUS_OK se enviada com sucesso, ou codigo de erro.
* @details Calcula automaticamente o tamanho da cadeia de caracteres via strlen
* e transmite todos os bytes ate o terminador nulo sem necessidade de passar o tamanho.
*
* Exemplo de uso:
* @code
* BspUart_SendString(&consoleUart, "Sistema Pronto!\r\n");
* @endcode
******************************************************************************/
status_t BspUart_SendString(bspUart_t *dev, const char *str);

#if dBSP_UART_ENABLE_PRINTF == dTRUE
/******************************************************************************/
/** @brief Imprime texto formatado na UART similar a printf.
* @param dev: ponteiro para a estrutura da UART.
* @param format: string de formato com especificadores (%d, %s, %f, etc.).
* @param ...: lista de argumentos variaveis.
* @retval eSTATUS_OK se enviado com sucesso, ou eSTATUS_ERROR se formatacao falhar.
* @details Utiliza vsnprintf internamente para compor a mensagem em um buffer
* temporario e despachar via BspUart_SendBuffer sem travar a aplicacao.
*
* Exemplo de uso:
* @code
* u16 adcVal = 2048;
* f32 tensao = 1.65f;
* BspUart_Printf(&consoleUart, "Leitura: %u | Tensao: %.2f V\r\n", adcVal, tensao);
* @endcode
******************************************************************************/
status_t BspUart_Printf(bspUart_t *dev, const char *format, ...);
#endif

/******************************************************************************/
/** @brief Retorna a quantidade de bytes disponiveis para leitura no buffer.
* @param dev: ponteiro para a estrutura da UART.
* @retval Quantidade de bytes acumulados no buffer de recepcao.
* @details Consulta a contagem de bytes sem alterar o estado ou ponteiros
* do buffer circular.
*
* Exemplo de uso:
* @code
* if(BspUart_Available(&consoleUart) > 0)
* {
*     // Existem dados a serem processados
* }
* @endcode
******************************************************************************/
u16 BspUart_Available(const bspUart_t *dev);

/******************************************************************************/
/** @brief Le o proximo byte disponivel no buffer de recepcao.
* @param dev: ponteiro para a estrutura da UART.
* @param byte: ponteiro onde sera armazenado o byte lido.
* @retval eSTATUS_OK se o byte foi lido, eSTATUS_TIMEOUT se o buffer estiver vazio.
* @details Retira o elemento mais antigo do buffer circular, incrementa o indice
* tail e decrementa a contagem de forma segura.
*
* Exemplo de uso:
* @code
* u8 recebido = 0;
* if(BspUart_ReceiveByte(&consoleUart, &recebido) == eSTATUS_OK)
* {
*     // Trata o byte recebido
* }
* @endcode
******************************************************************************/
status_t BspUart_ReceiveByte(bspUart_t *dev, u8 *byte);

/******************************************************************************/
/** @brief Le uma linha completa de texto ate encontrar '\n' ou '\r'.
* @param dev: ponteiro para a estrutura da UART.
* @param buffer: destino onde a linha sera copiada terminada em '\0'.
* @param maxLen: tamanho maximo suportado pelo buffer de destino.
* @retval Quantidade de caracteres gravados na linha (excluindo terminador nulo).
* @details Varre o buffer circular procurando finalizadores de linha. Ao encontrar,
* extrai os caracteres preenchendo a string com seguranca contra estouro de limite.
* Se a linha ainda nao estiver completa, nao consome nada e retorna 0.
*
* Exemplo de uso:
* @code
* char linhaComando[32];
* u16 qtd = BspUart_ReadLine(&consoleUart, linhaComando, sizeof(linhaComando));
* if(qtd > 0)
* {
*     BspUart_Printf(&consoleUart, "Comando recebido: %s\r\n", linhaComando);
* }
* @endcode
******************************************************************************/
u16 BspUart_ReadLine(bspUart_t *dev, char *buffer, u16 maxLen);

/******************************************************************************/
/** @brief Manipulador de interrupcao de recepcao a ser chamado no callback HAL.
* @param dev: ponteiro para a estrutura da UART correspondente.
* @param byte: byte recebido da UART pelo hardware.
* @retval Nenhum.
* @details Insere o byte recebido no buffer circular e avanca o ponteiro head.
* Caso o buffer atinja a capacidade maxima, o byte mais antigo e sobrescrito.
* Deve ser invocado dentro de HAL_UART_RxCpltCallback.
*
* Exemplo de uso:
* @code
* void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
* {
*     if(huart->Instance == USART3)
*     {
*         BspUart_RxInterruptHandler(&consoleUart, byteRecebidoDoHardware);
*         HAL_UART_Receive_IT(huart, &byteRecebidoDoHardware, 1);
*     }
* }
* @endcode
******************************************************************************/
void BspUart_RxInterruptHandler(bspUart_t *dev, u8 byte);

#endif /* _BSP_UART_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */
