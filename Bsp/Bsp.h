/******************************************************************************/
/**
* @file Bsp.h
* @addtogroup BSP
* @{
******************************************************************************/
#ifndef _BSP_H_
#define _BSP_H_

/*******************************************************************************
* INCLUDES NECESSARIOS
******************************************************************************/
#include "BspTypes.h"
#include "BspUart.h"
#include "BspAdc.h"
#include "BspDac.h"

/*******************************************************************************
* CONFIGURACOES
******************************************************************************/
/// Tensao de referencia analogica da placa Nucleo-F767ZI em milivolts
#define dBSP_SYSTEM_VREF_MV                 3300                                // [mV]

/// Tensao de referencia analogica em Volts (calculada)
#define dBSP_SYSTEM_VREF                    ((f32)dBSP_SYSTEM_VREF_MV / 1000.0f)// [Volts]

/*******************************************************************************
* DEFINES PUBLICOS
******************************************************************************/

/*******************************************************************************
* TIPOS DE DADOS PUBLICOS
******************************************************************************/

/*******************************************************************************
* PROTOTIPOS PUBLICOS
******************************************************************************/
/******************************************************************************/
/** @brief Ponto unico de inicializacao de todo o hardware do microcontrolador.
* @param Nenhum.
* @retval eSTATUS_OK se todo o hardware foi inicializado com sucesso, outro codigo caso erro.
* @details Conforme a secao 4.12 da norma Assert, esta funcao deve ser a unica
* interface da aplicacao (main.c) com a camada de baixo nivel. Ela invoca a
* inicializacao da HAL, dos clocks do sistema, dos perifericos gerados pelo
* CubeMX e inicializa os drivers BspUart, BspAdc e BspDac.
*
* Exemplo de uso:
* @code
* int main(void)
* {
*     Bsp_Init();
*     while(1)
*     {
*         // Codigo da aplicacao limpo e desacoplado
*     }
* }
* @endcode
******************************************************************************/
status_t Bsp_Init(void);

/******************************************************************************/
/** @brief Retorna o ponteiro para a instancia da UART de console da placa.
* @param Nenhum.
* @retval Ponteiro para a estrutura bspUart_t do console serial.
* @details Permite que qualquer modulo da aplicacao acesse a porta serial de
* comunicacao sem necessidade de expor variaveis globais ou handles da HAL.
*
* Exemplo de uso:
* @code
* bspUart_t *uart = Bsp_GetConsoleUart();
* BspUart_SendString(uart, "Ola Mundo!\r\n");
* @endcode
******************************************************************************/
bspUart_t *Bsp_GetConsoleUart(void);

/******************************************************************************/
/** @brief Retorna a instancia padrao do conversor analogico (ADC1).
* @param Nenhum.
* @retval Ponteiro para a estrutura bspAdc_t inicializada.
* @details Disponibiliza o canal analogico principal ja calibrado com filtro
* de media movel pronto para amostragem.
*
* Exemplo de uso:
* @code
* bspAdc_t *adc = Bsp_GetDefaultAdc();
* f32 tensao = 0.0f;
* BspAdc_ReadFilteredVoltage(adc, &tensao);
* @endcode
******************************************************************************/
bspAdc_t *Bsp_GetDefaultAdc(void);

/******************************************************************************/
/** @brief Retorna a instancia padrao da saida analogica (DAC1 canal 1).
* @param Nenhum.
* @retval Ponteiro para a estrutura bspDac_t inicializada.
* @details Disponibiliza o canal de saida analogica pronto para escrita de
* niveis de tensao ou formas de onda.
*
* Exemplo de uso:
* @code
* bspDac_t *dac = Bsp_GetDefaultDac();
* BspDac_SetVoltage(dac, 2.0f);
* @endcode
******************************************************************************/
bspDac_t *Bsp_GetDefaultDac(void);

/******************************************************************************/
/** @brief Executa atraso bloqueante em milissegundos sem acoplamento direto a HAL.
* @param ms: tempo de atraso em milissegundos.
* @retval Nenhum.
* @details Encapsula a chamada HAL_Delay para que a aplicacao nao precise
* incluir os arquivos nativos da ST diretamente.
*
* Exemplo de uso:
* @code
* Bsp_DelayMs(500);
* @endcode
******************************************************************************/
void Bsp_DelayMs(u32 ms);

/******************************************************************************/
/** @brief Retorna o contador de tempo atual do sistema em milissegundos.
* @param Nenhum.
* @retval Valor atual da base de tempo em milissegundos (SysTick).
* @details Encapsula HAL_GetTick para controle de temporizacao nao-bloqueante
* na camada de aplicacao.
*
* Exemplo de uso:
* @code
* u32 tempoAnterior = Bsp_GetTick();
* if((Bsp_GetTick() - tempoAnterior) > 1000)
* {
*     // Passou 1 segundo
* }
* @endcode
******************************************************************************/
u32 Bsp_GetTick(void);

#endif /* _BSP_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */
