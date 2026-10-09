/******************************************************************************/
/**
* @file BspDma.h
* @addtogroup BSP_DMA
* @{
******************************************************************************/
#ifndef _BSP_DMA_H_
#define _BSP_DMA_H_

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
#ifndef HAL_DMA_MODULE_ENABLED
typedef struct __DMA_HandleTypeDef DMA_HandleTypeDef;
#endif

/*******************************************************************************
* CONFIGURACOES
******************************************************************************/
/// Tamanho da linha de D-Cache do processador ARM Cortex-M7 em bytes
#define dBSP_DMA_CACHE_LINE_SIZE_BYTES      32                                  // [bytes]

/*******************************************************************************
* DEFINES PUBLICOS
******************************************************************************/
/// Macro para forcar alinhamento de buffers em multiplos da linha de cache (32 bytes)
#define dBSP_DMA_BUFFER_ALIGN               __attribute__((aligned(32)))

/// Macro para arredondar o tamanho de memoria para cima em multiplos de 32 bytes
#define dBSP_DMA_ALIGN_SIZE(SIZE)           (((SIZE) + 31U) & ~31U)

/*******************************************************************************
* TIPOS DE DADOS PUBLICOS
******************************************************************************/

/*******************************************************************************
* PROTOTIPOS PUBLICOS
******************************************************************************/
/******************************************************************************/
/** @brief Descarrega a D-Cache da CPU para a memoria SRAM fisica (Clean).
* @param address: ponteiro para o inicio do buffer de dados.
* @param sizeInBytes: tamanho do buffer em bytes a ser sincronizado.
* @retval Nenhum.
* @details No Cortex-M7, dados gravados pela CPU podem ficar temporariamente
* retidos na D-Cache. Esta funcao forca a escrita para a memoria fisica antes que
* o DMA inicie a leitura/transmissao de perifericos como UART TX ou DAC.
*
* Exemplo de uso:
* @code
* // Antes de disparar transmissao serial via DMA:
* BspDma_CacheClean(bufferTx, tamanhoTx);
* HAL_UART_Transmit_DMA(&huart3, bufferTx, tamanhoTx);
* @endcode
******************************************************************************/
void BspDma_CacheClean(void *address, u32 sizeInBytes);

/******************************************************************************/
/** @brief Invalida a D-Cache forcando a CPU a ler a SRAM atualizada (Invalidate).
* @param address: ponteiro para o inicio do buffer recebido pelo DMA.
* @param sizeInBytes: tamanho em bytes a ser invalidado.
* @retval Nenhum.
* @details Apos o DMA gravar dados vindos de perifericos como ADC ou UART RX na
* memoria SRAM, a CPU pode ainda enxergar dados velhos na cache. Esta funcao
* descarta a cache forçando a leitura direta da memoria física fresca.
*
* Exemplo de uso:
* @code
* // Apos receber dados via DMA:
* BspDma_CacheInvalidate(bufferRx, tamanhoRx);
* ProcessarPacote(bufferRx);
* @endcode
******************************************************************************/
void BspDma_CacheInvalidate(void *address, u32 sizeInBytes);

/******************************************************************************/
/** @brief Executa descarregamento e invalidacao simultanea da D-Cache.
* @param address: ponteiro para o bloco de memoria.
* @param sizeInBytes: tamanho em bytes.
* @retval Nenhum.
* @details Executa Clean e Invalidate em uma unica operacao, ideal para buffers
* compartilhados de entrada e saida.
*
* Exemplo de uso:
* @code
* BspDma_CacheCleanAndInvalidate(bufferCompartilhado, 256);
* @endcode
******************************************************************************/
void BspDma_CacheCleanAndInvalidate(void *address, u32 sizeInBytes);

/******************************************************************************/
/** @brief Verifica se um fluxo/canal de DMA esta ocupado transferindo dados.
* @param hdma: ponteiro para o handle de DMA correspondente.
* @retval true se o canal estiver em transferencia ativa, false se ocioso.
* @details Consulta o estado da maquina de estados interna do handle da HAL.
*
* Exemplo de uso:
* @code
* if(BspDma_IsBusy(huart3.hdmatx) == false)
* {
*     // Canal livre para iniciar novo envio
* }
* @endcode
******************************************************************************/
bool BspDma_IsBusy(const DMA_HandleTypeDef *hdma);

/******************************************************************************/
/** @brief Retorna a quantidade de transferencias restantes no registrador NDTR.
* @param hdma: ponteiro para o handle de DMA.
* @retval Numero de itens/bytes ainda pendentes para transferencia.
* @details Le diretamente o registrador de contagem do hardware (NDTR). Em
* recepcoes circulares, permite calcular a posicao exata do ponteiro de gravacao.
*
* Exemplo de uso:
* @code
* u32 itensRestantes = BspDma_GetRemainingTransfers(huart3.hdmarx);
* u32 itensGravados = tamanhoTotal - itensRestantes;
* @endcode
******************************************************************************/
u32 BspDma_GetRemainingTransfers(const DMA_HandleTypeDef *hdma);

/******************************************************************************/
/** @brief Interrompe com seguranca uma transferencia DMA em andamento.
* @param hdma: ponteiro para o handle de DMA.
* @retval eSTATUS_OK se abortado com sucesso, ou eSTATUS_ERROR se falha.
* @details Invoca HAL_DMA_Abort cancelando a transferencia e desarmando o canal.
*
* Exemplo de uso:
* @code
* BspDma_Abort(huart3.hdmatx);
* @endcode
******************************************************************************/
status_t BspDma_Abort(DMA_HandleTypeDef *hdma);

/******************************************************************************/
/** @brief Realiza copia acelerada de memoria via hardware (Memory-to-Memory).
* @param hdma: ponteiro para um handle de DMA configurado em modo Mem2Mem.
* @param destination: ponteiro para o buffer de destino.
* @param source: ponteiro para o buffer de origem.
* @param length: quantidade de dados a serem copiados.
* @retval eSTATUS_OK se disparado com sucesso, ou codigo de erro.
* @details Copia blocos grandes de memoria sem gastar ciclos da CPU.
*
* Exemplo de uso:
* @code
* BspDma_MemoryCopy(&hdma_memtomem_dma2_stream0, destino, origem, 1024);
* @endcode
******************************************************************************/
status_t BspDma_MemoryCopy(DMA_HandleTypeDef *hdma, void *destination, const void *source, u32 length);

#endif /* _BSP_DMA_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */
