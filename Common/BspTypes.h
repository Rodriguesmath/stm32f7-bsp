/******************************************************************************/
/**
* @file BspTypes.h
* @addtogroup BSP_TYPES
* @brief Tipos primitivos e constantes fundamentais para firmware embarcado.
* @author Rodrigues
* @{
******************************************************************************/
#ifndef _BSP_TYPES_H_
#define _BSP_TYPES_H_

/*******************************************************************************
* INCLUDES NECESSARIOS
******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*******************************************************************************
* CONFIGURACOES
******************************************************************************/

/*******************************************************************************
* DEFINES PUBLICOS
******************************************************************************/
/// Constante de valor logico verdadeiro
#define dTRUE                               1

/// Constante de valor logico falso
#define dFALSE                              0

/// Definicao padronizada de ponteiro nulo
#ifndef dNULL
#define dNULL                               ((void *)0)
#endif

/*******************************************************************************
* TIPOS DE DADOS PUBLICOS
******************************************************************************/
/// Inteiro sem sinal de 8 bits
typedef uint8_t                             u8;

/// Inteiro sem sinal de 16 bits
typedef uint16_t                            u16;

/// Inteiro sem sinal de 32 bits
typedef uint32_t                            u32;

/// Inteiro sem sinal de 64 bits
typedef uint64_t                            u64;

/// Inteiro com sinal de 8 bits
typedef int8_t                              s8;

/// Inteiro com sinal de 16 bits
typedef int16_t                             s16;

/// Inteiro com sinal de 32 bits
typedef int32_t                             s32;

/// Inteiro com sinal de 64 bits
typedef int64_t                             s64;

/// Ponto flutuante de precisao simples (32 bits)
typedef float                               f32;

/// Ponto flutuante de precisao dupla (64 bits)
typedef double                              f64;

/// Status geral de operacao para funcoes de biblioteca
typedef enum
{
    eSTATUS_OK = 0,
    eSTATUS_ERROR,
    eSTATUS_BUSY,
    eSTATUS_TIMEOUT,
    eSTATUS_INVALID_PARAM
} status_t;

/*******************************************************************************
* PROTOTIPOS PUBLICOS
******************************************************************************/

#endif /* _BSP_TYPES_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */
