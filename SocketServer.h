#pragma once

#define _CRT_SECURE_NO_WARNINGS         /* Para evitar warnings sobre funçoes seguras de */
/*   manipulacao de strings */
#define _WINSOCK_DEPRECATED_NO_WARNINGS /* Para evitar warnings de funções WINSOCK depreciadas */

#include <winsock2.h>
#include <stdio.h>
#include <conio.h>

#include "Orchestrator.h"

#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H

#define WHITE   FOREGROUND_RED   | FOREGROUND_GREEN     | FOREGROUND_BLUE
#define HLGREEN FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define HLRED   FOREGROUND_RED   | FOREGROUND_INTENSITY
#define HLBLUE  FOREGROUND_BLUE  | FOREGROUND_INTENSITY
#define MAGENTA FOREGROUND_RED   | FOREGROUND_BLUE | FOREGROUND_INTENSITY
#define YELLOW  FOREGROUND_RED   | FOREGROUND_GREEN

#define TAMMSGREQ     9  // 2 caracteres + 6 caracteres + 1 separador
#define TAMMSGDADOS  54  // 2 caracteres + 6 caracteres + 3 caracteres + 10 caracteres +
						 // + 6 caracteres + 2 caracteres + 19 caracteres + 6 separadores
#define TAMMSGPAR    22  // 2 + 6 + 3 + 2 + 5 caracteres + 4 separadores
#define TAMMSGACK     9  // 2 caracteres + 6 caracteres + 1 separador
#define TAMindicatorIdentifier 3 // 3 caracteres 
#define TAMindicatorType 2 // 2 caracteres 
#define TAMindicatorState 5 // 5 caracteres, pois "TRUE " vem com espaco 

#define ESC		   0x1B

#define IP_ADDR_LENGTH 15 //12 números no máximo, mais 3 pontos
#define SERVER_ADDRESS_STR "127.0.0.1" //servidor tem endereco fixo. loopback para testes
#define SERVER_PORT 3889

#pragma warning(push)
#pragma warning(disable:6031)
#pragma warning(disable:6385)

/*****************************************************************************************/

DWORD WINAPI SocketServer(LPVOID dataForThreads);

#endif SOCKET_SERVER_H