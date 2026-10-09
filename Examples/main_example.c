/******************************************************************************/
/**
* @file main_example.c
* @addtogroup EXAMPLES
* @brief Exemplo de aplicacao integrando BspUart, BspAdc e BspDac na Nucleo-F767ZI.
* @author Rodrigues
* @details
* \n <b>Ferramentas:</b>
* - STM32CubeIDE / GCC ARM.
*
* \n <b>Dependencias:</b>
* - Bsp.h
*
* \n <b>Observacoes:</b>
* - Exemplo demonstrativo de codigo limpo desacoplado do CubeMX.
* - Cumpre a secao 4.12 da norma de firmware Assert (ponto unico Bsp_Init).
*
* Changelog
* @version <b>1.0.0 - 08/10/2026</b> \n Rodrigues \n Versao inicial do exemplo de aplicacao.
*
* @copyright Generic STM32F7 BSP Library
* @{
******************************************************************************/
/*******************************************************************************
* INCLUDES
******************************************************************************/
#include "Bsp.h"

/*******************************************************************************
* DEFINES LOCAIS (fixos, apenas auxiliar para calculos)
******************************************************************************/
/// Intervalo de envio de telemetria serial em milissegundos
#define dTELEMETRY_INTERVAL_MS              500

/// Tamanho maximo do buffer de recepcao de linha de comando
#define dCOMMAND_LINE_BUFFER_SIZE           32

/*******************************************************************************
* CONSTANTES
******************************************************************************/

/*******************************************************************************
* ESTRUTURAS DE DADOS LOCAIS
******************************************************************************/
/// Dados de controle da aplicacao de exemplo
static struct
{
    /// Timestamp da ultima telemetria transmitida
    u32 lastTelemetryTime;
    /// Buffer para recepcao de comandos via UART
    char commandBuffer[dCOMMAND_LINE_BUFFER_SIZE];
} app;

/*******************************************************************************
* PROTOTIPOS LOCAIS
******************************************************************************/
static void App_ProcessCommands(bspUart_t *uart, bspDac_t *dac);

/*******************************************************************************
* FUNCOES PUBLICAS
******************************************************************************/
/******************************************************************************/
/** @brief Funcao principal da aplicacao.
* @param Nenhum.
* @retval int: retorno padrao (0).
******************************************************************************/
int main(void)
{
    // 1. Inicializacao unica de todo o hardware e drivers
    (void)Bsp_Init();

    // 2. Obtencao dos ponteiros dos drivers inicializados
    bspUart_t *uart = Bsp_GetConsoleUart();
    bspAdc_t  *adc  = Bsp_GetDefaultAdc();
    bspDac_t  *dac  = Bsp_GetDefaultDac();

    app.lastTelemetryTime = 0;

    BspUart_SendString(uart, "\r\n========================================\r\n");
    BspUart_SendString(uart, "  NUCLEO-F767ZI - BSP DEMO INICIADA   \r\n");
    BspUart_SendString(uart, "========================================\r\n");

    // Loop principal da aplicacao
    while(1)
    {
        u32 currentTime = Bsp_GetTick();

        // Tarefa 1: Transmissao periodica de telemetria analogica
        if((currentTime - app.lastTelemetryTime) >= dTELEMETRY_INTERVAL_MS)
        {
            app.lastTelemetryTime = currentTime;

            f32 tensaoLida = 0.0f;
            status_t status = BspAdc_ReadFilteredVoltage(adc, &tensaoLida);

            if(status == eSTATUS_OK)
            {
                // Replica a tensao medida na entrada ADC diretamente na saida DAC
                (void)BspDac_SetVoltage(dac, tensaoLida);

                // Imprime a telemetria formatada no terminal serial
                BspUart_Printf(uart, "[TELEMETRIA] ADC1: %.3f V | DAC1 ajustado: %.3f V\r\n",
                               tensaoLida, tensaoLida);
            }
        }

        // Tarefa 2: Processamento de comandos recebidos na serial
        App_ProcessCommands(uart, dac);

        // Pequeno atraso para aliviar o processador caso nao haja SO
        Bsp_DelayMs(10);
    }

    return 0;
}

/*******************************************************************************
* FUNCOES LOCAIS
******************************************************************************/
/******************************************************************************/
/** @brief Processa comandos de texto recebidos pela porta UART.
* @param uart: ponteiro para a instancia da UART.
* @param dac: ponteiro para a instancia do DAC.
* @retval Nenhum.
******************************************************************************/
static void App_ProcessCommands(bspUart_t *uart, bspDac_t *dac)
{
    u16 len = BspUart_ReadLine(uart, app.commandBuffer, sizeof(app.commandBuffer));

    if(len > 0)
    {
        BspUart_Printf(uart, "[COMANDO] Recebido: '%s'\r\n", app.commandBuffer);

        // Exemplo: se o comando for '0', zera o DAC; se for '1', coloca 3.3V
        if(app.commandBuffer[0] == '0')
        {
            (void)BspDac_SetVoltage(dac, 0.0f);
            BspUart_SendString(uart, "[DAC] Saida colocada em 0.0 V\r\n");
        }
        else if(app.commandBuffer[0] == '1')
        {
            (void)BspDac_SetVoltage(dac, 3.3f);
            BspUart_SendString(uart, "[DAC] Saida colocada em 3.3 V\r\n");
        }
    }
}

/** @} DOXYGEN GROUP TAG END OF FILE */
