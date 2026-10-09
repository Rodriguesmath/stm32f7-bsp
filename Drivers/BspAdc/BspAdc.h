/******************************************************************************/
/**
* @file BspAdc.h
* @addtogroup BSP_ADC
* @{
******************************************************************************/
#ifndef _BSP_ADC_H_
#define _BSP_ADC_H_

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
#ifndef HAL_ADC_MODULE_ENABLED
typedef struct __ADC_HandleTypeDef ADC_HandleTypeDef;
#endif

/*******************************************************************************
* CONFIGURACOES
******************************************************************************/
/// Quantidade de amostras para o filtro de media movel do sinal analogico
#define dBSP_ADC_MOVING_AVG_SIZE            8                                   // [amostras: 4, 8 ou 16]

/// Timeout padrao para conversao analogica em milissegundos
#define dBSP_ADC_CONVERSION_TIMEOUT_MS      100                                 // [ms]

/// Tensao padrao de referencia analógica da placa Nucleo F767ZI
#define dBSP_ADC_DEFAULT_VREF               3.3f                                // [Volts]

/*******************************************************************************
* DEFINES PUBLICOS
******************************************************************************/
/// Resolucao maxima em bits do conversor analogico do STM32F7 (12 bits)
#define dBSP_ADC_RESOLUTION_BITS            12

/// Valor maximo de contagem do conversor de 12 bits (2^12 - 1)
#define dBSP_ADC_MAX_COUNTS                 4095

/*******************************************************************************
* TIPOS DE DADOS PUBLICOS
******************************************************************************/
/// Estrutura para buffer circular de media movel
typedef struct
{
    /// Historico de amostras brutas
    u16 samples[dBSP_ADC_MOVING_AVG_SIZE];
    /// Indice da proxima amostra a ser inserida
    u8 headIndex;
    /// Quantidade de amostras validas acumuladas
    u8 sampleCount;
    /// Soma acumulada das amostras para calculo rapido da media
    u32 runningSum;
} bspAdcFilter_t;

/// Estrutura de controle da instancia do periférico ADC
typedef struct
{
    /// Ponteiro para o handle do ADC gerado pelo CubeMX
    ADC_HandleTypeDef *hadc;
    /// Tensao de referencia analógica em Volts (ex: 3.3V)
    f32 vRef;
    /// Instancia do filtro de media movel integrado
    bspAdcFilter_t filter;
    /// Flag de estado de inicializacao da instancia
    bool isInitialized;
} bspAdc_t;

/*******************************************************************************
* PROTOTIPOS PUBLICOS
******************************************************************************/
/******************************************************************************/
/** @brief Inicializa a instancia do ADC vinculando ao handle do CubeMX.
* @param dev: ponteiro para a estrutura de dados do ADC.
* @param hadc: ponteiro para o handle HAL gerado pelo CubeMX (ex: &hadc1).
* @param vRef: tensao de referencia do circuito analogico em Volts (ex: 3.3f).
* @retval eSTATUS_OK se sucesso, ou eSTATUS_INVALID_PARAM se ponteiro nulo.
* @details Associa a instancia ao driver da ST, zera o acumulador do filtro de
* media movel e define a constante de referencia de tensao.
*
* Exemplo de uso:
* @code
* bspAdc_t sensorPressao;
* BspAdc_Init(&sensorPressao, &hadc1, 3.3f);
* @endcode
******************************************************************************/
status_t BspAdc_Init(bspAdc_t *dev, ADC_HandleTypeDef *hadc, f32 vRef);

/******************************************************************************/
/** @brief Realiza uma leitura direta e bruta do conversor AD (0 a 4095).
* @param dev: ponteiro para a estrutura do ADC.
* @param rawVal: ponteiro onde sera armazenado o valor de 12 bits medido.
* @retval eSTATUS_OK se conversao concluida, ou erro caso timeout da HAL.
* @details Dispara a conversao analogica via HAL_ADC_Start, aguarda o termino
* com base no timeout configurado e desliga o conversor para economia de energia.
*
* Exemplo de uso:
* @code
* u16 valorBruto = 0;
* if(BspAdc_ReadRaw(&sensorPressao, &valorBruto) == eSTATUS_OK)
* {
*     // Processa valor bruto
* }
* @endcode
******************************************************************************/
status_t BspAdc_ReadRaw(bspAdc_t *dev, u16 *rawVal);

/******************************************************************************/
/** @brief Converte a leitura analogica diretamente para Volts (0.0V a VRef).
* @param dev: ponteiro para a estrutura do ADC.
* @param voltage: ponteiro onde sera gravado o valor de tensao em Volts.
* @retval eSTATUS_OK se sucesso, ou codigo de erro caso falha na leitura.
* @details Executa a amostragem bruta e aplica a equacao de escala:
* Tensao = (Amostra * VRef) / 4095.0.
*
* Exemplo de uso:
* @code
* f32 tensaoVolts = 0.0f;
* if(BspAdc_ReadVoltage(&sensorPressao, &tensaoVolts) == eSTATUS_OK)
* {
*     BspUart_Printf(&console, "Tensao lida: %.3f V\r\n", tensaoVolts);
* }
* @endcode
******************************************************************************/
status_t BspAdc_ReadVoltage(bspAdc_t *dev, f32 *voltage);

/******************************************************************************/
/** @brief Converte a leitura analogica para milivolts (0 a 3300 mV).
* @param dev: ponteiro para a estrutura do ADC.
* @param milliVolts: ponteiro onde sera gravado o valor em milivolts.
* @retval eSTATUS_OK se sucesso, ou erro caso falha de conversao.
* @details Converte o valor para inteiros em milivolts sem operacoes com ponto
* flutuante em tempo de execucao, ideal para microcontroladores ou logs rapidos.
*
* Exemplo de uso:
* @code
* u16 tensaoMilliVolts = 0;
* BspAdc_ReadMilliVolts(&sensorPressao, &tensaoMilliVolts);
* @endcode
******************************************************************************/
status_t BspAdc_ReadMilliVolts(bspAdc_t *dev, u16 *milliVolts);

/******************************************************************************/
/** @brief Realiza leitura com filtro de media movel integrado.
* @param dev: ponteiro para a estrutura do ADC.
* @param filteredRaw: ponteiro onde sera gravada a media das ultimas amostras.
* @retval eSTATUS_OK se medido com sucesso, ou codigo de erro.
* @details Realiza uma nova amostragem, insere no buffer de historico,
* descarta a amostra mais antiga e calcula a media aritmetica em O(1) usando soma
* corrente, eliminando ruidos de alta frequencia conforme a norma Assert.
*
* Exemplo de uso:
* @code
* u16 sinalEstavel = 0;
* if(BspAdc_ReadFilteredRaw(&sensorPressao, &sinalEstavel) == eSTATUS_OK)
* {
*     // Sinal filtrado pronto para controle
* }
* @endcode
******************************************************************************/
status_t BspAdc_ReadFilteredRaw(bspAdc_t *dev, u16 *filteredRaw);

/******************************************************************************/
/** @brief Retorna a tensao filtrada em Volts apos processamento da media movel.
* @param dev: ponteiro para a estrutura do ADC.
* @param filteredVoltage: ponteiro onde sera armazenada a tensao filtrada.
* @retval eSTATUS_OK se sucesso, ou codigo de erro.
* @details Combina a filtragem por media movel com a conversao para grandeza
* de engenharia em Volts com alta estabilidade.
*
* Exemplo de uso:
* @code
* f32 tensaoFiltrada = 0.0f;
* BspAdc_ReadFilteredVoltage(&sensorPressao, &tensaoFiltrada);
* @endcode
******************************************************************************/
status_t BspAdc_ReadFilteredVoltage(bspAdc_t *dev, f32 *filteredVoltage);

/******************************************************************************/
/** @brief Limpa o historico de amostras do filtro de media movel.
* @param dev: ponteiro para a estrutura do ADC.
* @retval eSTATUS_OK se limpo, ou eSTATUS_INVALID_PARAM se ponteiro nulo.
* @details Reinicializa o buffer de media movel, util ao alternar canais
* analogicos ou reiniciar sequencias de operacao para evitar transientes.
*
* Exemplo de uso:
* @code
* BspAdc_ResetFilter(&sensorPressao);
* @endcode
******************************************************************************/
status_t BspAdc_ResetFilter(bspAdc_t *dev);

/******************************************************************************/
/** @brief Inicia a amostragem continua de dados analogicos via DMA.
* @param dev: ponteiro para a estrutura do ADC.
* @param buffer: ponteiro para o buffer de destino alinhado a 32 bytes (u16).
* @param length: quantidade total de amostras no buffer (ex: 256 ou 512).
* @retval eSTATUS_OK se disparado com sucesso, ou codigo de erro.
* @details Descarta a D-Cache (Invalidate) para a regiao do buffer e aciona
* HAL_ADC_Start_DMA. O hardware preenche o buffer continuamente na cadencia
* configurada pelo Timer no CubeMX sem gastar tempo de CPU.
*
* Exemplo de uso:
* @code
* dBSP_DMA_BUFFER_ALIGN u16 bufferAdcDma[512];
* BspAdc_StartContinuousDma(&sensorAdc, bufferAdcDma, 512);
* @endcode
******************************************************************************/
status_t BspAdc_StartContinuousDma(bspAdc_t *dev, u16 *buffer, u32 length);

/******************************************************************************/
/** @brief Encerra a amostragem continua por DMA.
* @param dev: ponteiro para a estrutura do ADC.
* @retval eSTATUS_OK se encerrado, ou codigo de erro.
* @details Invoca HAL_ADC_Stop_DMA desligando o canal e parando as conversoes.
*
* Exemplo de uso:
* @code
* BspAdc_StopContinuousDma(&sensorAdc);
* @endcode
******************************************************************************/
status_t BspAdc_StopContinuousDma(bspAdc_t *dev);

/******************************************************************************/
/** @brief Converte um bloco inteiro de amostras brutas para Volts de uma so vez.
* @param dev: ponteiro para a estrutura do ADC.
* @param rawBuffer: array com as amostras brutas de 12 bits preenchidas pelo DMA.
* @param voltageBuffer: array de saida onde serao gravados os valores em Volts.
* @param length: quantidade de amostras a converter.
* @retval eSTATUS_OK se convertido com sucesso, ou codigo de erro.
* @details Processa um lote de medicoes aplicando a escala de VRef em cada elemento,
* muito conveniente para processamento de sinais apos uma captura por DMA.
*
* Exemplo de uso:
* @code
* f32 tensoes[256];
* BspAdc_ConvertBufferToVoltages(&sensorAdc, bufferAdcDma, tensoes, 256);
* @endcode
******************************************************************************/
status_t BspAdc_ConvertBufferToVoltages(const bspAdc_t *dev, const u16 *rawBuffer, f32 *voltageBuffer, u32 length);

/******************************************************************************/
/** @brief Calcula a media aritmetica de um bloco de amostras lido pelo DMA.
* @param buffer: array com as amostras brutas.
* @param length: quantidade de amostras no array.
* @retval Valor medio calculado em contagens (u16).
* @details Soma todas as amostras do bloco com acumulador de 64 bits para evitar
* estouro de memoria e divide pelo total, proporcionando excelente rejeicao de ruido.
*
* Exemplo de uso:
* @code
* u16 mediaBloco = BspAdc_CalculateBufferAverage(bufferAdcDma, 512);
* @endcode
******************************************************************************/
u16 BspAdc_CalculateBufferAverage(const u16 *buffer, u32 length);

#endif /* _BSP_ADC_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */
