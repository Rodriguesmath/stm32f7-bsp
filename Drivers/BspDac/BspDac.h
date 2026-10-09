/******************************************************************************/
/**
* @file BspDac.h
* @addtogroup BSP_DAC
* @{
******************************************************************************/
#ifndef _BSP_DAC_H_
#define _BSP_DAC_H_

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
#ifndef HAL_DAC_MODULE_ENABLED
typedef struct __DAC_HandleTypeDef DAC_HandleTypeDef;

#ifndef DAC_CHANNEL_1
#define DAC_CHANNEL_1                       0x00000000U
#endif

#ifndef DAC_CHANNEL_2
#define DAC_CHANNEL_2                       0x00000010U
#endif

#ifndef DAC_ALIGN_12B_R
#define DAC_ALIGN_12B_R                     0x00000000U
#endif
#endif

/*******************************************************************************
* CONFIGURACOES
******************************************************************************/
/// Tensao de referencia analogica padrao para o DAC da Nucleo F767ZI
#define dBSP_DAC_DEFAULT_VREF               3.3f                                // [Volts]

/*******************************************************************************
* DEFINES PUBLICOS
******************************************************************************/
/// Resolucao do DAC do STM32F7 em bits (12 bits)
#define dBSP_DAC_RESOLUTION_BITS            12

/// Valor maximo de contagem do DAC de 12 bits (2^12 - 1)
#define dBSP_DAC_MAX_COUNTS                 4095

/*******************************************************************************
* TIPOS DE DADOS PUBLICOS
******************************************************************************/
/// Estrutura de controle da instancia de saida do DAC
typedef struct
{
    /// Ponteiro para o handle gerado pelo CubeMX (ex: &hdac)
    DAC_HandleTypeDef *hdac;
    /// Canal configurado (DAC_CHANNEL_1 ou DAC_CHANNEL_2)
    u32 channel;
    /// Tensao de referencia analogica em Volts (ex: 3.3V)
    f32 vRef;
    /// Ultimo valor bruto de contagem enviado ao registrador
    u16 lastRawValue;
    /// Flag de estado da instancia
    bool isRunning;
} bspDac_t;

/*******************************************************************************
* PROTOTIPOS PUBLICOS
******************************************************************************/
/******************************************************************************/
/** @brief Inicializa a saida analogica DAC e associa ao handle do CubeMX.
* @param dev: ponteiro para a estrutura de dados do DAC.
* @param hdac: ponteiro para o handle HAL do DAC gerado pelo CubeMX.
* @param channel: canal de saida (DAC_CHANNEL_1 ou DAC_CHANNEL_2).
* @param vRef: tensao de referencia analogica em Volts (normalmente 3.3f).
* @retval eSTATUS_OK se sucesso, ou eSTATUS_INVALID_PARAM se ponteiro nulo.
* @details Configura a estrutura, inicia o canal analogico via HAL_DAC_Start
* e posiciona a tensao inicial de saida em 0 Volts.
*
* Exemplo de uso:
* @code
* bspDac_t geradorOnda;
* BspDac_Init(&geradorOnda, &hdac, DAC_CHANNEL_1, 3.3f);
* @endcode
******************************************************************************/
status_t BspDac_Init(bspDac_t *dev, DAC_HandleTypeDef *hdac, u32 channel, f32 vRef);

/******************************************************************************/
/** @brief Define o valor bruto de contagem do registrador do DAC (0 a 4095).
* @param dev: ponteiro para a estrutura do DAC.
* @param rawValue: valor digital de 12 bits a ser convertido para tensao.
* @retval eSTATUS_OK se atualizado com sucesso, ou codigo de erro da HAL.
* @details Limita o valor ao maximo suportado pelo conversor de 12 bits (4095)
* e escreve no registrador com alinhamento a direita (DAC_ALIGN_12B_R).
*
* Exemplo de uso:
* @code
* // Ajusta a saida para exatamente metade da escala (2048 contagens)
* BspDac_SetRaw(&geradorOnda, 2048);
* @endcode
******************************************************************************/
status_t BspDac_SetRaw(bspDac_t *dev, u16 rawValue);

/******************************************************************************/
/** @brief Ajusta a tensao de saida analogica diretamente em Volts.
* @param dev: ponteiro para a estrutura do DAC.
* @param voltage: tensao desejada no pino em Volts (ex: 1.65f).
* @retval eSTATUS_OK se aplicado com sucesso, ou erro se parametros invalidos.
* @details Aplica saturacao de seguranca entre 0.0V e VRef, converte o valor
* para a escala de 12 bits atraves da formula: Counts = (Tensao * 4095) / VRef,
* e atualiza o registrador de saida do microcontrolador.
*
* Exemplo de uso:
* @code
* // Coloca exatamente 2.5 Volts no pino PA4 (DAC_CHANNEL_1)
* BspDac_SetVoltage(&geradorOnda, 2.5f);
* @endcode
******************************************************************************/
status_t BspDac_SetVoltage(bspDac_t *dev, f32 voltage);

/******************************************************************************/
/** @brief Define a saida em milivolts usando aritmetica inteira (0 a 3300 mV).
* @param dev: ponteiro para a estrutura do DAC.
* @param milliVolts: tensao desejada em milivolts.
* @retval eSTATUS_OK se sucesso, ou erro caso falha.
* @details Facilita o envio de niveis de tensao sem necessidade de passar
* numeros decimais na chamada da funcao.
*
* Exemplo de uso:
* @code
* // Ajusta a saida para 1200 mV (1.2 V)
* BspDac_SetMilliVolts(&geradorOnda, 1200);
* @endcode
******************************************************************************/
status_t BspDac_SetMilliVolts(bspDac_t *dev, u16 milliVolts);

/******************************************************************************/
/** @brief Ajusta o nivel de saida por valor normalizado entre 0.0 e 1.0.
* @param dev: ponteiro para a estrutura do DAC.
* @param ratio: valor decimal entre 0.0f (0%) e 1.0f (100% da escala).
* @retval eSTATUS_OK se sucesso, ou codigo de erro.
* @details Muito util para geradores de funcao (senoides, ondas triangulares)
* onde os valores estao normalizados na faixa unitaria.
*
* Exemplo de uso:
* @code
* // Saida ajustada para 75% da tensao de referencia
* BspDac_SetNormalized(&geradorOnda, 0.75f);
* @endcode
******************************************************************************/
status_t BspDac_SetNormalized(bspDac_t *dev, f32 ratio);

/******************************************************************************/
/** @brief Desliga a saida analogica do DAC para economia de energia.
* @param dev: ponteiro para a estrutura do DAC.
* @retval eSTATUS_OK se parado com sucesso, ou erro se falha na HAL.
* @details Invoca HAL_DAC_Stop desativando o conversor interno e marcando
* a flag da instancia como inativa.
*
* Exemplo de uso:
* @code
* BspDac_Stop(&geradorOnda);
* @endcode
******************************************************************************/
status_t BspDac_Stop(bspDac_t *dev);

#endif /* _BSP_DAC_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */
