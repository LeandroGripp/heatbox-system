// Engenharia de Controle e Automacao - UFMG
// Disciplina: Sistemas Distribuidos para Automacao
// Trabalho sobre OPC e Sockets
//
// Programa-cliente para simular o Sistema de Planejamento de Opera��es - 2023/1
// Luiz T. S. Mendes - 03/05/2023
//
// O programa envia mensagens de solicita��o de dados de processo ao servidor
// de sockets, com periodicidade dada pela constante PERIODO em unidades de
// 100 ms. Assim, p. ex., se PERIODO = 20,  as mensagens ser�o enviadas a cada
// 20 * 100ms = 2 segundos.
//
// A tecla "s", quando digitada, for�a o envio de uma mensagem de par�metros
// de controle ao servidor de sockets.
//
// Para a correta compilacao deste programa, nao se esqueca de incluir a
// biblioteca Winsock2 (Ws2_32.lib) no projeto ! (No Visual C++ Express Edition:
// Project->Properties->Configuration Properties->Linker->Input->Additional Dependencies).
//
// Este programa deve ser executado via janela de comandos, e requer OBRIGATORIAMENTE
// o endere�o IP do computador servidor e o porto de conex�o. Se voc� quiser execut�-lo
// diretamente do Visual Studio, lembre-se de definir estes dois par�metros previamente
// clicando em Project->Properties->Configuration Properties->Debugging->Command Arguments
//
// ATENC�O: APESAR DESTE PROGRAMA TER SIDO TESTADO PELO PROFESSOR, NAO H� GARANTIAS DE
// QUE ESTEJA LIVRE DE ERROS. SE VOCE ENCONTRAR ALGUM ERRO NO PROGRAMA, COMUNIQUE O
// MESMO AO PROFESSOR, INDICANDO TAMB�M, SEMPRE QUE POSS�VEL, SUA CORRE��O PROPOSTA.
//

#define _CRT_SECURE_NO_WARNINGS         /* Para evitar warnings sobre fun�oes seguras de */
                                        /*   manipulacao de strings */
#define _WINSOCK_DEPRECATED_NO_WARNINGS /* Para evitar warnings de fun��es WINSOCK depreciadas */

// Recentes vers�es do Visual Studio passam a indicar o seguinte
// warning do IntelliSense:
//
//    C6031: Return value ignored: 'sscanf'.
//
// Os comandos 'pragma' abaixo s�o para instruir o compilador a ignorar
// estes warnings. Dica extra�da de:
//    https://www.viva64.com/en/k/0048/
//    https://www.fluentcpp.com/2019/08/30/how-to-disable-a-warning-in-cpp/

#pragma warning(push)
#pragma warning(disable:6031)
#pragma warning(disable:6385)
// Ao final do programa h� um #pragma warning(pop) !

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
#define PERIODO      20 // O per�odo � estabelecido em unidades de 100ms
                        // 20 * 100ms = 2 segundos

void main(int argc, char **argv)
{
   WSADATA       wsaData;
   SOCKET        s;
   SOCKADDR_IN   ServerAddr;
   SYSTEMTIME    SystemTime;
   HANDLE        hTimer;
   LARGE_INTEGER Preset;
   BOOL          bSucesso;

   // Define uma constante para acelerar c�lculo do atraso e per�odo
   const int nMultiplicadorParaMs = 10000;

   DWORD wstatus;
   char msgreq[TAMMSGREQ + 1] = "99|005335";
   char msgack[TAMMSGACK + 1] = "33|007653";
   char msgackclp[TAMMSGACK + 1];
   char msgdados[TAMMSGDADOS + 1];
   char msgpar[TAMMSGPAR + 1];
   char msgpar1[TAMMSGPAR + 1] = "00|044356|076|10|FALSE";
   char msgpar2[TAMMSGPAR + 1] = "00|000552|122|05|TRUE ";

   char buf[100];
   char msgcode[3];
   char *ipaddr;
   int  port, status, nseq = 0, nseqreq = 0, tecla = 0, cont, vez = 0, flagreq = 0;
   int nseql, nseqr;
   int k=0;
   HANDLE hOut;

   // Testa parametros passados na linha de comando
   if (argc != 3) {
	   printf ("Uso: cliente_tcp <endere�o IP> <port>\n");
	   exit(0);
   }
   ipaddr = argv[1];
   port = atoi(argv[2]);

   // Inicializa Winsock vers�o 2.2
   status = WSAStartup(MAKEWORD(2,2), &wsaData);
   if (status != 0) {
	   printf("Falha na inicializacao do Winsock 2! Erro  = %d\n", WSAGetLastError());
	   WSACleanup();
       exit(0);
   }

   // Cria "waitable timer" com reset autom�tico
   hTimer = CreateWaitableTimer(NULL, FALSE, "MyTimer");

   // Programa o temporizador para que a primeira sinaliza��o ocorra 100ms 
   // depois de SetWaitableTimer 
   // Use - para tempo relativo
   Preset.QuadPart = -(100 * nMultiplicadorParaMs);
   // Dispara timer
   bSucesso = SetWaitableTimer(hTimer, &Preset, 100, NULL, NULL, FALSE);
   if(!bSucesso) {
	 printf ("Erro no disparo da temporiza��o! C�digo = %d\n", GetLastError());
	 WSACleanup();
	 exit(0);
   }

   // Inicializa a estrutura SOCKADDR_IN que ser� utilizada para
   // a conex�o ao servidor.
   ServerAddr.sin_family = AF_INET;
   ServerAddr.sin_port = htons(port);    
   ServerAddr.sin_addr.s_addr = inet_addr(ipaddr);

   	// Obt�m um handle para a sa�da da console
	hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE)
		printf("Erro ao obter handle para a sa�da da console\n");

   // ==================================================================
   // LOOP PRINCIPAL - CRIA SOCKET E ESTABELECE CONEX�O COM O SERVIDOR
   // ==================================================================

   while (TRUE) {
     //printf("LOOP PRINCIPAL\n");
	 SetConsoleTextAttribute(hOut, WHITE);
	 nseql = 0; // Reinicia numera��o das mensagens

     // Cria um novo socket para estabelecer a conex�o.
     s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
     if (s == INVALID_SOCKET) {
		 SetConsoleTextAttribute(hOut, WHITE);
	     status=WSAGetLastError();
	     if ( status == WSAENETDOWN)
           printf("Rede ou servidor de sockets inacess�veis!\n");
	     else
		   printf("Falha na rede: codigo de erro = %d\n", status);
		 SetConsoleTextAttribute(hOut, WHITE);
	     WSACleanup();
	     exit(0);
     }
   
	 // Estabelece a conex�o inicial com o servidor
	 printf("ESTABELECENDO CONEXAO COM O SERVIDOR TCP...\n");
     status = connect(s, (SOCKADDR *) &ServerAddr, sizeof(ServerAddr));
     if (status == SOCKET_ERROR) {
		SetConsoleTextAttribute(hOut, HLRED);
	    if (WSAGetLastError() == WSAEHOSTUNREACH) {
			printf ("Rede inacessivel... aguardando 5s e tentando novamente\n");
			// Testa se ESC foi digitado antes da espera
		    if (_kbhit() != 0) {
		      if (_getch() == ESC) break;
			}
			else { 
			  Sleep(5000);
			  continue;
			}
		}
		else {
			printf("Falha na conexao ao servidor ! Erro  = %d\n", WSAGetLastError());
			SetConsoleTextAttribute(hOut, WHITE);
	        WSACleanup();
			closesocket(s);
            exit(0);
        }
	 }

	 // ----------------------------------------------------------------------
     // LOOP SECUNDARIO - Troca de mensagens com o sistema de controle,
     // at� que o usu�rio digite ESC.
	 // ----------------------------------------------------------------------

	 cont = 0;
     for (;;) {
	   // Aguarda sinaliza��o do temporizador para enviar msg de requisi��o de dados
	   wstatus = WaitForSingleObject(hTimer, INFINITE);
	   if (wstatus != WAIT_OBJECT_0) {
		  SetConsoleTextAttribute(hOut, HLRED);
		  printf("Erro no recebimento da temporizacao ! c�digo  = %d\n", GetLastError());
	      WSACleanup();
		  closesocket(s);
          exit(0);
	   }
	   // Testa se usu�rio digitou ESC, em caso afirmativo, encerra o programa
	   if (_kbhit() != 0)
		 if ((tecla = _getch()) == ESC) break;
	     // Ajusta indicador caso a tecla 'p' tenha sido digitada
		 else if (tecla == 's') flagreq = 1;
	   
	   // A variavel "cont", quando igual a zero, indica o momento de enviar nova mensagem.
	   // Isto ocorre no 1o. pulso de temporiza��o e, depois, de 20 em 20 pulsos (lembrando
	   // que 20 * 100ms = 2s).
	   if (cont == 0) {
		 // Exibe a hora corrente
         GetSystemTime(&SystemTime);
		 SetConsoleTextAttribute(hOut, WHITE);
		 printf ("Sist. Planej. Operacoes: data/hora local = %02d-%02d-%04d %02d:%02d:%02d\n",
	              SystemTime.wDay, SystemTime.wMonth, SystemTime.wYear,
		          SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);
		 
		 // Envia msg de solicita��o de dados
	     sprintf(buf, "%06d", ++nseql);
	     memcpy(&msgreq[3], buf, 6);
	     status = send(s, msgreq, TAMMSGREQ, 0);
	     if (status == TAMMSGREQ) {
           SetConsoleTextAttribute(hOut, HLGREEN);
		   printf ("Msg de requisicao de dados enviada ao CLP [%s]:\n%s\n\n",
			       ipaddr, msgreq);
		 }
		 else {
		   SetConsoleTextAttribute(hOut, HLRED);
           if (status == 0)
			 // Esta situa��o N�O deve ocorrer, e indica algum tipo de erro.
			 printf("SEND (2) retornou 0 bytes ... Falha!\n");
		   else if (status == SOCKET_ERROR) {
			 status = WSAGetLastError();
			 printf("Falha no SEND (2): codigo de erro = %d\n", status);
		   }
		   printf("Tentando reconex�o ..\n\n");
		   break;
		 }

		 // Aguarda mensagem de dados do CLP
	     memset(buf, sizeof(buf), 0);
	     status = recv(s, buf, TAMMSGDADOS, 0);
		 if (status == TAMMSGDADOS) {
		   if (strncmp(buf, "09", 2) != 0) {
			 strncpy(msgcode, buf, 2);
			 msgcode[2] = 0x00;
			 SetConsoleTextAttribute(hOut, HLRED);
			 printf("Codigo incorreto de mensagem de dados do CLP: recebido '%s' ao inves de '09'\n", msgcode);
			 printf("Encerrando o programa...\n");
			 closesocket(s);
			 WSACleanup();
			 SetConsoleTextAttribute(hOut, WHITE);
			 exit(0);
		   }
		   sscanf(&buf[3], "%6d", &nseqr);
		   if (++nseql != nseqr) {
			 SetConsoleTextAttribute(hOut, HLRED);
			 printf("Numero sequencial de mensagem incorreto [1]: recebido %d ao inves de %d.\n",
			         nseqr, nseql);
			 printf("Encerrando o programa...\n");
			 closesocket(s);
			 SetConsoleTextAttribute(hOut, WHITE);
			 WSACleanup();
			 exit(0);
		   }
		   strncpy(msgdados, buf, TAMMSGDADOS);
		   msgdados[TAMMSGDADOS] = 0x00;
		   SetConsoleTextAttribute(hOut, YELLOW);
		   printf ("Mensagem de dados recebida do CLP [%s]:\n%s\n\n",
			       ipaddr, msgdados);

		   /* Envia ACK ao Sistema de Controle */
		   sprintf(buf, "%06d", ++nseql);
		   memcpy(&msgack[3], buf, 6);
		   status = send(s, msgack, TAMMSGACK, 0);
		   if (status == TAMMSGACK) {
			   msgack[TAMMSGACK] = 0x00;
			   SetConsoleTextAttribute(hOut, HLBLUE);
			   printf("Mensagem de ACK enviada ao CLP [%s]:\n%s\n\n",
				   ipaddr, msgack);
		   }
		   else {
			   SetConsoleTextAttribute(hOut, HLRED);
			   if (status == 0)
				   // Esta situa��o N�O deve ocorrer, e indica algum tipo de erro.
				   printf("SEND (3) retornou 0 bytes ... Falha!\n");
			   else if (status == SOCKET_ERROR) {
				   status = WSAGetLastError();
				   printf("Falha no SEND (3): codigo de erro = %d\n", status);
			   }
			   printf("Tentando reconex�o ..\n\n");
			   break;
		   }
		 }
	     else {
		   SetConsoleTextAttribute(hOut, HLRED);
		   if (status == 0)
	         // Esta situa��o N�O deve ocorrer, e indica algum tipo de erro.
		     printf ("RECV (2) retornou 0 bytes ... Falha!\n");
		   else if (status == SOCKET_ERROR) {
             status = WSAGetLastError();
	         if (status == WSAETIMEDOUT) 
			   printf ("Timeout no RECV (2)!\n");
		     else printf("Erro no RECV (2)! codigo de erro = %d\n", status);
		   }
		   printf ("Tentando reconexao ..\n\n");
		   break;
		   }
	     }

	   // Testa se usu�rio digitou tecla "s"
	   if (flagreq == 1) {
		 flagreq = 0;

		 // Sim: envia msg de par�metros de controle ao CLP
		 if (vez == 0)
			 strncpy(msgpar, msgpar1, TAMMSGPAR);
		 else
			 strncpy(msgpar, msgpar2, TAMMSGPAR);
		 vez = 1 - vez;
		 sprintf(buf, "%06d", ++nseql);
		 memcpy(&msgpar[3], buf, 6);
	     status = send(s, msgpar, TAMMSGPAR, 0);
		 if (status == TAMMSGPAR){
		   msgpar[TAMMSGPAR] = 0x00;
		   SetConsoleTextAttribute(hOut, MAGENTA);
		   printf("Mensagem com comandos de sinalizacao enviada ao CLP [%s]:\n%s\n\n",
			       ipaddr, msgpar);
		 }
		 else {
		   SetConsoleTextAttribute(hOut, HLRED);
		   if (status == 0)
			 // Esta situa��o N�O deve ocorrer, e indica algum tipo de erro.
		     printf("SEND (3) retornou 0 bytes ... Falha!\n");
		   else if (status == SOCKET_ERROR) {
			 status = WSAGetLastError();
			 printf("Falha no SEND (3): codigo de erro = %d\n", status);
		   }
		   printf("Tentando reconex�o ..\n\n");
		   break;
		 }

		 // Aguarda mensagem de ACK do CLP
		 memset(buf, sizeof(buf), 0);
		 status = recv(s, buf, TAMMSGACK, 0);
		 if (status == TAMMSGACK) {
		   if (strncmp(buf, "44", 2) != 0) {
		     strncpy(msgcode, buf, 2);
			 msgcode[2] = 0x00;
			 SetConsoleTextAttribute(hOut, HLRED);
			 printf("Codigo incorreto de mensagem de ACK: recebido '%s' ao inves de '33'\n", msgcode);
			 printf("Encerrando o programa...\n");
			 closesocket(s);
			 WSACleanup();
			 SetConsoleTextAttribute(hOut, WHITE);
			 exit(0);
		   }
		   sscanf(&buf[3], "%6d", &nseqr);
		   if (++nseql != nseqr) {
			 SetConsoleTextAttribute(hOut, HLRED);
			 printf("Numero sequencial de mensagem incorreto [1]: recebido %d ao inves de %d.\n",
				     nseqr, nseql);
			 printf("Encerrando o programa...\n");
			 closesocket(s);
			 WSACleanup();
			 SetConsoleTextAttribute(hOut, WHITE);
			 exit(0);
		   }
		   strncpy(msgackclp, buf, TAMMSGACK);
		   msgackclp[TAMMSGACK] = 0x00;
		   SetConsoleTextAttribute(hOut, MAGENTA);
		   printf("Mensagem de ACK recebida do CLP [%s]:\n%s\n\n", ipaddr, msgackclp);
		 }
		 else {
		   SetConsoleTextAttribute(hOut, HLRED);
		   if (status == 0)
		     // Esta situa��o N�O deve ocorrer, e indica algum tipo de erro.
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
	   // Atualiza o contador de pulsos. Quando "cont" alcanca o valor de 19, seu valor �
	   // zerado para que a proxima ativacao do temporizador (que ocorrera' no pulso de n. 20)
	   // force novo envio de mensagem.
	   cont = ++cont % (PERIODO-1);
	 }// end for(;;) secund�rio

	 if (tecla == ESC) break;
	 else closesocket(s);
   }// end while principal

   // Fim do enlace: fecha o "socket" e encerra o ambiente Winsock 2.
   SetConsoleTextAttribute(hOut, WHITE);
   printf ("Encerrando a conexao ...\n");
   closesocket(s);
   WSACleanup();
   printf ("Conexao encerrada. Fim do programa.\n");
   exit(0);
}
#pragma warning(pop)
