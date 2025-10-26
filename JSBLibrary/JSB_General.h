#pragma once

#include "stdint.h"
#include "JSB_GeneralOptions.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t min(uint32_t i_A, uint32_t i_B);
uint32_t max(uint32_t i_A, uint32_t i_B);
uint32_t clamp(uint32_t i_Value, uint32_t i_Min, uint32_t i_Max);

#if JSBGeneral_Sockets
#include "lwip/arch.h"
#endif

void JSB_ErrorHandler();

#if JSBGeneral_Sockets
int JSB_Sockets_Connect(const char *IP, u16_t Port, int SocketType);
#endif

#ifdef __cplusplus
}
#endif
