/******************************************************************************/
/**
* @file LibModel.h
* @addtogroup LIB_MODEL
* @{
******************************************************************************/
#ifndef _LIB_MODEL_H_
#define _LIB_MODEL_H_

/*******************************************************************************
* INCLUDES NECESSARIOS
******************************************************************************/
#include "BspTypes.h"

/*******************************************************************************
* CONFIGURACOES
******************************************************************************/
/// Exemplo de configuracao de habilitacao do modulo
#define dLIB_MODEL_ENABLE_FEATURE           dTRUE                               // [dTRUE ou dFALSE]

/// Exemplo de configuracao de tamanho de buffer
#define dLIB_MODEL_BUFFER_SIZE              64                                  // [bytes]

/*******************************************************************************
* DEFINES PUBLICOS
******************************************************************************/
/// Constante publica de identificacao
#define dLIB_MODEL_MAX_ITEMS                10

/*******************************************************************************
* TIPOS DE DADOS PUBLICOS
******************************************************************************/
/// Modos de operacao do modelo
typedef enum
{
    eLIB_MODEL_MODE_IDLE = 0,
    eLIB_MODEL_MODE_RUNNING,
    eLIB_MODEL_MODE_ERROR
} libModelMode_t;

/// Estrutura de dados para instancia do modulo
typedef struct
{
    libModelMode_t mode;
    u32 counter;
} libModel_t;

/*******************************************************************************
* PROTOTIPOS PUBLICOS
******************************************************************************/
/******************************************************************************/
/** @brief Inicializa a instancia do modelo.
* @param dev: ponteiro para a estrutura de dados do modulo.
* @retval eSTATUS_OK se sucesso, outro valor caso erro.
******************************************************************************/
status_t LibModel_Init(libModel_t *dev);

/******************************************************************************/
/** @brief Executa o processamento ciclico do modelo.
* @param dev: ponteiro para a estrutura de dados do modulo.
* @retval eSTATUS_OK se sucesso, outro valor caso erro.
******************************************************************************/
status_t LibModel_Process(libModel_t *dev);

#endif /* _LIB_MODEL_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */
