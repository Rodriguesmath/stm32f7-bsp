/******************************************************************************/
/**
* @file Bsp.c
* @addtogroup BSP
* @brief Implementacao do ponto central de integracao do hardware da Nucleo-F767ZI.
* @author Rodrigues
* @details
* \n <b>Ferramentas:</b>
* - STM32CubeIDE / GCC ARM.
*
* \n <b>Dependencias:</b>
* - Bsp.h
* - BspUart.h
* - BspAdc.h
* - BspDac.h
*
* \n <b>Observacoes:</b>
* - Centraliza a inicializacao de baixo nivel e expoe os modulos prontos.
* - Cumpre a secao 4.12 da norma de desenvolvimento de firmware Assert.
*
* Changelog
* @version <b>1.0.0 - 08/10/2026</b> \n Rodrigues \n Versao inicial da camada unificada BSP.
*
* @copyright Generic STM32F7 BSP Library
* @{
******************************************************************************/
/*******************************************************************************
* INCLUDES
******************************************************************************/
#include "Bsp.h"

#include <string.h>

/*******************************************************************************
* DEFINES LOCAIS (fixos, apenas auxiliar para calculos)
******************************************************************************/
// Verificacao de consistencia dos defines
#if (dBSP_SYSTEM_VREF_MV <= 0)
#error "dBSP_SYSTEM_VREF_MV deve ser maior do que zero."
#endif

/*******************************************************************************
* CONSTANTES
******************************************************************************/

/*******************************************************************************
* ESTRUTURAS DE DADOS LOCAIS
******************************************************************************/
/// Estrutura estatica centralizando todas as instancias e estados locais do BSP
static struct
{
    /// Instancia principal do driver UART de comunicacao
    bspUart_t consoleUart;
    /// Instancia principal do driver de conversao analogica ADC
    bspAdc_t  defaultAdc;
    /// Instancia principal do driver de geracao analogica DAC
    bspDac_t  defaultDac;
    /// Byte temporario para recepcao da interrupcao da UART
    u8 rxInterruptByte;
    /// Flag indicando se o hardware foi inicializado
    bool isHardwareReady;
} bsp;

/*******************************************************************************
* PROTOTIPOS LOCAIS
******************************************************************************/
// Declaracoes com atributo weak para permitir compilacao com ou sem CubeMX
#if !defined(HAL_OK)
__attribute__((weak)) void HAL_Delay(uint32_t Delay)
{
    (void)Delay;
}

__attribute__((weak)) uint32_t HAL_GetTick(void)
{
    return 0;
}
#endif

// Handles gerados pelo STM32CubeMX
extern UART_HandleTypeDef huart3 __attribute__((weak));
extern ADC_HandleTypeDef  hadc1  __attribute__((weak));
extern DAC_HandleTypeDef  hdac   __attribute__((weak));

// Funcoes de inicializacao geradas pelo CubeMX
extern void MX_GPIO_Init(void) __attribute__((weak));
extern void MX_USART3_UART_Init(void) __attribute__((weak));
extern void MX_ADC1_Init(void) __attribute__((weak));
extern void MX_DAC_Init(void) __attribute__((weak));

static void Bsp_StartRxInterrupt(void);

/*******************************************************************************
* FUNCOES PUBLICAS
******************************************************************************/
/******************************************************************************/
/** @brief Ponto unico de inicializacao de todo o hardware do microcontrolador.
* @param Nenhum.
* @retval eSTATUS_OK se sucesso, outro codigo caso erro.
******************************************************************************/
status_t Bsp_Init(void)
{
    status_t status = eSTATUS_OK;

    // Inicializacoes dos perifericos geradas pelo CubeMX (se presentes)
    if(MX_GPIO_Init != dNULL)
    {
        MX_GPIO_Init();
    }

    if(MX_USART3_UART_Init != dNULL)
    {
        MX_USART3_UART_Init();
    }

    if(MX_ADC1_Init != dNULL)
    {
        MX_ADC1_Init();
    }

    if(MX_DAC_Init != dNULL)
    {
        MX_DAC_Init();
    }

    // Inicializacao dos drivers desacoplados com protecao de presenca de handle
    if(&huart3 != dNULL)
    {
        status = BspUart_Init(&bsp.consoleUart, &huart3);
        if(status == eSTATUS_OK)
        {
            Bsp_StartRxInterrupt();
        }
    }

    if(&hadc1 != dNULL)
    {
        (void)BspAdc_Init(&bsp.defaultAdc, &hadc1, dBSP_SYSTEM_VREF);
    }

    if(&hdac != dNULL)
    {
        (void)BspDac_Init(&bsp.defaultDac, &hdac, DAC_CHANNEL_1, dBSP_SYSTEM_VREF);
    }

    bsp.isHardwareReady = true;

    return status;
}

/******************************************************************************/
/** @brief Retorna o ponteiro para a instancia da UART de console da placa.
* @param Nenhum.
* @retval Ponteiro para a estrutura bspUart_t do console serial.
******************************************************************************/
bspUart_t *Bsp_GetConsoleUart(void)
{
    return &bsp.consoleUart;
}

/******************************************************************************/
/** @brief Retorna a instancia padrao do conversor analogico (ADC1).
* @param Nenhum.
* @retval Ponteiro para a estrutura bspAdc_t inicializada.
******************************************************************************/
bspAdc_t *Bsp_GetDefaultAdc(void)
{
    return &bsp.defaultAdc;
}

/******************************************************************************/
/** @brief Retorna a instancia padrao da saida analogica (DAC1 canal 1).
* @param Nenhum.
* @retval Ponteiro para a estrutura bspDac_t inicializada.
******************************************************************************/
bspDac_t *Bsp_GetDefaultDac(void)
{
    return &bsp.defaultDac;
}

/******************************************************************************/
/** @brief Executa atraso bloqueante em milissegundos sem acoplamento direto a HAL.
* @param ms: tempo de atraso em milissegundos.
* @retval Nenhum.
******************************************************************************/
void Bsp_DelayMs(u32 ms)
{
    HAL_Delay((uint32_t)ms);
}

/******************************************************************************/
/** @brief Retorna o contador de tempo atual do sistema em milissegundos.
* @param Nenhum.
* @retval Valor atual da base de tempo em milissegundos.
******************************************************************************/
u32 Bsp_GetTick(void)
{
    return (u32)HAL_GetTick();
}

/******************************************************************************/
/** @brief Callback da HAL interceptando recepcao serial completa.
* @param huart: ponteiro para o handle da UART que disparou o evento.
* @retval Nenhum.
* @details Encaminha os dados recebidos para o buffer circular da biblioteca BspUart.
******************************************************************************/
__attribute__((weak)) void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if((huart != dNULL) && (huart == bsp.consoleUart.huart))
    {
        BspUart_RxInterruptHandler(&bsp.consoleUart, bsp.rxInterruptByte);
        Bsp_StartRxInterrupt();
    }
}

/*******************************************************************************
* FUNCOES LOCAIS
******************************************************************************/
/******************************************************************************/
/** @brief Reativa a recepcao por interrupcao de 1 byte na HAL.
* @param Nenhum.
* @retval Nenhum.
******************************************************************************/
static void Bsp_StartRxInterrupt(void)
{
#if defined(HAL_UART_MODULE_ENABLED)
    if(bsp.consoleUart.huart != dNULL)
    {
        (void)HAL_UART_Receive_IT(bsp.consoleUart.huart, &bsp.rxInterruptByte, 1);
    }
#endif
}

/** @} DOXYGEN GROUP TAG END OF FILE */
