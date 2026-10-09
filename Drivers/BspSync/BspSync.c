/******************************************************************************/
/**
* @file BspSync.c
* @addtogroup BSP_SYNC
* @brief Implementacao do driver de sincronizacao e controle compativel com CMSIS-RTOS v2.
* @author Rodrigues
* @details
* \n <b>Ferramentas:</b>
* - STM32CubeIDE / GCC ARM.
*
* \n <b>Dependencias:</b>
* - BspSync.h
* - cmsis_os2.h (CMSIS-RTOS v2 Kernel)
*
* \n <b>Observacoes:</b>
* - Segue estritamente a nomenclatura padrao da ARM e CMSIS-RTOS v2.
* - Garante conversao automatica de milissegundos para ticks do escalonador.
*
* Changelog
* @version <b>1.0.0 - 08/10/2026</b> \n Rodrigues \n Versao inicial fiel ao CMSIS v2.
*
* @copyright Generic STM32F7 BSP Library
* @{
******************************************************************************/
/*******************************************************************************
* INCLUDES
******************************************************************************/
#include "BspSync.h"

/*******************************************************************************
* DEFINES LOCAIS (fixos, apenas auxiliar para calculos)
******************************************************************************/
// Verificacao de integridade das configuracoes
#if (dBSP_SYNC_DEFAULT_KERNEL_TICK_FREQ_HZ == 0)
#error "dBSP_SYNC_DEFAULT_KERNEL_TICK_FREQ_HZ deve ser maior do que zero."
#endif

/// Quantidade de milissegundos em um segundo
#define dMILLISECONDS_PER_SECOND            1000U

/*******************************************************************************
* CONSTANTES
******************************************************************************/

/*******************************************************************************
* ESTRUTURAS DE DADOS LOCAIS
******************************************************************************/
/// Estrutura de rastreamento estatistico das operacoes de sincronismo
static struct
{
    /// Total de aquisicoes de Mutex realizadas
    u32 totalMutexAcquisitions;
    /// Total de liberacoes de Semaforo realizadas
    u32 totalSemaphoreReleases;
} bspSync;

/*******************************************************************************
* PROTOTIPOS LOCAIS
******************************************************************************/
#if !defined(_CMSIS_OS2_H)
// Declaracoes com atributo weak para permitir compilacao fora do ecossistema RTOS
typedef enum
{
    osOK = 0,
    osError = -1,
    osErrorTimeout = -2,
    osErrorResource = -3,
    osErrorParameter = -4
} osStatus_t;

__attribute__((weak)) osStatus_t osMutexAcquire(osMutexId_t mutex_id, uint32_t timeout)
{
    (void)mutex_id;
    (void)timeout;
    return osOK;
}

__attribute__((weak)) osStatus_t osMutexRelease(osMutexId_t mutex_id)
{
    (void)mutex_id;
    return osOK;
}

__attribute__((weak)) osThreadId_t osMutexGetOwner(osMutexId_t mutex_id)
{
    (void)mutex_id;
    return dNULL;
}

__attribute__((weak)) osStatus_t osSemaphoreAcquire(osSemaphoreId_t semaphore_id, uint32_t timeout)
{
    (void)semaphore_id;
    (void)timeout;
    return osOK;
}

__attribute__((weak)) osStatus_t osSemaphoreRelease(osSemaphoreId_t semaphore_id)
{
    (void)semaphore_id;
    return osOK;
}

__attribute__((weak)) uint32_t osSemaphoreGetCount(osSemaphoreId_t semaphore_id)
{
    (void)semaphore_id;
    return 1U;
}

__attribute__((weak)) osStatus_t osDelay(uint32_t ticks)
{
    (void)ticks;
    return osOK;
}

__attribute__((weak)) osStatus_t osDelayUntil(uint32_t ticks)
{
    (void)ticks;
    return osOK;
}

__attribute__((weak)) osStatus_t osThreadYield(void)
{
    return osOK;
}

__attribute__((weak)) int32_t osKernelLock(void)
{
    return 1;
}

__attribute__((weak)) int32_t osKernelUnlock(void)
{
    return 0;
}

__attribute__((weak)) uint32_t osKernelGetTickFreq(void)
{
    return dBSP_SYNC_DEFAULT_KERNEL_TICK_FREQ_HZ;
}

__attribute__((weak)) uint32_t osKernelGetTickCount(void)
{
    return 0;
}
#endif

static u32 BspSync_MillisecondsToTicks(u32 milliseconds);
static status_t BspSync_MapOsStatus(int32_t osStatus);

/*******************************************************************************
* FUNCOES PUBLICAS
******************************************************************************/
/******************************************************************************/
/** @brief Requisita posse exclusiva de um Mutex com timeout em milissegundos.
* @param mutexId: identificador do Mutex criado no CubeMX ou aplicacao.
* @param timeoutMilliseconds: tempo maximo de espera em ms (ou dBSP_SYNC_WAIT_FOREVER).
* @retval eSTATUS_OK se obtido, eSTATUS_TIMEOUT se expirou, ou eSTATUS_INVALID_PARAM.
******************************************************************************/
status_t BspSync_MutexAcquire(osMutexId_t mutexId, u32 timeoutMilliseconds)
{
    status_t status = eSTATUS_INVALID_PARAM;

    if(mutexId != dNULL)
    {
        u32 ticks = BspSync_MillisecondsToTicks(timeoutMilliseconds);
        int32_t osStatus = (int32_t)osMutexAcquire(mutexId, ticks);
        status = BspSync_MapOsStatus(osStatus);

        if(status == eSTATUS_OK)
        {
            bspSync.totalMutexAcquisitions++;
        }
    }

    return status;
}

/******************************************************************************/
/** @brief Libera a posse de um Mutex previamente adquirido.
* @param mutexId: identificador do Mutex a ser liberado.
* @retval eSTATUS_OK se liberado com sucesso, ou codigo de erro.
******************************************************************************/
status_t BspSync_MutexRelease(osMutexId_t mutexId)
{
    status_t status = eSTATUS_INVALID_PARAM;

    if(mutexId != dNULL)
    {
        int32_t osStatus = (int32_t)osMutexRelease(mutexId);
        status = BspSync_MapOsStatus(osStatus);
    }

    return status;
}

/******************************************************************************/
/** @brief Retorna o identificador da thread que atualmente possui o Mutex.
* @param mutexId: identificador do Mutex a ser consultado.
* @retval osThreadId_t da thread proprietaria, ou dNULL se livre.
******************************************************************************/
osThreadId_t BspSync_MutexGetOwner(osMutexId_t mutexId)
{
    osThreadId_t owner = dNULL;

    if(mutexId != dNULL)
    {
        owner = osMutexGetOwner(mutexId);
    }

    return owner;
}

/******************************************************************************/
/** @brief Aguarda e consome um token do Semaforo com timeout em milissegundos.
* @param semaphoreId: identificador do Semaforo (binario ou contador).
* @param timeoutMilliseconds: tempo maximo de espera em ms (ou dBSP_SYNC_WAIT_FOREVER).
* @retval eSTATUS_OK se adquirido, eSTATUS_TIMEOUT se expirou tempo.
******************************************************************************/
status_t BspSync_SemaphoreAcquire(osSemaphoreId_t semaphoreId, u32 timeoutMilliseconds)
{
    status_t status = eSTATUS_INVALID_PARAM;

    if(semaphoreId != dNULL)
    {
        u32 ticks = BspSync_MillisecondsToTicks(timeoutMilliseconds);
        int32_t osStatus = (int32_t)osSemaphoreAcquire(semaphoreId, ticks);
        status = BspSync_MapOsStatus(osStatus);
    }

    return status;
}

/******************************************************************************/
/** @brief Libera e incrementa a contagem de um Semaforo.
* @param semaphoreId: identificador do Semaforo a ser sinalizado.
* @retval eSTATUS_OK se sinalizado com sucesso, ou codigo de erro.
******************************************************************************/
status_t BspSync_SemaphoreRelease(osSemaphoreId_t semaphoreId)
{
    status_t status = eSTATUS_INVALID_PARAM;

    if(semaphoreId != dNULL)
    {
        int32_t osStatus = (int32_t)osSemaphoreRelease(semaphoreId);
        status = BspSync_MapOsStatus(osStatus);

        if(status == eSTATUS_OK)
        {
            bspSync.totalSemaphoreReleases++;
        }
    }

    return status;
}

/******************************************************************************/
/** @brief Retorna a contagem atual de tokens disponiveis no Semaforo.
* @param semaphoreId: identificador do Semaforo.
* @retval Quantidade de tokens disponiveis para requisicao imediata.
******************************************************************************/
u32 BspSync_SemaphoreGetCount(osSemaphoreId_t semaphoreId)
{
    u32 count = 0;

    if(semaphoreId != dNULL)
    {
        count = (u32)osSemaphoreGetCount(semaphoreId);
    }

    return count;
}

/******************************************************************************/
/** @brief Bloqueia a thread atual por um periodo especificado em milissegundos.
* @param milliseconds: tempo de suspensao em milissegundos reais.
* @retval Nenhum.
******************************************************************************/
void BspSync_ThreadDelay(u32 milliseconds)
{
    u32 ticks = BspSync_MillisecondsToTicks(milliseconds);
    (void)osDelay(ticks);
}

/******************************************************************************/
/** @brief Bloqueia a thread atual ate um ponto temporal futuro garantindo periodo exato.
* @param previousWakeTime: ponteiro contendo o tick da ultima ativacao em milissegundos.
* @param periodMilliseconds: periodo desejado do ciclo em milissegundos.
* @retval Nenhum.
******************************************************************************/
void BspSync_ThreadDelayUntil(u32 *previousWakeTime, u32 periodMilliseconds)
{
    if(previousWakeTime != dNULL)
    {
        u32 periodTicks = BspSync_MillisecondsToTicks(periodMilliseconds);
        u32 targetTicks = *previousWakeTime + periodTicks;
        (void)osDelayUntil(targetTicks);
        *previousWakeTime = targetTicks;
    }
}

/******************************************************************************/
/** @brief Cede a execucao da thread atual para outra thread de mesma prioridade.
* @param Nenhum.
* @retval Nenhum.
******************************************************************************/
void BspSync_ThreadYield(void)
{
    (void)osThreadYield();
}

/******************************************************************************/
/** @brief Trava o escalonador do kernel RTOS impedindo trocas de contexto.
* @param Nenhum.
* @retval 1 se o kernel estava desbloqueado, 0 se ja estava travado, ou valor negativo se erro.
******************************************************************************/
s32 BspSync_KernelLock(void)
{
    return (s32)osKernelLock();
}

/******************************************************************************/
/** @brief Restaura o estado anterior do escalonador do kernel RTOS.
* @param Nenhum.
* @retval 1 se o kernel voltou a ficar destravado, 0 se ainda travado, ou erro negativo.
******************************************************************************/
s32 BspSync_KernelUnlock(void)
{
    return (s32)osKernelUnlock();
}

/*******************************************************************************
* FUNCOES LOCAIS
******************************************************************************/
/******************************************************************************/
/** @brief Converte milissegundos reais para a quantidade equivalente de ticks do kernel.
* @param milliseconds: tempo em milissegundos (ou dBSP_SYNC_WAIT_FOREVER).
* @retval Quantidade de ticks calculada.
******************************************************************************/
static u32 BspSync_MillisecondsToTicks(u32 milliseconds)
{
    u32 ticks = 0;

    if(milliseconds == dBSP_SYNC_WAIT_FOREVER)
    {
        ticks = osWaitForever;
    }
    else if(milliseconds == 0)
    {
        ticks = 0;
    }
    else
    {
        u32 tickFreq = (u32)osKernelGetTickFreq();
        if(tickFreq == 0)
        {
            tickFreq = dBSP_SYNC_DEFAULT_KERNEL_TICK_FREQ_HZ;
        }

        // Calcula com arredondamento para cima para garantir tempo minimo
        ticks = ((milliseconds * tickFreq) + (dMILLISECONDS_PER_SECOND - 1U)) / dMILLISECONDS_PER_SECOND;
        if(ticks == 0)
        {
            ticks = 1;
        }
    }

    return ticks;
}

/******************************************************************************/
/** @brief Mapeia o codigo de retorno do CMSIS-RTOS para o enum status_t da BSP.
* @param osStatus: codigo de status do CMSIS-RTOS.
* @retval status_t mapeado equivalente.
******************************************************************************/
static status_t BspSync_MapOsStatus(int32_t osStatus)
{
    status_t status = eSTATUS_ERROR;

    switch(osStatus)
    {
        case 0: // osOK
            status = eSTATUS_OK;
            break;

        case -2: // osErrorTimeout
            status = eSTATUS_TIMEOUT;
            break;

        case -4: // osErrorParameter
            status = eSTATUS_INVALID_PARAM;
            break;

        case -3: // osErrorResource
            status = eSTATUS_BUSY;
            break;

        case -1: // osError
        default:
            status = eSTATUS_ERROR;
            break;
    }

    return status;
}

/** @} DOXYGEN GROUP TAG END OF FILE */
