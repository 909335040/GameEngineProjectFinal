 
#pragma comment(lib,"ws2_32.lib")

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <iostream>
#include <vector>
using namespace std;

SOCKET	connections[100];                                                                                                                       /* holds up to 100 connections/connected sockets */
int	connectionNumbers = 0;                                                                                                                  /* number of max connected clients(not current, the max ever reached) */


void receiveFromClient(int index)                                                                                                             /* function/thread to receive data from clients and send to other clients */
{
	vector<float>	messageFromClient;                                                                                                      /* where the playback data will be stored */
	int		length;                                                                                                                 /* number of elements in the playback data vector. */
	while (true)
	{
		char buffer[256];                                                                                                               /* buffer */


		char charCheck = recv(connections[index], (char *)buffer, sizeof(buffer), NULL);                                             /* receive buffer, the first message received specifies whats the data about */
		if (charCheck == SOCKET_ERROR)                                                                                                /* if its socket error just return or else it'll spam the last message it got */
		{
			return;
		}
		else {
			string str = buffer;                                                                                                    /* turn the char buffer in to a string */
			if (str == "sync")                                                                                                    /* if the first message received is "sync", means that further data received will be about playback */

			{
				char charCheck1 = recv(connections[index], (char *)&length, sizeof(int), NULL);                              /* receive length, which is numbers of elements in the all data vector */
				if (charCheck1 == SOCKET_ERROR)                                                                               /* if its socket error, return */
				{
					return;
				}
				else {
					messageFromClient.resize(length);                                                                     /* resize the local vector , to prepare for receiving data */
				}
				char charCheck2 = recv(connections[index], (char *)&messageFromClient[0], length * sizeof(float), NULL);     /* receive vector */

				if (charCheck2 == SOCKET_ERROR)                                                                               /* if its socket error, return */
				{
					return;
				}
				else {
					/* if data is received send data to other connections */

					char MOTD[256] = "sync";
					for (int i = 0; i < connectionNumbers; i++)
					{
						if (i != index)                                                                               /* send data to all OTHER connected clients */
						{
							send(connections[i], MOTD, sizeof(MOTD), NULL);                                       /* send what the is data about */
							send(connections[i], (char *)&length, sizeof(int), NULL);                            /* send length */
							send(connections[i], (char *)&messageFromClient[0], sizeof(float) * length, NULL);   /* send vector */
						}
					}
				}
			}
			else if (str == "othermovement")                                                                                     /* if the string received from the first recv is other movement which meant its about moving the red */

			{
				char	action[256];                                                                                            /* message holder, message holding will be about controlling the red cube */
				char	charCheck3 = recv(connections[index], (char *)action, sizeof(action), NULL);                         /* receive data */

				if (charCheck3 == SOCKET_ERROR)                                                                               /* if socket error return */
				{
					return;
				}
				else {
					/* if data received correctly,sned this message to all OTHER clients connected */
					for (int i = 0; i < connectionNumbers; i++)
					{
						if (i != index)
						{
							send(connections[i], (char *)action, sizeof(action), NULL);
						}
					}
				}
			}
		}
	}
}


void control(int index)                       /* function/thread for remote control the models */
{
	bool	moveModel = false;        /* flags ,so when key events changes it only sends a message for once, and not repeating over and over again, also represents current state of model */
	bool	moveModelBack = false;
	bool	moveModelLeft = false;
	bool	moveModelRight = false;
	bool	moveModelFront = false;

	while (true)
	{
		if (GetAsyncKeyState('I') != 0)                                                                                     /* if Key I is pressed */


		{
			char MOTD[256] = "W";                                                                                           /* message W */
			moveModel = false;                                                                                              /* change movemodel flag to false */
			if (moveModelFront == false)                                                                                  /* use this flag so the message only send once per client */
			{
				for (int i = 0; i < connectionNumbers; i++)
				{
					send(connections[i], MOTD, sizeof(MOTD), NULL);                                               /* send this message "W" to all connected clients */
				}

				moveModelFront = true;                                                                                  /* change the flag so message wont send again before key release and pressed again */
			}
		}
		else {
			moveModelFront = false;
		}                                                                                      /* if the key I is released change the flag} */
			if (GetAsyncKeyState('K') != 0)                                                                             /* if Key K is pressed */

			{
				char MOTD[256] = "S";                                                                                   /* message S */
				moveModel = false;                                                                                      /* change movemodel flag to false meant this model should be moving at this time */
				if (moveModelBack == false)
				{
					for (int i = 0; i < connectionNumbers; i++)
					{
						send(connections[i], MOTD, sizeof(MOTD), NULL);                                       /* send this message "S" to all connected clients */
					}

					moveModelBack = true;                                                                           /* change the flag so message wont send again before key release and pressed again */
				}
			}
			else { moveModelBack = false; }
			 if (GetAsyncKeyState('J') != 0)                                                                             /* if Key J is pressed */

			{
				char MOTD[256] = "A";                                                                                   /* message A */
				moveModel = false;                                                                                      /* change movemodel flag to false */
				if (moveModelLeft == false)                                                                           /* use this flag so the message only send once per client */
				{
					for (int i = 0; i < connectionNumbers; i++)
					{
						send(connections[i], MOTD, sizeof(MOTD), NULL);                                       /* send this message "A" to all connected clients */
					}

					moveModelLeft = true;                                                                           /* change the flag so message wont send again before key release and pressed again */
				}
			}
			else { moveModelLeft = false; }
			 if (GetAsyncKeyState('L') != 0)                                                                             /* if Key L is pressed */

			{
				char MOTD[256] = "D";                                                                                   /* message D */
				moveModel = false;                                                                                      /* change movemodel flag to false */
				if (moveModelRight == false)                                                                          /* use this flag so the message only send once per client */
				{
					for (int i = 0; i < connectionNumbers; i++)
					{
						send(connections[i], MOTD, sizeof(MOTD), NULL);                                       /* send this message "D" to all connected clients */
					}

					moveModelRight = true;                                                                          /* change the flag so message wont send again before key release and pressed again */
				}
			}
			else { moveModelRight = false; }


			if (moveModelLeft == false && moveModelRight == false && moveModelFront == false && moveModelBack == false)   /* if none of the keys are pressed */
			{
				char MOTD[256] = "none";                                                                                /* message none */
				if (moveModel == false)                                                                               /* use this flag so the message only send once per client */
				{
					for (int i = 0; i < connectionNumbers; i++)
					{
						send(connections[i], MOTD, sizeof(MOTD), NULL);                                       /* send this message "none" to all connected clients */
					}

					moveModel = true;                                                                               /* change the flag so message wont send again before key release and pressed again */
				}
			}
		
	}
}


int main()
{                                                                                               /* winsock startups */
	WSAData wsadata;
	WORD	dllversion = MAKEWORD(2, 1);
	if (WSAStartup(dllversion, &wsadata) != 0)
	{
		MessageBoxA(NULL, "winsockstartupfaled", "error", MB_OK | MB_ICONERROR);      /* if return value is not zero, its an error pop out the message box */
	}
	SOCKADDR_IN	addr;                                                                   /* address to bind listening socket to */
	int		addrlen = sizeof(addr);                                                 /* whenever bind the address to the socket, need a pointer to this integer(a integer that holds the size) */
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");                                     /* ip, to host server */
	addr.sin_port = htons(1111);                                                /* port */
	addr.sin_family = AF_INET;                                                      /* ipv4socket */

	SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);                                  /* creates socket to listen new connections */
	bind(sListen, (SOCKADDR *)&addr, sizeof(addr));                                      /* bind address&port to socket */
	listen(sListen, SOMAXCONN);                                                           /* make the socket listen to connections */
	SOCKET newconnection;                                                                 /*client socket*/
	for (int i = 0; i < 100; i++)
	{
		newconnection = accept(sListen, (SOCKADDR *)&addr, &addrlen);                /* accept new connections, up to 100 */
		if (newconnection == 0)
		{
			cout << "No connection" << endl;
		}                                                                               /* no connection */
		else {
			cout << "connection" << endl;                                           /* if theres connection,store it in an array and create one control thread and one client thread for each connection */
			connections[i] = newconnection;
			connectionNumbers += 1;                                           /* add connections index by 1 */
			CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)control, (LPVOID)(i), NULL, NULL);
			CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)receiveFromClient, (LPVOID)(i), NULL, NULL);
		}
	}
	system("pause");
	return(0);
}


