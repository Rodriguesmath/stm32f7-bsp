/******************************************************************************/
/**
* @file BspDac.c
* @addtogroup BSP_DAC
* @brief Implementacao do driver de conversao digital-analogica de alto nivel.
* @author Rodrigues
* @details
* \n <b>Ferramentas:</b>
* - STM32CubeIDE / GCC ARM.
*
* \n <b>Dependencias:</b>
* - BspDac.h
* - stm32f7xx_hal.h (HAL DAC Module)
*
* \n <b>Observacoes:</b>
* - Compativel com os canais DAC1 e DAC2 do microcontrolador STM32F767ZI.
*
* Changelog
* @version <b>1.0.0 - 08/10/2026</b> \n Rodrigues \n Versao inicial com suporte a Volts, mV e normalizado.
*
* @copyright Generic STM32F7 BSP Library
* @{
******************************************************************************/
/*******************************************************************************
* INCLUDES
******************************************************************************/
#include "BspDac.h"

/*******************************************************************************
* DEFINES LOCAIS (fixos, apenas auxiliar para calculos)
******************************************************************************/
// Verificacao de integridade das constantes de configuracao
#if (dBSP_DAC_MAX_COUNTS != 4095)
#error "dBSP_DAC_MAX_COUNTS deve ser 4095 para resolucao de 12 bits."
#endif

/// Fator de escala para conversao de milivolts
#define dMILLIVOLTS_FACTOR                  1000.0f

/*******************************************************************************
* CONSTANTES
******************************************************************************/

/*******************************************************************************
* ESTRUTURAS DE DADOS LOCAIS
******************************************************************************/
/// Variaveis internas de rastreamento do DAC
static struct
{
    /// Quantidade total de atualizacoes de valor no conversor
    u32 totalUpdates;
} bspDac;

/*******************************************************************************
* PROTOTIPOS LOCAIS
******************************************************************************/
#if !defined(HAL_OK)
// Declaracoes auxiliares caso compilado fora do ambiente de firmware STM32
typedef enum
{
    HAL_OK = 0x00U,
    HAL_ERROR = 0x01U,
    HAL_BUSY = 0x02U,
    HAL_TIMEOUT = 0x03U
} HAL_StatusTypeDef;

__attribute__((weak)) HAL_StatusTypeDef HAL_DAC_Start(DAC_HandleTypeDef *hdac, uint32_t Channel)
{
    (void)hdac;
    (void)Channel;
    return HAL_OK;
}

__attribute__((weak)) HAL_StatusTypeDef HAL_DAC_SetValue(DAC_HandleTypeDef *hdac, uint32_t Channel, uint32_t Alignment, uint32_t Data)
{
    (void)hdac;
    (void)Channel;
    (void)Alignment;
    (void)Data;
    return HAL_OK;
}

__attribute__((weak)) HAL_StatusTypeDef HAL_DAC_Stop(DAC_HandleTypeDef *hdac, uint32_t Channel)
{
    (void)hdac;
    (void)Channel;
    return HAL_OK;
}
#endif

static status_t BspDac_MapHalStatus(HAL_StatusTypeDef halStatus);

/*******************************************************************************
* FUNCOES PUBLICAS
******************************************************************************/
/******************************************************************************/
/** @brief Inicializa a saida analogica DAC e associa ao handle do CubeMX.
* @param dev: ponteiro para a estrutura de dados do DAC.
* @param hdac: ponteiro para o handle HAL do DAC gerado pelo CubeMX.
* @param channel: canal de saida (DAC_CHANNEL_1 ou DAC_CHANNEL_2).
* @param vRef: tensao de referencia analogica em Volts.
* @retval eSTATUS_OK se sucesso, ou eSTATUS_INVALID_PARAM se ponteiro nulo.
******************************************************************************/
status_t BspDac_Init(bspDac_t *dev, DAC_HandleTypeDef *hdac, u32 channel, f32 vRef)
{
    status_t ret = eSTATUS_OK;

    if((dev == dNULL) || (hdac == dNULL) || (vRef <= 0.0f))
    {
        ret = eSTATUS_INVALID_PARAM;
    }
    else
    {
        dev->hdac = hdac;
        dev->channel = channel;
        dev->vRef = vRef;
        dev->lastRawValue = 0;
        dev->isRunning = false;

        HAL_StatusTypeDef halStatus = HAL_DAC_Start(dev->hdac, dev->channel);
        ret = BspDac_MapHalStatus(halStatus);

        if(ret == eSTATUS_OK)
        {
            dev->isRunning = true;
            ret = BspDac_SetRaw(dev, 0);
        }
    }

    return ret;
}

/******************************************************************************/
/** @brief Define o valor bruto de contagem do registrador do DAC (0 a 4095).
* @param dev: ponteiro para a estrutura do DAC.
* @param rawValue: valor digital de 12 bits a ser convertido para tensao.
* @retval eSTATUS_OK se atualizado com sucesso, ou codigo de erro da HAL.
******************************************************************************/
status_t BspDac_SetRaw(bspDac_t *dev, u16 rawValue)
{
    status_t ret = eSTATUS_OK;

    if((dev == dNULL) || (dev->isRunning == false))
    {
        ret = eSTATUS_INVALID_PARAM;
    }
    else
    {
        u32 clampedValue = (u32)rawValue;
        if(clampedValue > (u32)dBSP_DAC_MAX_COUNTS)
        {
            clampedValue = (u32)dBSP_DAC_MAX_COUNTS;
        }

        HAL_StatusTypeDef halStatus = HAL_DAC_SetValue(dev->hdac, dev->channel, DAC_ALIGN_12B_R, clampedValue);
        ret = BspDac_MapHalStatus(halStatus);

        if(ret == eSTATUS_OK)
        {
            dev->lastRawValue = (u16)clampedValue;
            bspDac.totalUpdates++;
        }
    }

    return ret;
}

/******************************************************************************/
/** @brief Ajusta a tensao de saida analogica diretamente em Volts.
* @param dev: ponteiro para a estrutura do DAC.
* @param voltage: tensao desejada no pino em Volts.
* @retval eSTATUS_OK se aplicado com sucesso, ou erro se parametros invalidos.
******************************************************************************/
status_t BspDac_SetVoltage(bspDac_t *dev, f32 voltage)
{
    status_t ret = eSTATUS_OK;

    if(dev == dNULL)
    {
        ret = eSTATUS_INVALID_PARAM;
    }
    else
    {
        // Limita a tensao na faixa segura permitida pelo hardware
        f32 safeVoltage = voltage;
        if(safeVoltage < 0.0f)
        {
            safeVoltage = 0.0f;
        }
        else if(safeVoltage > dev->vRef)
        {
            safeVoltage = dev->vRef;
        }

        f32 calculatedCounts = (safeVoltage * (f32)dBSP_DAC_MAX_COUNTS) / dev->vRef;
        u16 rawCounts = (u16)(calculatedCounts + 0.5f);

        ret = BspDac_SetRaw(dev, rawCounts);
    }

    return ret;
}

/******************************************************************************/
/** @brief Define a saida em milivolts usando aritmetica inteira (0 a 3300 mV).
* @param dev: ponteiro para a estrutura do DAC.
* @param milliVolts: tensao desejada em milivolts.
* @retval eSTATUS_OK se sucesso, ou erro caso falha.
******************************************************************************/
status_t BspDac_SetMilliVolts(bspDac_t *dev, u16 milliVolts)
{
    status_t ret = eSTATUS_OK;

    if(dev == dNULL)
    {
        ret = eSTATUS_INVALID_PARAM;
    }
    else
    {
        f32 voltageInVolts = (f32)milliVolts / dMILLIVOLTS_FACTOR;
        ret = BspDac_SetVoltage(dev, voltageInVolts);
    }

    return ret;
}

/******************************************************************************/
/** @brief Ajusta o nivel de saida por valor normalizado entre 0.0 e 1.0.
* @param dev: ponteiro para a estrutura do DAC.
* @param ratio: valor decimal entre 0.0f (0%) e 1.0f (100% da escala).
* @retval eSTATUS_OK se sucesso, ou codigo de erro.
******************************************************************************/
status_t BspDac_SetNormalized(bspDac_t *dev, f32 ratio)
{
    status_t ret = eSTATUS_OK;

    if(dev == dNULL)
    {
        ret = eSTATUS_INVALID_PARAM;
    }
    else
    {
        f32 safeRatio = ratio;
        if(safeRatio < 0.0f)
        {
            safeRatio = 0.0f;
        }
        else if(safeRatio > 1.0f)
        {
            safeRatio = 1.0f;
        }

        f32 calculatedCounts = safeRatio * (f32)dBSP_DAC_MAX_COUNTS;
        u16 rawCounts = (u16)(calculatedCounts + 0.5f);

        ret = BspDac_SetRaw(dev, rawCounts);
    }

    return ret;
}

/******************************************************************************/
/** @brief Desliga a saida analogica do DAC para economia de energia.
* @param dev: ponteiro para a estrutura do DAC.
* @retval eSTATUS_OK se parado com sucesso, ou erro se falha na HAL.
******************************************************************************/
status_t BspDac_Stop(bspDac_t *dev)
{
    status_t ret = eSTATUS_OK;

    if((dev == dNULL) || (dev->isRunning == false))
    {
        ret = eSTATUS_INVALID_PARAM;
    }
    else
    {
        HAL_StatusTypeDef halStatus = HAL_DAC_Stop(dev->hdac, dev->channel);
        ret = BspDac_MapHalStatus(halStatus);

        if(ret == eSTATUS_OK)
        {
            dev->isRunning = false;
        }
    }

    return ret;
}

/*******************************************************************************
* FUNCOES LOCAIS
******************************************************************************/
/******************************************************************************/
/** @brief Mapeia o status de retorno da HAL para a tipagem status_t do BSP.
* @param halStatus: codigo de status retornado pela biblioteca HAL.
* @retval status_t mapeado equivalente.
******************************************************************************/
static status_t BspDac_MapHalStatus(HAL_StatusTypeDef halStatus)
{
    status_t status = eSTATUS_ERROR;

    switch(halStatus)
    {
        case HAL_OK:
            status = eSTATUS_OK;
            break;
        case HAL_BUSY:
            status = eSTATUS_BUSY;
            break;
        case HAL_TIMEOUT:
            status = eSTATUS_TIMEOUT;
            break;
        case HAL_ERROR:
        default:
            status = eSTATUS_ERROR;
            break;
    }

    return status;
}

/** @} DOXYGEN GROUP TAG END OF FILE */
