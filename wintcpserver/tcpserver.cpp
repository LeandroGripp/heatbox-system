
#define _CRT_SECURE_NO_WARNINGS         /* Para evitar warnings sobre funçoes seguras de */
/*   manipulacao de strings */
#define _WINSOCK_DEPRECATED_NO_WARNINGS /* Para evitar warnings de funções WINSOCK depreciadas */

#include <winsock2.h>
#include <stdio.h>
#include <conio.h>

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

#define ESC		   0x1B

# define IP_ADDR_LENGTH 15 //12 números no máximo, mais 3 pontos
#define SERVER_ADDRESS_STR "127.0.0.1" //servidor tem endereco fixo. loopback para testes
#define SERVER_PORT 2222

#pragma warning(push)
#pragma warning(disable:6031)
#pragma warning(disable:6385)

/*****************************************************************************************/

int main()
{	
	WSADATA       wsaData;
	SOCKET        s, new_s;
	SOCKADDR_IN   ServerAddr;

	char msgreq[TAMMSGREQ + 1]; //Centro de operações solicita dados
	char msgack[TAMMSGACK + 1]; //ACK do centro de operações
	char msgackclp[TAMMSGACK + 1] = "44|000000"; //ACK do clp
	char msgpar[TAMMSGPAR + 1]; //Centro de operações envia parâmetros
	char msgdados[TAMMSGDADOS + 1] = "09|000000|015|PTN-034234|0115.3|02|27/04/2023 10:12:33";//Hotbox envia dados sobre a linha ferroviária

	char buf[100];
	char msgcode[3];
	char ipaddr[IP_ADDR_LENGTH + 1] = SERVER_ADDRESS_STR;
	int port = SERVER_PORT;
	int status, tecla = 0;
	int nseql, nseqr;

	HANDLE hOut;

	/*********************************************************************************************/

	status = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (status != 0) {
		printf("Falha na inicializacao do Winsock 2! Erro  = %d\n", WSAGetLastError());
		WSACleanup();
		exit(0);
	}

	// Inicializa a estrutura SOCKADDR_IN que será utilizada para
	// a conexão ao cliente.
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(port);
	ServerAddr.sin_addr.s_addr = inet_addr(ipaddr);

	// Obtém um handle para a saída da console, para mudar as cores
	hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE)
		printf("Erro ao obter handle para a saida da console\n");

	// ==================================================================
   // LOOP PRINCIPAL - CRIA SOCKET E AGUARDA CONEXÃO DO CLIENTE
   // ==================================================================

	while (TRUE) {
		//printf("LOOP PRINCIPAL\n");
		SetConsoleTextAttribute(hOut, WHITE);
		nseql = 0; // Reinicia numeração das mensagens

		// Cria um novo socket para estabelecer a conexão.
		s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (s == INVALID_SOCKET) {
			SetConsoleTextAttribute(hOut, WHITE);
			status = WSAGetLastError();
			if (status == WSAENETDOWN)
				printf("Rede inacessivel!\n");
			else
				printf("Falha na chamada da funcao socket: codigo de erro = %d\n", status);
			SetConsoleTextAttribute(hOut, WHITE);
			WSACleanup();
			exit(0);
		}

		// Cria o socket passivo e aguarda a conexão
		printf("INICIANDO O SERVIDOR TCP...\n");
		
		status = bind(s, (SOCKADDR *)&ServerAddr, sizeof(ServerAddr));
		if (status == SOCKET_ERROR) {
			SetConsoleTextAttribute(hOut, HLRED);
			printf("Falha na chamada da funcao bind ! Erro  = %d\n", WSAGetLastError());
			SetConsoleTextAttribute(hOut, WHITE);
			WSACleanup();
			closesocket(s);
			exit(0);
		}

		status = listen(s, 5);
		if (status == SOCKET_ERROR) {
			SetConsoleTextAttribute(hOut, HLRED);
			printf("Falha na chamada da funcao listen ! Erro  = %d\n", WSAGetLastError());
			SetConsoleTextAttribute(hOut, WHITE);
			WSACleanup();
			closesocket(s);
			exit(0);
		}

		new_s = accept(s, NULL, NULL); //endereço do cliente ignorado
		if (new_s == INVALID_SOCKET) {
			SetConsoleTextAttribute(hOut, HLRED);
			printf("Falha na chamada da funcao accept ! Erro  = %d\n", WSAGetLastError());
			SetConsoleTextAttribute(hOut, WHITE);
			WSACleanup();
			closesocket(s);
			closesocket(new_s);
			exit(0);
		}

		// ----------------------------------------------------------------------
		// LOOP SECUNDARIO - Troca de mensagens com o sistema de controle,
		// até que o usuário digite ESC.
		// ----------------------------------------------------------------------

		for (;;) {
			
			// Testa se usuário digitou ESC, em caso afirmativo, encerra o programa
			if (_kbhit() != 0)
				if ((tecla = _getch()) == ESC) break;

			// Aguarda o comando do Centro de Operações
			memset(buf, sizeof(buf), 0);
			status = recv(new_s, buf, sizeof(buf), 0);
			
			if (status == TAMMSGPAR || status == TAMMSGREQ) {

				sscanf(&buf[3], "%6d", &nseqr);
				if (++nseql != nseqr) {
					SetConsoleTextAttribute(hOut, HLRED);
					printf("Numero sequencial de mensagem incorreto [1]: recebido %d em vez de %d.\n",
						nseqr, nseql);
					printf("Encerrando o programa...\n");
					closesocket(s);
					closesocket(new_s);
					SetConsoleTextAttribute(hOut, WHITE);
					WSACleanup();
					exit(0);
				}

				// mensagem de parâmetros de sinalização
				if (status == TAMMSGPAR) {
					/* verifica código da msg*/
					if (strncmp(buf, "00", 2) != 0) {
						strncpy(msgcode, buf, 2);
						msgcode[2] = 0x00;
						SetConsoleTextAttribute(hOut, HLRED);
						printf("Codigo incorreto de mensagem de comandos de sinalizacao (parametros): recebido '%s' em vez de '00'\n", msgcode);
						printf("Encerrando o programa...\n");
						closesocket(s);
						closesocket(new_s);
						WSACleanup();
						SetConsoleTextAttribute(hOut, WHITE);
						exit(0);
					}
					/* recebe a mensagem de comandos de sinalização*/
					strncpy(msgpar, buf, TAMMSGPAR);
					msgpar[TAMMSGPAR] = 0x00;
					SetConsoleTextAttribute(hOut, MAGENTA);
					printf("Mensagem de comandos recebida do Centro de Operacoes:\n%s\n\n",
						msgpar);

					/* Envia ACK ao Centro de Operações */
					sprintf(buf, "%06d", ++nseql);
					memcpy(&msgackclp[3], buf, 6);
					status = send(new_s, msgackclp, TAMMSGACK, 0);
					if (status == TAMMSGACK) {
						msgackclp[TAMMSGACK] = 0x00;
						SetConsoleTextAttribute(hOut, MAGENTA);
						printf("Mensagem de ACK enviada ao Centro de Operacoes [%s]:\n%s\n\n",
							ipaddr, msgackclp);
					}
					else {
						SetConsoleTextAttribute(hOut, HLRED);
						if (status == 0)
							// Esta situação NÃO deve ocorrer, e indica algum tipo de erro.
							printf("SEND (3) retornou 0 bytes ... Falha!\n");
						else if (status == SOCKET_ERROR) {
							status = WSAGetLastError();
							printf("Falha no SEND (3): codigo de erro = %d\n", status);
						}
						printf("Tentando reconexao ..\n\n");
						break;
					}
				}
				//mensagem de requisição de dados
				else if (status == TAMMSGREQ) {
					/* verifica código da msg*/
					if (strncmp(buf, "99", 2) != 0) {
						strncpy(msgcode, buf, 2);
						msgcode[2] = 0x00;
						SetConsoleTextAttribute(hOut, HLRED);
						printf("Codigo incorreto de mensagem de requisicao de dados recebido '%s' em vez de '99'\n", msgcode);
						printf("Encerrando o programa...\n");
						closesocket(s);
						closesocket(new_s);
						WSACleanup();
						SetConsoleTextAttribute(hOut, WHITE);
						exit(0);
					}

					/* recebe a mensagem de comandos de sinalização*/
					strncpy(msgreq, buf, TAMMSGREQ);
					msgreq[TAMMSGREQ] = 0x00;
					SetConsoleTextAttribute(hOut, HLGREEN);
					printf("Mensagem de requisicao de dados recebida do Centro de Operacoes:\n%s\n\n",
						msgreq);

					/* envio de dados para o Centro de Operações*/
					sprintf(buf, "%06d", ++nseql);
					memcpy(&msgdados[3], buf, 6);
					status = send(new_s, msgdados, TAMMSGDADOS, 0);
					if (status == TAMMSGDADOS) {
						msgdados[TAMMSGDADOS] = 0x00;
						SetConsoleTextAttribute(hOut, YELLOW);
						printf("Mensagem de dados enviada ao Centro de Operacoes:\n%s\n\n",
							msgdados);
					}
					else {
						SetConsoleTextAttribute(hOut, HLRED);
						if (status == 0)
							// Esta situação NÃO deve ocorrer, e indica algum tipo de erro.
							printf("SEND (3) retornou 0 bytes ... Falha!\n");
						else if (status == SOCKET_ERROR) {
							status = WSAGetLastError();
							printf("Falha no SEND (3): codigo de erro = %d\n", status);
						}
						printf("Tentando reconexao ..\n\n");
						break;
					}

					/* aguarda o ACK do Centro de Operações*/
					memset(buf, sizeof(buf), 0);
					status = recv(new_s, buf, TAMMSGACK, 0);
					if (status == TAMMSGACK) {
						if (strncmp(buf, "33", 2) != 0) {
							strncpy(msgcode, buf, 2);
							msgcode[2] = 0x00;
							SetConsoleTextAttribute(hOut, HLRED);
							printf("Codigo incorreto de mensagem de ACK: recebido '%s' em vez de '33'\n", msgcode);
							printf("Encerrando o programa...\n");
							closesocket(s);
							closesocket(new_s);
							WSACleanup();
							SetConsoleTextAttribute(hOut, WHITE);
							exit(0);
						}
						sscanf(&buf[3], "%6d", &nseqr);
						if (++nseql != nseqr) {
							SetConsoleTextAttribute(hOut, HLRED);
							printf("Numero sequencial de mensagem incorreto [1]: recebido %d em vez de %d.\n",
								nseqr, nseql);
							printf("Encerrando o programa...\n");
							closesocket(s);
							closesocket(new_s);
							WSACleanup();
							SetConsoleTextAttribute(hOut, WHITE);
							exit(0);
						}
						strncpy(msgack, buf, TAMMSGACK);
						msgack[TAMMSGACK] = 0x00;
						SetConsoleTextAttribute(hOut, HLBLUE);
						printf("Mensagem de ACK recebida do Centro de Operacoes :\n%s\n\n", msgack);
					}
					else {
						SetConsoleTextAttribute(hOut, HLRED);
						if (status == 0)
							// Esta situação NÃO deve ocorrer, e indica algum tipo de erro.
							printf("RECV (4) retornou 0 bytes ... Falha!\n");
						else if (status == SOCKET_ERROR) {
							status = WSAGetLastError();
							if (status == WSAETIMEDOUT) printf("Timeout no RECV (4)!\n");
							else printf("Erro no RECV (4)! codigo de erro = %d\n", status);
						}
						printf("Tentando reconexao ..\n\n");
						break;
					}

				}
			}
			//numero de bytes não corresponde a nenhuma mensagem
			else {
				SetConsoleTextAttribute(hOut, HLRED);
				if (status == 0)
					// Esta situação NÃO deve ocorrer, e indica algum tipo de erro.
					printf("RECV (2) retornou 0 bytes ... Falha!\n");
				else if (status == SOCKET_ERROR) {
					status = WSAGetLastError();
					if (status == WSAETIMEDOUT)
						printf("Timeout no RECV (2)!\n");
					else printf("Erro no RECV (2)! codigo de erro = %d\n", status);
				}
				else {
					printf("RECV (2) retornou um número invalido de bytes. \n");
				}
				printf("Tentando reconexao...\n\n");
				break;
			}
		


		} // FIM DO LOOP SECUNDARIO

		if (tecla == ESC) break;
		else {
			closesocket(s);
			closesocket(new_s);
		}

	} //FIM DO LOOP PRINCIPAL

	// Fim do enlace: fecha o "socket" e encerra o ambiente Winsock 2.
	SetConsoleTextAttribute(hOut, WHITE);
	printf("Encerrando o servidor ...\n");
	closesocket(s);
	closesocket(new_s);
	WSACleanup();
	printf("Servidor encerrado. Fim do programa.\n");
	exit(0);

}
#pragma warning(pop)
