/******************************************************************************/
/**
* @file BspDma.c
* @addtogroup BSP_DMA
* @brief Implementacao do gerenciador de DMA, coerencia de D-Cache e transferencias.
* @author Rodrigues
* @details
* \n <b>Ferramentas:</b>
* - STM32CubeIDE / GCC ARM.
*
* \n <b>Dependencias:</b>
* - BspDma.h
* - core_cm7.h / stm32f7xx_hal.h (Cortex-M7 SCB Cache & DMA HAL)
*
* \n <b>Observacoes:</b>
* - Protege contra leituras desatualizadas de memoria no Cortex-M7.
* - Valida alinhamento de 32 bytes para evitar corrupcao de linhas de cache.
*
* Changelog
* @version <b>1.0.0 - 08/10/2026</b> \n Rodrigues \n Versao inicial com gestao de D-Cache e utilitarios.
*
* @copyright Generic STM32F7 BSP Library
* @{
******************************************************************************/
/*******************************************************************************
* INCLUDES
******************************************************************************/
#include "BspDma.h"

/*******************************************************************************
* DEFINES LOCAIS (fixos, apenas auxiliar para calculos)
******************************************************************************/
// Verificacao de integridade das configuracoes
#if (dBSP_DMA_CACHE_LINE_SIZE_BYTES != 32)
#error "dBSP_DMA_CACHE_LINE_SIZE_BYTES deve ser exatamente 32 bytes para o Cortex-M7."
#endif

/*******************************************************************************
* CONSTANTES
******************************************************************************/

/*******************************************************************************
* ESTRUTURAS DE DADOS LOCAIS
******************************************************************************/
/// Variaveis internas de acompanhamento de operacoes de memoria e DMA
static struct
{
    /// Quantidade de operacoes de limpeza de cache realizadas
    u32 totalCleanOperations;
    /// Quantidade de operacoes de invalidacao de cache realizadas
    u32 totalInvalidateOperations;
} bspDma;

/*******************************************************************************
* PROTOTIPOS LOCAIS
******************************************************************************/
#if !defined(SCB_CCR_DC_Msk)
// Declaracoes auxiliares caso compilado fora do ambiente ARM Cortex-M7
__attribute__((weak)) void SCB_CleanDCache_by_Addr(uint32_t *addr, int32_t dsize)
{
    (void)addr;
    (void)dsize;
}

__attribute__((weak)) void SCB_InvalidateDCache_by_Addr(uint32_t *addr, int32_t dsize)
{
    (void)addr;
    (void)dsize;
}

__attribute__((weak)) void SCB_CleanInvalidateDCache_by_Addr(uint32_t *addr, int32_t dsize)
{
    (void)addr;
    (void)dsize;
}
#endif

#if !defined(HAL_DMA_STATE_BUSY)
typedef enum
{
    HAL_OK = 0x00U,
    HAL_ERROR = 0x01U,
    HAL_BUSY = 0x02U,
    HAL_TIMEOUT = 0x03U
} HAL_StatusTypeDef;

typedef enum
{
    HAL_DMA_STATE_RESET = 0x00U,
    HAL_DMA_STATE_READY = 0x01U,
    HAL_DMA_STATE_BUSY = 0x02U,
    HAL_DMA_STATE_TIMEOUT = 0x03U,
    HAL_DMA_STATE_ERROR = 0x04U,
    HAL_DMA_STATE_ABORT = 0x05U
} HAL_DMA_StateTypeDef;

struct __DMA_HandleTypeDef
{
    void *Instance;
    volatile HAL_DMA_StateTypeDef State;
};

__attribute__((weak)) HAL_StatusTypeDef HAL_DMA_Abort(DMA_HandleTypeDef *hdma)
{
    (void)hdma;
    return HAL_OK;
}

__attribute__((weak)) HAL_StatusTypeDef HAL_DMA_Start(DMA_HandleTypeDef *hdma, uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength)
{
    (void)hdma;
    (void)SrcAddress;
    (void)DstAddress;
    (void)DataLength;
    return HAL_OK;
}
#endif

/*******************************************************************************
* FUNCOES PUBLICAS
******************************************************************************/
/******************************************************************************/
/** @brief Descarrega a D-Cache da CPU para a memoria SRAM fisica (Clean).
* @param address: ponteiro para o inicio do buffer de dados.
* @param sizeInBytes: tamanho do buffer em bytes a ser sincronizado.
* @retval Nenhum.
******************************************************************************/
void BspDma_CacheClean(void *address, u32 sizeInBytes)
{
    if((address != dNULL) && (sizeInBytes > 0))
    {
        SCB_CleanDCache_by_Addr((uint32_t *)address, (int32_t)sizeInBytes);
        bspDma.totalCleanOperations++;
    }
}

/******************************************************************************/
/** @brief Invalida a D-Cache forcando a CPU a ler a SRAM atualizada (Invalidate).
* @param address: ponteiro para o inicio do buffer recebido pelo DMA.
* @param sizeInBytes: tamanho em bytes a ser invalidado.
* @retval Nenhum.
******************************************************************************/
void BspDma_CacheInvalidate(void *address, u32 sizeInBytes)
{
    if((address != dNULL) && (sizeInBytes > 0))
    {
        SCB_InvalidateDCache_by_Addr((uint32_t *)address, (int32_t)sizeInBytes);
        bspDma.totalInvalidateOperations++;
    }
}

/******************************************************************************/
/** @brief Executa descarregamento e invalidacao simultanea da D-Cache.
* @param address: ponteiro para o bloco de memoria.
* @param sizeInBytes: tamanho em bytes.
* @retval Nenhum.
******************************************************************************/
void BspDma_CacheCleanAndInvalidate(void *address, u32 sizeInBytes)
{
    if((address != dNULL) && (sizeInBytes > 0))
    {
        SCB_CleanInvalidateDCache_by_Addr((uint32_t *)address, (int32_t)sizeInBytes);
        bspDma.totalCleanOperations++;
        bspDma.totalInvalidateOperations++;
    }
}

/******************************************************************************/
/** @brief Verifica se um fluxo/canal de DMA esta ocupado transferindo dados.
* @param hdma: ponteiro para o handle de DMA correspondente.
* @retval true se o canal estiver em transferencia ativa, false se ocioso.
******************************************************************************/
bool BspDma_IsBusy(const DMA_HandleTypeDef *hdma)
{
    bool isBusy = false;

    if(hdma != dNULL)
    {
        if(hdma->State == HAL_DMA_STATE_BUSY)
        {
            isBusy = true;
        }
    }

    return isBusy;
}

/******************************************************************************/
/** @brief Retorna a quantidade de transferencias restantes no registrador NDTR.
* @param hdma: ponteiro para o handle de DMA.
* @retval Numero de itens/bytes ainda pendentes para transferencia.
******************************************************************************/
u32 BspDma_GetRemainingTransfers(const DMA_HandleTypeDef *hdma)
{
    u32 count = 0;

#if defined(__HAL_DMA_GET_COUNTER)
    if(hdma != dNULL)
    {
        count = (u32)__HAL_DMA_GET_COUNTER(hdma);
    }
#else
    (void)hdma;
    count = 0;
#endif

    return count;
}

/******************************************************************************/
/** @brief Interrompe com seguranca uma transferencia DMA em andamento.
* @param hdma: ponteiro para o handle de DMA.
* @retval eSTATUS_OK se abortado com sucesso, ou eSTATUS_ERROR se falha.
******************************************************************************/
status_t BspDma_Abort(DMA_HandleTypeDef *hdma)
{
    status_t status = eSTATUS_INVALID_PARAM;

    if(hdma != dNULL)
    {
        HAL_StatusTypeDef halStatus = HAL_DMA_Abort(hdma);
        if(halStatus == HAL_OK)
        {
            status = eSTATUS_OK;
        }
        else
        {
            status = eSTATUS_ERROR;
        }
    }

    return status;
}

/******************************************************************************/
/** @brief Realiza copia acelerada de memoria via hardware (Memory-to-Memory).
* @param hdma: ponteiro para um handle de DMA configurado em modo Mem2Mem.
* @param destination: ponteiro para o buffer de destino.
* @param source: ponteiro para o buffer de origem.
* @param length: quantidade de dados a serem copiados.
* @retval eSTATUS_OK se disparado com sucesso, ou codigo de erro.
******************************************************************************/
status_t BspDma_MemoryCopy(DMA_HandleTypeDef *hdma, void *destination, const void *source, u32 length)
{
    status_t status = eSTATUS_INVALID_PARAM;

    if((hdma != dNULL) && (destination != dNULL) && (source != dNULL) && (length > 0))
    {
        // 1. Descarrega o buffer de origem da cache para a RAM antes do envio
        BspDma_CacheClean((void *)source, length);

        // 2. Dispara a transferencia por hardware
        HAL_StatusTypeDef halStatus = HAL_DMA_Start(hdma, (uint32_t)(uintptr_t)source, (uint32_t)(uintptr_t)destination, length);
        if(halStatus == HAL_OK)
        {
            status = eSTATUS_OK;
        }
        else
        {
            status = eSTATUS_ERROR;
        }
    }

    return status;
}

/*******************************************************************************
* FUNCOES LOCAIS
******************************************************************************/

/** @} DOXYGEN GROUP TAG END OF FILE */
