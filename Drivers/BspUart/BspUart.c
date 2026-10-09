/******************************************************************************/
/**
* @file BspUart.c
* @addtogroup BSP_UART
* @brief Implementacao do driver de comunicacao serial de alto nivel para UART.
* @author Rodrigues
* @details
* \n <b>Ferramentas:</b>
* - STM32CubeIDE / GCC ARM.
*
* \n <b>Dependencias:</b>
* - BspUart.h
* - stm32f7xx_hal.h (HAL UART Module)
*
* \n <b>Observacoes:</b>
* - Compativel com qualquer USART/UART da familia STM32F7.
* - Recepcao com buffer circular para evitar perda de dados.
*
* Changelog
* @version <b>1.0.0 - 08/10/2026</b> \n Rodrigues \n Versao inicial com buffer circular e printf.
*
* @copyright Generic STM32F7 BSP Library
* @{
******************************************************************************/
/*******************************************************************************
* INCLUDES
******************************************************************************/
#include "BspUart.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/*******************************************************************************
* DEFINES LOCAIS (fixos, apenas auxiliar para calculos)
******************************************************************************/
// Verificacao de integridade das configuracoes do header
#if (dBSP_UART_RX_BUFFER_SIZE < 8)
#error "dBSP_UART_RX_BUFFER_SIZE deve ser de no minimo 8 bytes."
#endif

#if (dBSP_UART_DEFAULT_TIMEOUT_MS == 0)
#error "dBSP_UART_DEFAULT_TIMEOUT_MS deve ser maior do que zero."
#endif

/// Tamanho do buffer temporario para impressao formatada via BspUart_Printf
#define dPRINTF_TEMP_BUFFER_SIZE            128

/*******************************************************************************
* CONSTANTES
******************************************************************************/

/*******************************************************************************
* ESTRUTURAS DE DADOS LOCAIS
******************************************************************************/
/// Variaveis internas de rastreamento do modulo serial
static struct
{
    /// Total de bytes transmitidos por todas as portas
    u32 totalBytesTransmitted;
    /// Total de bytes recebidos por todas as portas
    u32 totalBytesReceived;
} bspUart;

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

__attribute__((weak)) HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    (void)huart;
    (void)pData;
    (void)Size;
    (void)Timeout;
    return HAL_OK;
}
#endif

static status_t BspUart_MapHalStatus(HAL_StatusTypeDef halStatus);

/*******************************************************************************
* FUNCOES PUBLICAS
******************************************************************************/
/******************************************************************************/
/** @brief Inicializa a instancia da UART vinculando ao handle do CubeMX.
* @param dev: ponteiro para a estrutura de dados da UART.
* @param huart: ponteiro para o handle HAL gerado pelo CubeMX.
* @retval eSTATUS_OK se sucesso, ou eSTATUS_INVALID_PARAM se ponteiro nulo.
******************************************************************************/
status_t BspUart_Init(bspUart_t *dev, UART_HandleTypeDef *huart)
{
    status_t ret = eSTATUS_OK;

    if((dev == dNULL) || (huart == dNULL))
    {
        ret = eSTATUS_INVALID_PARAM;
    }
    else
    {
        dev->huart = huart;
        dev->rxHead = 0;
        dev->rxTail = 0;
        dev->rxCount = 0;
        dev->isInitialized = true;

        (void)memset(dev->rxBuffer, 0, sizeof(dev->rxBuffer));
    }

    return ret;
}

/******************************************************************************/
/** @brief Envia um unico byte pela UART.
* @param dev: ponteiro para a estrutura da UART.
* @param byte: valor de 8 bits a ser transmitido.
* @retval eSTATUS_OK se transmitido, eSTATUS_TIMEOUT ou eSTATUS_ERROR caso falha.
******************************************************************************/
status_t BspUart_SendByte(bspUart_t *dev, u8 byte)
{
    status_t ret = eSTATUS_OK;

    if((dev == dNULL) || (dev->isInitialized == false))
    {
        ret = eSTATUS_INVALID_PARAM;
    }
    else
    {
        HAL_StatusTypeDef halStatus = HAL_UART_Transmit(dev->huart, &byte, 1, dBSP_UART_DEFAULT_TIMEOUT_MS);
        ret = BspUart_MapHalStatus(halStatus);

        if(ret == eSTATUS_OK)
        {
            bspUart.totalBytesTransmitted++;
        }
    }

    return ret;
}

/******************************************************************************/
/** @brief Transmite um buffer de bytes pela UART.
* @param dev: ponteiro para a estrutura da UART.
* @param buffer: ponteiro para o array de dados a serem enviados.
* @param size: quantidade de bytes a transmitir.
* @retval eSTATUS_OK se sucesso, eSTATUS_TIMEOUT ou eSTATUS_INVALID_PARAM se erro.
******************************************************************************/
status_t BspUart_SendBuffer(bspUart_t *dev, const u8 *buffer, u16 size)
{
    status_t ret = eSTATUS_OK;

    if((dev == dNULL) || (buffer == dNULL) || (size == 0) || (dev->isInitialized == false))
    {
        ret = eSTATUS_INVALID_PARAM;
    }
    else
    {
        HAL_StatusTypeDef halStatus = HAL_UART_Transmit(dev->huart, (uint8_t *)buffer, size, dBSP_UART_DEFAULT_TIMEOUT_MS);
        ret = BspUart_MapHalStatus(halStatus);

        if(ret == eSTATUS_OK)
        {
            bspUart.totalBytesTransmitted += size;
        }
    }

    return ret;
}

/******************************************************************************/
/** @brief Envia uma string terminada em zero (null-terminated).
* @param dev: ponteiro para a estrutura da UART.
* @param str: string de texto constante a ser enviada.
* @retval eSTATUS_OK se enviada com sucesso, ou codigo de erro.
******************************************************************************/
status_t BspUart_SendString(bspUart_t *dev, const char *str)
{
    status_t ret = eSTATUS_OK;

    if((dev == dNULL) || (str == dNULL))
    {
        ret = eSTATUS_INVALID_PARAM;
    }
    else
    {
        u16 len = (u16)strlen(str);
        ret = BspUart_SendBuffer(dev, (const u8 *)str, len);
    }

    return ret;
}

#if dBSP_UART_ENABLE_PRINTF == dTRUE
/******************************************************************************/
/** @brief Imprime texto formatado na UART similar a printf.
* @param dev: ponteiro para a estrutura da UART.
* @param format: string de formato com especificadores.
* @param ...: lista de argumentos variaveis.
* @retval eSTATUS_OK se enviado com sucesso, ou eSTATUS_ERROR se formatacao falhar.
******************************************************************************/
status_t BspUart_Printf(bspUart_t *dev, const char *format, ...)
{
    status_t ret = eSTATUS_OK;

    if((dev == dNULL) || (format == dNULL))
    {
        ret = eSTATUS_INVALID_PARAM;
    }
    else
    {
        char tempBuffer[dPRINTF_TEMP_BUFFER_SIZE];
        va_list args;
        va_start(args, format);

        int written = vsnprintf(tempBuffer, sizeof(tempBuffer), format, args);
        va_end(args);

        if(written > 0)
        {
            u16 sizeToSend = (written < (int)sizeof(tempBuffer)) ? (u16)written : (u16)(sizeof(tempBuffer) - 1);
            ret = BspUart_SendBuffer(dev, (const u8 *)tempBuffer, sizeToSend);
        }
        else
        {
            ret = eSTATUS_ERROR;
        }
    }

    return ret;
}
#endif

/******************************************************************************/
/** @brief Retorna a quantidade de bytes disponiveis para leitura no buffer.
* @param dev: ponteiro para a estrutura da UART.
* @retval Quantidade de bytes acumulados no buffer de recepcao.
******************************************************************************/
u16 BspUart_Available(const bspUart_t *dev)
{
    u16 count = 0;

    if(dev != dNULL)
    {
        count = dev->rxCount;
    }

    return count;
}

/******************************************************************************/
/** @brief Le o proximo byte disponivel no buffer de recepcao.
* @param dev: ponteiro para a estrutura da UART.
* @param byte: ponteiro onde sera armazenado o byte lido.
* @retval eSTATUS_OK se o byte foi lido, eSTATUS_TIMEOUT se o buffer estiver vazio.
******************************************************************************/
status_t BspUart_ReceiveByte(bspUart_t *dev, u8 *byte)
{
    status_t ret = eSTATUS_OK;

    if((dev == dNULL) || (byte == dNULL) || (dev->isInitialized == false))
    {
        ret = eSTATUS_INVALID_PARAM;
    }
    else if(dev->rxCount == 0)
    {
        ret = eSTATUS_TIMEOUT;
    }
    else
    {
        *byte = dev->rxBuffer[dev->rxTail];
        dev->rxTail = (dev->rxTail + 1) % dBSP_UART_RX_BUFFER_SIZE;
        dev->rxCount--;
    }

    return ret;
}

/******************************************************************************/
/** @brief Le uma linha completa de texto ate encontrar '\n' ou '\r'.
* @param dev: ponteiro para a estrutura da UART.
* @param buffer: destino onde a linha sera copiada terminada em '\0'.
* @param maxLen: tamanho maximo suportado pelo buffer de destino.
* @retval Quantidade de caracteres gravados na linha (excluindo terminador nulo).
******************************************************************************/
u16 BspUart_ReadLine(bspUart_t *dev, char *buffer, u16 maxLen)
{
    u16 bytesRead = 0;

    if((dev == dNULL) || (buffer == dNULL) || (maxLen == 0) || (dev->isInitialized == false))
    {
        bytesRead = 0;
    }
    else
    {
        // Verifica primeiro se existe um terminador de linha no buffer disponivel
        bool lineComplete = false;
        u16 searchTail = dev->rxTail;
        u16 i = 0;

        for(i = 0; i < dev->rxCount; i++)
        {
            u8 c = dev->rxBuffer[searchTail];
            if((c == '\n') || (c == '\r'))
            {
                lineComplete = true;
                break;
            }
            searchTail = (searchTail + 1) % dBSP_UART_RX_BUFFER_SIZE;
        }

        if(lineComplete == true)
        {
            u8 byte = 0;
            while((BspUart_ReceiveByte(dev, &byte) == eSTATUS_OK) && (bytesRead < (maxLen - 1)))
            {
                if((byte == '\r') || (byte == '\n'))
                {
                    // Descarte de eventuais pares \r\n subsequentes imediatos
                    if(dev->rxCount > 0)
                    {
                        u8 nextChar = dev->rxBuffer[dev->rxTail];
                        if(((byte == '\r') && (nextChar == '\n')) || ((byte == '\n') && (nextChar == '\r')))
                        {
                            u8 discard = 0;
                            (void)BspUart_ReceiveByte(dev, &discard);
                        }
                    }
                    break;
                }

                buffer[bytesRead] = (char)byte;
                bytesRead++;
            }

            buffer[bytesRead] = '\0';
        }
    }

    return bytesRead;
}

/******************************************************************************/
/** @brief Manipulador de interrupcao de recepcao a ser chamado no callback HAL.
* @param dev: ponteiro para a estrutura da UART correspondente.
* @param byte: byte recebido da UART pelo hardware.
* @retval Nenhum.
******************************************************************************/
void BspUart_RxInterruptHandler(bspUart_t *dev, u8 byte)
{
    if((dev != dNULL) && (dev->isInitialized == true))
    {
        dev->rxBuffer[dev->rxHead] = byte;
        dev->rxHead = (dev->rxHead + 1) % dBSP_UART_RX_BUFFER_SIZE;

        if(dev->rxCount < dBSP_UART_RX_BUFFER_SIZE)
        {
            dev->rxCount++;
        }
        else
        {
            // Buffer cheio: o byte mais antigo (tail) e avancado para nao desalinhar
            dev->rxTail = (dev->rxTail + 1) % dBSP_UART_RX_BUFFER_SIZE;
        }

        bspUart.totalBytesReceived++;
    }
}

/*******************************************************************************
* FUNCOES LOCAIS
******************************************************************************/
/******************************************************************************/
/** @brief Mapeia o status de retorno da HAL para a tipagem status_t do BSP.
* @param halStatus: codigo de status retornado pela biblioteca HAL.
* @retval status_t mapeado equivalente.
******************************************************************************/
static status_t BspUart_MapHalStatus(HAL_StatusTypeDef halStatus)
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
