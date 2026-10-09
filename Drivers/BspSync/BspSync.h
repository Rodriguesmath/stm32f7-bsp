/******************************************************************************/
/**
* @file BspSync.h
* @addtogroup BSP_SYNC
* @{
******************************************************************************/
#ifndef _BSP_SYNC_H_
#define _BSP_SYNC_H_

/*******************************************************************************
* INCLUDES NECESSARIOS
******************************************************************************/
#include "BspTypes.h"

#if defined(__has_include)
#if __has_include("cmsis_os2.h")
#include "cmsis_os2.h"
#endif
#endif

// Declaracoes antecipadas caso compilado fora do ambiente CMSIS-RTOS v2
#ifndef _CMSIS_OS2_H
typedef void *osMutexId_t;
typedef void *osSemaphoreId_t;
typedef void *osThreadId_t;

#ifndef osWaitForever
#define osWaitForever                       0xFFFFFFFFU
#endif
#endif

/*******************************************************************************
* CONFIGURACOES
******************************************************************************/
/// Frequencia padrao do tick do kernel RTOS em Hertz caso nao seja possivel ler
#define dBSP_SYNC_DEFAULT_KERNEL_TICK_FREQ_HZ 1000                              // [Hz]

/*******************************************************************************
* DEFINES PUBLICOS
******************************************************************************/
/// Constante para bloqueio por tempo indeterminado ate obtencao do recurso
#define dBSP_SYNC_WAIT_FOREVER              osWaitForever

/// Constante para tentativa sem bloqueio (retorno imediato)
#define dBSP_SYNC_NO_WAIT                   0U

/*******************************************************************************
* TIPOS DE DADOS PUBLICOS
******************************************************************************/

/*******************************************************************************
* PROTOTIPOS PUBLICOS
******************************************************************************/
/******************************************************************************/
/** @brief Requisita posse exclusiva de um Mutex com timeout em milissegundos.
* @param mutexId: identificador do Mutex criado no CubeMX ou aplicacao.
* @param timeoutMilliseconds: tempo maximo de espera em ms (ou dBSP_SYNC_WAIT_FOREVER).
* @retval eSTATUS_OK se obtido, eSTATUS_TIMEOUT se expirou, ou eSTATUS_INVALID_PARAM.
* @details Converte o tempo informado em milissegundos reais para ticks do kernel
* e invoca osMutexAcquire. Garante exclusao mutua com heranca de prioridade.
*
* Exemplo de uso:
* @code
* if(BspSync_MutexAcquire(displayMutex, 100) == eSTATUS_OK)
* {
*     // Regiao protegida: escreve no display
*     BspSync_MutexRelease(displayMutex);
* }
* @endcode
******************************************************************************/
status_t BspSync_MutexAcquire(osMutexId_t mutexId, u32 timeoutMilliseconds);

/******************************************************************************/
/** @brief Libera a posse de um Mutex previamente adquirido.
* @param mutexId: identificador do Mutex a ser liberado.
* @retval eSTATUS_OK se liberado com sucesso, ou codigo de erro.
* @details Invoca osMutexRelease devolvendo o recurso para outras threads
* e restaurando a prioridade original da tarefa se houve heranca de prioridade.
*
* Exemplo de uso:
* @code
* BspSync_MutexRelease(displayMutex);
* @endcode
******************************************************************************/
status_t BspSync_MutexRelease(osMutexId_t mutexId);

/******************************************************************************/
/** @brief Retorna o identificador da thread que atualmente possui o Mutex.
* @param mutexId: identificador do Mutex a ser consultado.
* @retval osThreadId_t da thread proprietaria, ou dNULL se livre.
* @details Consulta a propriedade atual do Mutex sem alterar seu estado.
*
* Exemplo de uso:
* @code
* osThreadId_t dono = BspSync_MutexGetOwner(displayMutex);
* if(dono != dNULL)
* {
*     // Mutex esta ocupado no momento
* }
* @endcode
******************************************************************************/
osThreadId_t BspSync_MutexGetOwner(osMutexId_t mutexId);

/******************************************************************************/
/** @brief Aguarda e consome um token do Semaforo com timeout em milissegundos.
* @param semaphoreId: identificador do Semaforo (binario ou contador).
* @param timeoutMilliseconds: tempo maximo de espera em ms (ou dBSP_SYNC_WAIT_FOREVER).
* @retval eSTATUS_OK se adquirido, eSTATUS_TIMEOUT se expirou tempo.
* @details Suspende a thread corrente ate que outra thread ou interrupcao
* incremente o semaforo via BspSync_SemaphoreRelease.
*
* Exemplo de uso:
* @code
* // Aguarda ate 500 ms pelo evento de recepcao
* if(BspSync_SemaphoreAcquire(dadosProntosSemaphore, 500) == eSTATUS_OK)
* {
*     // Processa pacote recebido
* }
* @endcode
******************************************************************************/
status_t BspSync_SemaphoreAcquire(osSemaphoreId_t semaphoreId, u32 timeoutMilliseconds);

/******************************************************************************/
/** @brief Libera e incrementa a contagem de um Semaforo.
* @param semaphoreId: identificador do Semaforo a ser sinalizado.
* @retval eSTATUS_OK se sinalizado com sucesso, ou codigo de erro.
* @details Incrementa a contagem interna do semaforo e acorda a thread de maior
* prioridade que estiver aguardando. No padrao CMSIS-RTOS v2, esta chamada e
* segura tanto para o contexto de Threads quanto para Interrupcoes (ISR).
*
* Exemplo de uso:
* @code
* // Pode ser chamado tanto de uma Tarefa quanto de dentro de um Callback ISR
* BspSync_SemaphoreRelease(dadosProntosSemaphore);
* @endcode
******************************************************************************/
status_t BspSync_SemaphoreRelease(osSemaphoreId_t semaphoreId);

/******************************************************************************/
/** @brief Retorna a contagem atual de tokens disponiveis no Semaforo.
* @param semaphoreId: identificador do Semaforo.
* @retval Quantidade de tokens disponiveis para requisicao imediata.
* @details Consulta a contagem interna sem decrementar ou alterar o recurso.
*
* Exemplo de uso:
* @code
* u32 tokens = BspSync_SemaphoreGetCount(dadosProntosSemaphore);
* @endcode
******************************************************************************/
u32 BspSync_SemaphoreGetCount(osSemaphoreId_t semaphoreId);

/******************************************************************************/
/** @brief Bloqueia a thread atual por um periodo especificado em milissegundos.
* @param milliseconds: tempo de suspensao em milissegundos reais.
* @retval Nenhum.
* @details Converte o tempo em milissegundos para ticks e invoca osDelay.
* Ao contrario de HAL_Delay, esta funcao cede imediatamente a CPU para que
* outras tarefas prontas possam executar.
*
* Exemplo de uso:
* @code
* while(1)
* {
*     // Executa ciclo
*     BspSync_ThreadDelay(100); // Dorme por 100ms cedendo CPU
* }
* @endcode
******************************************************************************/
void BspSync_ThreadDelay(u32 milliseconds);

/******************************************************************************/
/** @brief Bloqueia a thread atual ate um ponto temporal futuro garantindo periodo exato.
* @param previousWakeTime: ponteiro contendo o tick da ultima ativacao em milissegundos.
* @param periodMilliseconds: periodo desejado do ciclo em milissegundos.
* @retval Nenhum.
* @details Utiliza osDelayUntil internamente. Evita o acumulo de atrasos (drift)
* em lacos de controle de tempo real periodicos.
*
* Exemplo de uso:
* @code
* u32 tempoAnterior = 0;
* while(1)
* {
*     // Executa exatamente a cada 20 ms (50 Hz) sem variacao
*     BspSync_ThreadDelayUntil(&tempoAnterior, 20);
* }
* @endcode
******************************************************************************/
void BspSync_ThreadDelayUntil(u32 *previousWakeTime, u32 periodMilliseconds);

/******************************************************************************/
/** @brief Cede a execucao da thread atual para outra thread de mesma prioridade.
* @param Nenhum.
* @retval Nenhum.
* @details Invoca osThreadYield forçando uma troca de contexto imediata para
* tarefas prontas sem suspender a tarefa por tempo.
*
* Exemplo de uso:
* @code
* BspSync_ThreadYield();
* @endcode
******************************************************************************/
void BspSync_ThreadYield(void);

/******************************************************************************/
/** @brief Trava o escalonador do kernel RTOS impedindo trocas de contexto.
* @param Nenhum.
* @retval 1 se o kernel estava desbloqueado, 0 se ja estava travado, ou valor negativo se erro.
* @details Inicia uma secao critica a nivel de threads (interrupcoes de hardware
* continuam ativas, mas nenhuma outra tarefa podera tomar a CPU).
*
* Exemplo de uso:
* @code
* BspSync_KernelLock();
* // Atualizacao rapida e indivisivel de ponteiros ou estados
* BspSync_KernelUnlock();
* @endcode
******************************************************************************/
s32 BspSync_KernelLock(void);

/******************************************************************************/
/** @brief Restaura o estado anterior do escalonador do kernel RTOS.
* @param Nenhum.
* @retval 1 se o kernel voltou a ficar destravado, 0 se ainda travado, ou erro negativo.
* @details Reabilita o escalonamento permitindo novamente a alternancia entre threads.
*
* Exemplo de uso:
* @code
* BspSync_KernelUnlock();
* @endcode
******************************************************************************/
s32 BspSync_KernelUnlock(void);

#endif /* _BSP_SYNC_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */
