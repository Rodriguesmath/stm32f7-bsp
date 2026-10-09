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

/******************************************************************************/
/** @brief Inicia a transmissao continua de forma de onda analogica via DMA.
* @param dev: ponteiro para a estrutura do DAC.
* @param lookupTable: array constante contendo os pontos da onda alinhado a 32 bytes (u16).
* @param length: quantidade de pontos contidos na tabela.
* @retval eSTATUS_OK se disparado com sucesso, ou codigo de erro.
* @details Descarrega a D-Cache (Clean) para garantir integridade e inicia
* HAL_DAC_Start_DMA. O hardware gera a forma de onda autonomamente em loop
* acionado por Timer com ZERO ciclos de processamento da CPU.
*
* Exemplo de uso:
* @code
* dBSP_DMA_BUFFER_ALIGN u16 tabelaSenoide[128];
* BspDac_GenerateSineLookupTable(tabelaSenoide, 128, 0.5f, 2.5f, 3.3f);
* BspDac_StartWaveformDma(&saidaDac, tabelaSenoide, 128);
* @endcode
******************************************************************************/
status_t BspDac_StartWaveformDma(bspDac_t *dev, const u16 *lookupTable, u32 length);

/******************************************************************************/
/** @brief Encerra a geracao continua de forma de onda via DMA.
* @param dev: ponteiro para a estrutura do DAC.
* @retval eSTATUS_OK se encerrado, ou codigo de erro.
* @details Invoca HAL_DAC_Stop_DMA desativando o fluxo de dados.
*
* Exemplo de uso:
* @code
* BspDac_StopWaveformDma(&saidaDac);
* @endcode
******************************************************************************/
status_t BspDac_StopWaveformDma(bspDac_t *dev);

/******************************************************************************/
/** @brief Preenche uma tabela de lookup com pontos de uma onda senoidal pura.
* @param lookupTable: array de destino (recomenda-se alinhado a 32 bytes).
* @param length: numero de pontos que comporao um ciclo completo da onda (ex: 64, 128).
* @param minVoltage: tensao de vale da senoide em Volts (ex: 0.5V).
* @param maxVoltage: tensao de pico da senoide em Volts (ex: 2.5V).
* @param vRef: tensao de referencia analogica do DAC (ex: 3.3V).
* @retval eSTATUS_OK se calculada com sucesso, ou eSTATUS_INVALID_PARAM se erro.
* @details Calcula os pontos matematicos da funcao seno e converte diretamente
* para contagens de 12 bits com saturacao automatica dentro da faixa segura.
*
* Exemplo de uso:
* @code
* u16 senoide[64];
* BspDac_GenerateSineLookupTable(senoide, 64, 0.2f, 3.0f, 3.3f);
* @endcode
******************************************************************************/
status_t BspDac_GenerateSineLookupTable(u16 *lookupTable, u32 length, f32 minVoltage, f32 maxVoltage, f32 vRef);

/******************************************************************************/
/** @brief Preenche uma tabela de lookup com pontos de uma onda triangular.
* @param lookupTable: array de destino (recomenda-se alinhado a 32 bytes).
* @param length: numero de pontos de um ciclo completo.
* @param minVoltage: tensao minima da onda em Volts.
* @param maxVoltage: tensao maxima da onda em Volts.
* @param vRef: tensao de referencia analogica do DAC.
* @retval eSTATUS_OK se gerado com sucesso, ou eSTATUS_INVALID_PARAM.
* @details Gera rampa linear de subida e descida simetrica entre minVoltage e maxVoltage.
*
* Exemplo de uso:
* @code
* u16 triangular[64];
* BspDac_GenerateTriangleLookupTable(triangular, 64, 0.0f, 3.3f, 3.3f);
* @endcode
******************************************************************************/
status_t BspDac_GenerateTriangleLookupTable(u16 *lookupTable, u32 length, f32 minVoltage, f32 maxVoltage, f32 vRef);

#endif /* _BSP_DAC_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */
