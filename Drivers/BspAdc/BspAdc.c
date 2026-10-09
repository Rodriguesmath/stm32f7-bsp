/******************************************************************************/
/**
* @file BspAdc.c
* @addtogroup BSP_ADC
* @brief Implementacao do driver de conversao analogico-digital com filtragem.
* @author Rodrigues
* @details
* \n <b>Ferramentas:</b>
* - STM32CubeIDE / GCC ARM.
*
* \n <b>Dependencias:</b>
* - BspAdc.h
* - stm32f7xx_hal.h (HAL ADC Module)
*
* \n <b>Observacoes:</b>
* - Inclui filtro de media movel integrado conforme recomendacao da norma Assert.
* - Suporta conversao automatica para Volts e Milivolts.
*
* Changelog
* @version <b>1.0.0 - 08/10/2026</b> \n Rodrigues \n Versao inicial com filtro de media movel.
*
* @copyright Generic STM32F7 BSP Library
* @{
******************************************************************************/
/*******************************************************************************
* INCLUDES
******************************************************************************/
#include "BspAdc.h"
#include "BspDma.h"

#include <string.h>

/*******************************************************************************
* DEFINES LOCAIS (fixos, apenas auxiliar para calculos)
******************************************************************************/
// Verificacao de integridade das configuracoes do header
#if (dBSP_ADC_MOVING_AVG_SIZE == 0)
#error "dBSP_ADC_MOVING_AVG_SIZE deve ser maior do que zero."
#endif

#if (dBSP_ADC_CONVERSION_TIMEOUT_MS == 0)
#error "dBSP_ADC_CONVERSION_TIMEOUT_MS deve ser maior do que zero."
#endif

/// Constante multiplicadora para conversao de Volts para Milivolts
#define dMILLIVOLTS_MULTIPLIER              1000.0f

/*******************************************************************************
* CONSTANTES
******************************************************************************/

/*******************************************************************************
* ESTRUTURAS DE DADOS LOCAIS
******************************************************************************/
/// Variaveis internas de acompanhamento estatistico do ADC
static struct
{
    /// Quantidade total de conversoes realizadas
    u32 totalConversions;
} bspAdc;

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

__attribute__((weak)) HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *hadc)
{
    (void)hadc;
    return HAL_OK;
}

__attribute__((weak)) HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *hadc, uint32_t Timeout)
{
    (void)hadc;
    (void)Timeout;
    return HAL_OK;
}

__attribute__((weak)) uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *hadc)
{
    (void)hadc;
    return 2048U;
}

__attribute__((weak)) HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *hadc)
{
    (void)hadc;
    return HAL_OK;
}

__attribute__((weak)) HAL_StatusTypeDef HAL_ADC_Start_DMA(ADC_HandleTypeDef *hadc, uint32_t *pData, uint32_t Length)
{
    (void)hadc;
    (void)pData;
    (void)Length;
    return HAL_OK;
}

__attribute__((weak)) HAL_StatusTypeDef HAL_ADC_Stop_DMA(ADC_HandleTypeDef *hadc)
{
    (void)hadc;
    return HAL_OK;
}
#endif

static status_t BspAdc_MapHalStatus(HAL_StatusTypeDef halStatus);
static u16 BspAdc_PushSampleToFilter(bspAdcFilter_t *filter, u16 sample);

/*******************************************************************************
* FUNCOES PUBLICAS
******************************************************************************/
/******************************************************************************/
/** @brief Inicializa a instancia do ADC vinculando ao handle do CubeMX.
* @param dev: ponteiro para a estrutura de dados do ADC.
* @param hadc: ponteiro para o handle HAL gerado pelo CubeMX.
* @param vRef: tensao de referencia do circuito analogico em Volts.
* @retval eSTATUS_OK se sucesso, ou eSTATUS_INVALID_PARAM se ponteiro nulo.
******************************************************************************/
status_t BspAdc_Init(bspAdc_t *dev, ADC_HandleTypeDef *hadc, f32 vRef)
{
    status_t ret = eSTATUS_OK;

    if((dev == dNULL) || (hadc == dNULL) || (vRef <= 0.0f))
    {
        ret = eSTATUS_INVALID_PARAM;
    }
    else
    {
        dev->hadc = hadc;
        dev->vRef = vRef;
        dev->isInitialized = true;

        ret = BspAdc_ResetFilter(dev);
    }

    return ret;
}

/******************************************************************************/
/** @brief Realiza uma leitura direta e bruta do conversor AD (0 a 4095).
* @param dev: ponteiro para a estrutura do ADC.
* @param rawVal: ponteiro onde sera armazenado o valor de 12 bits medido.
* @retval eSTATUS_OK se conversao concluida, ou erro caso falha na HAL.
******************************************************************************/
status_t BspAdc_ReadRaw(bspAdc_t *dev, u16 *rawVal)
{
    status_t ret = eSTATUS_OK;

    if((dev == dNULL) || (rawVal == dNULL) || (dev->isInitialized == false))
    {
        ret = eSTATUS_INVALID_PARAM;
    }
    else
    {
        HAL_StatusTypeDef halStatus = HAL_ADC_Start(dev->hadc);
        ret = BspAdc_MapHalStatus(halStatus);

        if(ret == eSTATUS_OK)
        {
            halStatus = HAL_ADC_PollForConversion(dev->hadc, dBSP_ADC_CONVERSION_TIMEOUT_MS);
            ret = BspAdc_MapHalStatus(halStatus);

            if(ret == eSTATUS_OK)
            {
                u32 val = (u32)HAL_ADC_GetValue(dev->hadc);
                *rawVal = (u16)(val & (u32)dBSP_ADC_MAX_COUNTS);
                bspAdc.totalConversions++;
            }

            (void)HAL_ADC_Stop(dev->hadc);
        }
    }

    return ret;
}

/******************************************************************************/
/** @brief Converte a leitura analogica diretamente para Volts (0.0V a VRef).
* @param dev: ponteiro para a estrutura do ADC.
* @param voltage: ponteiro onde sera gravado o valor de tensao em Volts.
* @retval eSTATUS_OK se sucesso, ou codigo de erro caso falha na leitura.
******************************************************************************/
status_t BspAdc_ReadVoltage(bspAdc_t *dev, f32 *voltage)
{
    status_t ret = eSTATUS_OK;

    if((dev == dNULL) || (voltage == dNULL))
    {
        ret = eSTATUS_INVALID_PARAM;
    }
    else
    {
        u16 rawVal = 0;
        ret = BspAdc_ReadRaw(dev, &rawVal);

        if(ret == eSTATUS_OK)
        {
            *voltage = ((f32)rawVal * dev->vRef) / (f32)dBSP_ADC_MAX_COUNTS;
        }
    }

    return ret;
}

/******************************************************************************/
/** @brief Converte a leitura analogica para milivolts (0 a 3300 mV).
* @param dev: ponteiro para a estrutura do ADC.
* @param milliVolts: ponteiro onde sera gravado o valor em milivolts.
* @retval eSTATUS_OK se sucesso, ou erro caso falha de conversao.
******************************************************************************/
status_t BspAdc_ReadMilliVolts(bspAdc_t *dev, u16 *milliVolts)
{
    status_t ret = eSTATUS_OK;

    if((dev == dNULL) || (milliVolts == dNULL))
    {
        ret = eSTATUS_INVALID_PARAM;
    }
    else
    {
        u16 rawVal = 0;
        ret = BspAdc_ReadRaw(dev, &rawVal);

        if(ret == eSTATUS_OK)
        {
            f32 mv = (((f32)rawVal * dev->vRef) * dMILLIVOLTS_MULTIPLIER) / (f32)dBSP_ADC_MAX_COUNTS;
            *milliVolts = (u16)mv;
        }
    }

    return ret;
}

/******************************************************************************/
/** @brief Realiza leitura com filtro de media movel integrado.
* @param dev: ponteiro para a estrutura do ADC.
* @param filteredRaw: ponteiro onde sera gravada a media das ultimas amostras.
* @retval eSTATUS_OK se medido com sucesso, ou codigo de erro.
******************************************************************************/
status_t BspAdc_ReadFilteredRaw(bspAdc_t *dev, u16 *filteredRaw)
{
    status_t ret = eSTATUS_OK;

    if((dev == dNULL) || (filteredRaw == dNULL))
    {
        ret = eSTATUS_INVALID_PARAM;
    }
    else
    {
        u16 newSample = 0;
        ret = BspAdc_ReadRaw(dev, &newSample);

        if(ret == eSTATUS_OK)
        {
            *filteredRaw = BspAdc_PushSampleToFilter(&dev->filter, newSample);
        }
    }

    return ret;
}

/******************************************************************************/
/** @brief Retorna a tensao filtrada em Volts apos processamento da media movel.
* @param dev: ponteiro para a estrutura do ADC.
* @param filteredVoltage: ponteiro onde sera armazenada a tensao filtrada.
* @retval eSTATUS_OK se sucesso, ou codigo de erro.
******************************************************************************/
status_t BspAdc_ReadFilteredVoltage(bspAdc_t *dev, f32 *filteredVoltage)
{
    status_t ret = eSTATUS_OK;

    if((dev == dNULL) || (filteredVoltage == dNULL))
    {
        ret = eSTATUS_INVALID_PARAM;
    }
    else
    {
        u16 filteredRaw = 0;
        ret = BspAdc_ReadFilteredRaw(dev, &filteredRaw);

        if(ret == eSTATUS_OK)
        {
            *filteredVoltage = ((f32)filteredRaw * dev->vRef) / (f32)dBSP_ADC_MAX_COUNTS;
        }
    }

    return ret;
}

/******************************************************************************/
/** @brief Limpa o historico de amostras do filtro de media movel.
* @param dev: ponteiro para a estrutura do ADC.
* @retval eSTATUS_OK se limpo, ou eSTATUS_INVALID_PARAM se ponteiro nulo.
******************************************************************************/
status_t BspAdc_ResetFilter(bspAdc_t *dev)
{
    status_t ret = eSTATUS_OK;

    if(dev == dNULL)
    {
        ret = eSTATUS_INVALID_PARAM;
    }
    else
    {
        (void)memset(dev->filter.samples, 0, sizeof(dev->filter.samples));
        dev->filter.headIndex = 0;
        dev->filter.sampleCount = 0;
        dev->filter.runningSum = 0;
    }

    return ret;
}

/******************************************************************************/
/** @brief Inicia a amostragem continua de dados analogicos via DMA.
* @param dev: ponteiro para a estrutura do ADC.
* @param buffer: ponteiro para o buffer de destino alinhado a 32 bytes (u16).
* @param length: quantidade total de amostras no buffer.
* @retval eSTATUS_OK se disparado com sucesso, ou codigo de erro.
******************************************************************************/
status_t BspAdc_StartContinuousDma(bspAdc_t *dev, u16 *buffer, u32 length)
{
    status_t ret = eSTATUS_INVALID_PARAM;

    if((dev != dNULL) && (buffer != dNULL) && (length > 0) && (dev->isInitialized == true))
    {
        // Descarta linhas de cache antigas antes do DMA preencher o buffer
        BspDma_CacheInvalidate((void *)(uintptr_t)buffer, length * sizeof(u16));

        HAL_StatusTypeDef halStatus = HAL_ADC_Start_DMA(dev->hadc, (uint32_t *)(uintptr_t)buffer, length);
        ret = BspAdc_MapHalStatus(halStatus);
    }

    return ret;
}

/******************************************************************************/
/** @brief Encerra a amostragem continua por DMA.
* @param dev: ponteiro para a estrutura do ADC.
* @retval eSTATUS_OK se encerrado, ou codigo de erro.
******************************************************************************/
status_t BspAdc_StopContinuousDma(bspAdc_t *dev)
{
    status_t ret = eSTATUS_INVALID_PARAM;

    if((dev != dNULL) && (dev->hadc != dNULL))
    {
        HAL_StatusTypeDef halStatus = HAL_ADC_Stop_DMA(dev->hadc);
        ret = BspAdc_MapHalStatus(halStatus);
    }

    return ret;
}

/******************************************************************************/
/** @brief Converte um bloco inteiro de amostras brutas para Volts de uma so vez.
* @param dev: ponteiro para a estrutura do ADC.
* @param rawBuffer: array com as amostras brutas de 12 bits preenchidas pelo DMA.
* @param voltageBuffer: array de saida onde serao gravados os valores em Volts.
* @param length: quantidade de amostras a converter.
* @retval eSTATUS_OK se convertido com sucesso, ou codigo de erro.
******************************************************************************/
status_t BspAdc_ConvertBufferToVoltages(const bspAdc_t *dev, const u16 *rawBuffer, f32 *voltageBuffer, u32 length)
{
    status_t ret = eSTATUS_INVALID_PARAM;

    if((dev != dNULL) && (rawBuffer != dNULL) && (voltageBuffer != dNULL) && (length > 0))
    {
        f32 scale = dev->vRef / (f32)dBSP_ADC_MAX_COUNTS;
        u32 i = 0;

        for(i = 0; i < length; i++)
        {
            voltageBuffer[i] = (f32)rawBuffer[i] * scale;
        }

        ret = eSTATUS_OK;
    }

    return ret;
}

/******************************************************************************/
/** @brief Calcula a media aritmetica de um bloco de amostras lido pelo DMA.
* @param buffer: array com as amostras brutas.
* @param length: quantidade de amostras no array.
* @retval Valor medio calculado em contagens (u16).
******************************************************************************/
u16 BspAdc_CalculateBufferAverage(const u16 *buffer, u32 length)
{
    u16 avg = 0;

    if((buffer != dNULL) && (length > 0))
    {
        u64 sum = 0;
        u32 i = 0;

        for(i = 0; i < length; i++)
        {
            sum += buffer[i];
        }

        avg = (u16)(sum / (u64)length);
    }

    return avg;
}

/*******************************************************************************
* FUNCOES LOCAIS
******************************************************************************/
/******************************************************************************/
/** @brief Mapeia o status de retorno da HAL para a tipagem status_t do BSP.
* @param halStatus: codigo de status retornado pela biblioteca HAL.
* @retval status_t mapeado equivalente.
******************************************************************************/
static status_t BspAdc_MapHalStatus(HAL_StatusTypeDef halStatus)
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

/******************************************************************************/
/** @brief Insere uma nova amostra no filtro e calcula a media atual.
* @param filter: ponteiro para a estrutura do filtro.
* @param sample: nova amostra de 12 bits medida.
* @retval Media aritmetica calculada com as amostras disponiveis.
******************************************************************************/
static u16 BspAdc_PushSampleToFilter(bspAdcFilter_t *filter, u16 sample)
{
    if(filter->sampleCount < dBSP_ADC_MOVING_AVG_SIZE)
    {
        filter->samples[filter->headIndex] = sample;
        filter->runningSum += sample;
        filter->sampleCount++;
        filter->headIndex = (filter->headIndex + 1) % dBSP_ADC_MOVING_AVG_SIZE;
    }
    else
    {
        // Remove a amostra mais antiga da soma corrente e insere a nova
        u16 oldestSample = filter->samples[filter->headIndex];
        filter->runningSum -= oldestSample;
        filter->runningSum += sample;
        filter->samples[filter->headIndex] = sample;
        filter->headIndex = (filter->headIndex + 1) % dBSP_ADC_MOVING_AVG_SIZE;
    }

    u16 avg = 0;
    if(filter->sampleCount > 0)
    {
        avg = (u16)(filter->runningSum / filter->sampleCount);
    }

    return avg;
}

/** @} DOXYGEN GROUP TAG END OF FILE */
