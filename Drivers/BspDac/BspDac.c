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
#include "BspDma.h"

#include <math.h>

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

__attribute__((weak)) HAL_StatusTypeDef HAL_DAC_Start_DMA(DAC_HandleTypeDef *hdac, uint32_t Channel, uint32_t *pData, uint32_t Length, uint32_t Alignment)
{
    (void)hdac;
    (void)Channel;
    (void)pData;
    (void)Length;
    (void)Alignment;
    return HAL_OK;
}

__attribute__((weak)) HAL_StatusTypeDef HAL_DAC_Stop_DMA(DAC_HandleTypeDef *hdac, uint32_t Channel)
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

/******************************************************************************/
/** @brief Inicia a transmissao continua de forma de onda analogica via DMA.
* @param dev: ponteiro para a estrutura do DAC.
* @param lookupTable: array constante contendo os pontos da onda alinhado a 32 bytes (u16).
* @param length: quantidade de pontos contidos na tabela.
* @retval eSTATUS_OK se disparado com sucesso, ou codigo de erro.
******************************************************************************/
status_t BspDac_StartWaveformDma(bspDac_t *dev, const u16 *lookupTable, u32 length)
{
    status_t ret = eSTATUS_INVALID_PARAM;

    if((dev != dNULL) && (lookupTable != dNULL) && (length > 0) && (dev->isRunning == true))
    {
        // 1. Descarrega a D-Cache da CPU para a RAM fisica antes da leitura pelo DMA
        BspDma_CacheClean((void *)(uintptr_t)lookupTable, length * sizeof(u16));

        HAL_StatusTypeDef halStatus = HAL_DAC_Start_DMA(dev->hdac, dev->channel, (uint32_t *)(uintptr_t)lookupTable, length, DAC_ALIGN_12B_R);
        ret = BspDac_MapHalStatus(halStatus);
    }

    return ret;
}

/******************************************************************************/
/** @brief Encerra a geracao continua de forma de onda via DMA.
* @param dev: ponteiro para a estrutura do DAC.
* @retval eSTATUS_OK se encerrado, ou codigo de erro.
******************************************************************************/
status_t BspDac_StopWaveformDma(bspDac_t *dev)
{
    status_t ret = eSTATUS_INVALID_PARAM;

    if((dev != dNULL) && (dev->hdac != dNULL))
    {
        HAL_StatusTypeDef halStatus = HAL_DAC_Stop_DMA(dev->hdac, dev->channel);
        ret = BspDac_MapHalStatus(halStatus);
    }

    return ret;
}

/******************************************************************************/
/** @brief Preenche uma tabela de lookup com pontos de uma onda senoidal pura.
* @param lookupTable: array de destino (recomenda-se alinhado a 32 bytes).
* @param length: numero de pontos que comporao um ciclo completo da onda.
* @param minVoltage: tensao de vale da senoide em Volts.
* @param maxVoltage: tensao de pico da senoide em Volts.
* @param vRef: tensao de referencia analogica do DAC.
* @retval eSTATUS_OK se calculada com sucesso, ou eSTATUS_INVALID_PARAM se erro.
******************************************************************************/
status_t BspDac_GenerateSineLookupTable(u16 *lookupTable, u32 length, f32 minVoltage, f32 maxVoltage, f32 vRef)
{
    status_t ret = eSTATUS_INVALID_PARAM;

    if((lookupTable != dNULL) && (length > 0) && (vRef > 0.0f) && (maxVoltage >= minVoltage))
    {
        f32 amplitude = (maxVoltage - minVoltage) / 2.0f;
        f32 offset = minVoltage + amplitude;
        f32 twoPi = 6.28318530718f;
        u32 i = 0;

        for(i = 0; i < length; i++)
        {
            f32 angle = (twoPi * (f32)i) / (f32)length;
            f32 voltage = offset + (amplitude * sinf(angle));

            if(voltage < 0.0f)
            {
                voltage = 0.0f;
            }
            else if(voltage > vRef)
            {
                voltage = vRef;
            }

            f32 counts = (voltage * (f32)dBSP_DAC_MAX_COUNTS) / vRef;
            lookupTable[i] = (u16)(counts + 0.5f);
        }

        ret = eSTATUS_OK;
    }

    return ret;
}

/******************************************************************************/
/** @brief Preenche uma tabela de lookup com pontos de uma onda triangular.
* @param lookupTable: array de destino (recomenda-se alinhado a 32 bytes).
* @param length: numero de pontos de um ciclo completo.
* @param minVoltage: tensao minima da onda em Volts.
* @param maxVoltage: tensao maxima da onda em Volts.
* @param vRef: tensao de referencia analogica do DAC.
* @retval eSTATUS_OK se gerado com sucesso, ou eSTATUS_INVALID_PARAM.
******************************************************************************/
status_t BspDac_GenerateTriangleLookupTable(u16 *lookupTable, u32 length, f32 minVoltage, f32 maxVoltage, f32 vRef)
{
    status_t ret = eSTATUS_INVALID_PARAM;

    if((lookupTable != dNULL) && (length >= 2) && (vRef > 0.0f) && (maxVoltage >= minVoltage))
    {
        u32 halfLength = length / 2;
        f32 deltaUp = (maxVoltage - minVoltage) / (f32)halfLength;
        f32 deltaDown = (maxVoltage - minVoltage) / (f32)(length - halfLength);
        u32 i = 0;

        for(i = 0; i < length; i++)
        {
            f32 voltage = 0.0f;
            if(i < halfLength)
            {
                voltage = minVoltage + (deltaUp * (f32)i);
            }
            else
            {
                voltage = maxVoltage - (deltaDown * (f32)(i - halfLength));
            }

            if(voltage < 0.0f)
            {
                voltage = 0.0f;
            }
            else if(voltage > vRef)
            {
                voltage = vRef;
            }

            f32 counts = (voltage * (f32)dBSP_DAC_MAX_COUNTS) / vRef;
            lookupTable[i] = (u16)(counts + 0.5f);
        }

        ret = eSTATUS_OK;
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
