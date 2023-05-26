#include "SocketServer.h"

DWORD WINAPI SocketServer (LPVOID dataForThreads) {
	WSADATA       wsaData;
	SOCKET        s, new_s;
	SOCKADDR_IN   ServerAddr;

	char msgreq[TAMMSGREQ + 1]; //Centro de operações solicita dados
	char msgack[TAMMSGACK + 1]; //ACK do centro de operações
	char msgackclp[TAMMSGACK + 1] = "44|000000"; //ACK do clp
	char msgpar[TAMMSGPAR + 1]; //Centro de operações envia parâmetros
	char msgdados[TAMMSGDADOS + 1];//Hotbox envia dados sobre a linha ferroviária

	char buf[100];
	char msgcode[3];
	char ipaddr[IP_ADDR_LENGTH + 1] = SERVER_ADDRESS_STR;
	int port = SERVER_PORT;
	int status, tecla = 0;
	int nseql, nseqr;

	HANDLE hOut;

	DataForThreads *data;
	data = (DataForThreads*)dataForThreads;

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

	// Obtém um handle para a saida da console, para mudar as cores
	hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE)
		printf("Erro ao obter handle para a saida da console\n");

	// ==================================================================
   // LOOP PRINCIPAL - CRIA SOCKET E AGUARDA CONEXÃO DO CLIENTE
   // ==================================================================

	while (TRUE) {
		
		SetConsoleTextAttribute(hOut, WHITE);
		nseql = 0; // Reinicia numeração das mensagens

		// Cria um novo socket para estabelecer a conexão.
		s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (s == INVALID_SOCKET) {
			SetConsoleTextAttribute(hOut, HLRED);
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

			// Aguarda o comando do Sistema de Planejamento de Operações
			memset(buf, sizeof(buf), 0);
			status = recv(new_s, buf, sizeof(buf), 0);

			if (status == TAMMSGPAR || status == TAMMSGREQ) { //se recebeu um dos dois tipos de mensagens validas

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

					//-------------- Trecho com memoria compartilhada ---------------------------
					
					//solicita a escrita dos dados ao cliente OPC
					WaitForSingleObject(data->paramsMutex, INFINITE);
					data->paramsMutexOwner = SOCKET_SERVER;

					//escreve na memoria compartilhada
					strncpy(buf, &msgpar[10], TAMindicatorIdentifier);
					buf[TAMindicatorIdentifier] = 0x00;
					data->hotboxParams.indicatorIdentifier = atoi(buf);
					
					strncpy(buf, &msgpar[14], TAMindicatorType);
					buf[TAMindicatorType] = 0x00;
					data->hotboxParams.indicatorType = atoi(buf);

					strncpy(buf, &msgpar[10], TAMindicatorState);
					buf[TAMindicatorState] = 0x00;
					if (buf == "TRUE ") {
						data->hotboxParams.indicatorState = true;
					}
					else { // nao verifica se e realmente FALSE
						data->hotboxParams.indicatorState = false;
					}

					data->paramsMutexOwner = NO_OWNER;
					ReleaseMutex(data->paramsMutex);

					//avisa o cliente OPC que houve uma solicitacao de escrita
					data->writeReqState = WRITE_REQUESTED;
					while (data->writeReqState != WRITE_FINISHED);

					data->writeReqState = NO_REQ;

					//--------------------- Fim do trecho com memoria compartilhada ------------------------------

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

					/* recebe a mensagem de requisicao de dados*/
					strncpy(msgreq, buf, TAMMSGREQ);
					msgreq[TAMMSGREQ] = 0x00;
					SetConsoleTextAttribute(hOut, HLGREEN);
					printf("Mensagem de requisicao de dados recebida do Centro de Operacoes:\n%s\n\n",
						msgreq);

					/* envio de dados para o Centro de Operações*/

					//--------------------- Trecho com memoria compartilhada ------------------------------

					WaitForSingleObject(data->dataMutex, INFINITE);
					data->dataMutexOwner = SOCKET_SERVER;
					
					//monta a mensagem a ser enviada
					char strToPrint[100];
					strncpy(strToPrint, data->hotboxData.railwayComposition, 10);
					strToPrint[10] = 0;

					float floatToPrint = ((int)data->hotboxData.temperature) % 10000 + (data->hotboxData.temperature - ((int)data->hotboxData.temperature));

					sprintf(buf, "09|%06d|%03d|%-10s|%06.1f|%02d|%s",
						1 % 1000000,
						data->hotboxData.hotboxIdentifier % 1000,
						strToPrint,
						floatToPrint,
						data->hotboxData.alarm % 100,
						data->hotboxData.datetime);
					memcpy(msgdados, buf, TAMMSGDADOS);
					
					sprintf(buf, "%06d", ++nseql);
					memcpy(&msgdados[3], buf, 6);

					data->dataMutexOwner = NO_OWNER;
					ReleaseMutex(data->dataMutex);

					//--------------------- Fim do trecho com memoria compartilhada ------------------------------

					//envio de fato
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



/* Notas do Leandro*/

// Setup the server to listen.
// Bind the server to the right port.
// Create the accept connections loop.
// Accept each new connection by running a new thread. The function to be run is described below:
	// Infinite loop receiving messages.
	// When message received, create a new thread to deal with the request and continue the loop.
	// The function to be run in this new thread is described below:
		// Check if it's read or write request
		// If it's read, send request to OPC Client and wait for data.
		// If it's write, send request to OPC Client and wait for ACK.