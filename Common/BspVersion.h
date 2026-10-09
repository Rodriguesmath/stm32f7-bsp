/******************************************************************************/
/**
* @file BspVersion.h
* @addtogroup BSP_VERSION
* @brief Estruturas e utilitarios para versionamento de bibliotecas e firmware.
* @author Rodrigues
* @{
******************************************************************************/
#ifndef _BSP_VERSION_H_
#define _BSP_VERSION_H_

/*******************************************************************************
* INCLUDES NECESSARIOS
******************************************************************************/
#include "BspTypes.h"

/*******************************************************************************
* CONFIGURACOES
******************************************************************************/

/*******************************************************************************
* DEFINES PUBLICOS
******************************************************************************/

/*******************************************************************************
* TIPOS DE DADOS PUBLICOS
******************************************************************************/
/// Estrutura para controle semantico de versao (Major.Minor.Patch)
typedef struct
{
    /// Versao principal (mudancas incompativeis de API)
    u8 major;
    /// Versao secundaria (novas funcionalidades compativeis)
    u8 minor;
    /// Versao de correcao (correcoes de bugs compativeis)
    u8 patch;
} version_t;

/*******************************************************************************
* PROTOTIPOS PUBLICOS
******************************************************************************/

#endif /* _BSP_VERSION_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */
