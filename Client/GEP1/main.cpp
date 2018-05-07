#pragma comment(lib,"ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <fstream>
#include <stdio.h>
#include "window.h"
#include "drawSphere.h"
#include <vector>
#include <thread>
#include <iostream>
using namespace std;
int		starttime;                              /* stores the time that recording keyboard events starts */
int		recordFlag = 0;
int		startKey = 0x70;                 /* virtual key code of f1 */
int		playKey = 0x71;                 /*  virtual key code for f2 */
vector4		modelTransforms;
vector<int>	keycodeVec;                             /* list for recording the event(what key) */
vector<int>	timeVec;                                /* list for recording the time(when is the key event happened) */
vector<bool>	eventTypeVec;                           /* list for recording what is the event(key down or key up) */
vector<float>	allData;                                /* vector with model pos x posz rotation and all data from 3 playback related vectors */
bool		keydownFlags[256];                      /* key down Flag, each spot for a key(the first 6 are for mouses,  which i have not and not planning to do...the 7th is undifined, 8-255 is for key board) */
bool		releaseFlag = false;                /* release Flag for the recording key(f1),to prevent it to get in the recording */
bool		doneReleaseFlag = false;                /* another release Flag still for f1, to prevent it from repeating over and over again(record stop record stop record stop while pressing the key) */
Window		window;
bool		sendKeyDown = false;                /* keydown flags, so that the messages are only send once, this flag is for 'Z',passing playback data */
bool		tKeyDown = false;                /* keydown flags for controlling the premade red cube */
bool		gKeyDown = false;
bool		fKeyDown = false;
bool		hKeyDown = false;
bool		allKeyUp = false;
SOCKET		connection;                             /* socket */

int	state = 0;                            /* state, this variable will change base on key pressed(ijkl on server side), will cause the model move */
int	otherState = 0;                            /* state, this variable will change if tfgh is pressed on another client, it will cause the premade red cube to move */

void saveToFile()
{
	ofstream	keyData("key.txt");           /* write playback related data to .txt files , different vectors to different files */
	ofstream	timeData("time.txt");
	ofstream	typeData("type.txt");
	ofstream	modelData("model.txt");
	for (size_t i = 0; i < keycodeVec.size(); i++)
	{
		keyData << keycodeVec.at(i) << endl;  /* loop through all vectors,and save each element in a line */
		timeData << timeVec.at(i) << endl;
		typeData << eventTypeVec.at(i) << endl;
	}
	modelTransforms = window.returnModelData();
	modelData << modelTransforms.x << endl << modelTransforms.z << endl << modelTransforms.rot << endl;
	keyData.close();
	timeData.close();
	typeData.close();
}


void loadFromFile()
{
	ifstream	keyData("key.txt");   /* read playback related data from .txt files */
	ifstream	timeData("time.txt");
	ifstream	typeData("type.txt");
	ifstream	modelData("model.txt");
	if (!keyData.is_open())
	{
		cout << "Error";                /* actually it doesn't handle any incorrect data but print error if failed to open the filess */
	}
	else if (!timeData.is_open())
	{
		cout << "Error";
	}
	else if (!typeData.is_open())
	{
		cout << "Error";
	}
	else {
		keycodeVec.clear();             /* if the files are opened, clear whatever it is in the vectors now */
		timeVec.clear();
		eventTypeVec.clear();


		while (!keyData.eof())
		{
			int dataFromFile;       /* read data to files,save in local variables, push to the vector */
			keyData >> dataFromFile;

			keycodeVec.push_back(dataFromFile);
		}
		while (!timeData.eof())
		{
			int dataFromFile;
			timeData >> dataFromFile;

			timeVec.push_back(dataFromFile);
		}
		while (!typeData.eof())
		{
			int dataFromFile;
			typeData >> dataFromFile;

			eventTypeVec.push_back(dataFromFile);
		}


		modelData >> modelTransforms.x >> modelTransforms.z >> modelTransforms.rot;     /* read model data */

		window.writeModelData(modelTransforms);                                       /* pass model data to window */


		keycodeVec.pop_back();                                                          /* for some reason when reading the files and pushing to vectors, the last element repeats once, so pop the one at the back */
		timeVec.pop_back();
		eventTypeVec.pop_back();
	}
}


int moveModel()
{
	while (true)
	{
		window.saveState(state); /* save state to the variable statea in window ,potentially moves the model(if state is 0 doesnt move) */
	}

	return(0);
}


int clientThread()
{
	char buffer[256];                                                                                               /* array to receive data */

	while (true)
	{
		char charr = recv(connection, buffer, sizeof(buffer), NULL);                                          /* receive data */
		if (charr == SOCKET_ERROR)                                                                            /* if it is a socket error, meant that the server probably disconnected, therefore reset state to 0 and return , otherwise it'll spam the message */

		{
			state = 0;                                                                                      /* set state to 0 , the model won't do anything */
			return(-1);
		}
		string str = buffer;                                                                                    /* if it isn't a socket error, cast the char array in to a string */
		if (str == "W")                                                                                       /* if the string received is w, change the state to 1,the model will move up */
		{
			cout << buffer;
			state = 1;
		}
		else if (str == "S")                                                                                 /* if the string received is s, change the state to 2,the model will move down */
		{
			cout << buffer;
			state = 2;
		}
		else if (str == "A")                                                                                 /* if the string received is a, change the state to 3,the model will move left */
		{
			cout << buffer;
			state = 3;//nimahaizaime
		}
		else if (str == "D")                                                                                 /* if the string received is d, change the state to 4,the model will move right */
		{
			cout << buffer;
			state = 4;
		}
		else if (str == "none")                                                                              /* if the string received is none(the server will send this once if none of the movement keys are pressed), change the state to 0,the model will move stay at where it and do nothing */
		{
			cout << buffer;


			state = 0;
			 
		}
		else if (str == "sync")                                                                              /* if the string received is sync, which means the data receving next will be related to playback data */
		{
			vector<float>	messageFromClient;                                                              /* vector to store the alldata vector from the other client */
			int		length;                                                                         /* length of the vector */
			char		sizeOfVec = recv(connection, (char *)&length, sizeof(int), NULL);            /* receive length */
			if (sizeOfVec == SOCKET_ERROR)                                                                /* if socket error return */
			{
				return(0);
			}
			else {
				messageFromClient.resize(length);                                                     /* resize vector so the vector have enough space to receive data */
			}
			char charr = recv(connection, (char *)&messageFromClient[0], length * sizeof(float), NULL);  /* receive playback data store to vectors */

			if (charr == SOCKET_ERROR)                                                                    /* if socket error return */
			{
				return(0);
			}
			else {
				keycodeVec.clear();                                                                     /* clear the current data in three vectors */
				timeVec.clear();
				eventTypeVec.clear();

				modelTransforms.x = messageFromClient[0];                                         /* the first three elements from the received vector are the models x pos z pos and rotation(at start of recording) */
				modelTransforms.z = messageFromClient[1];
				modelTransforms.rot = messageFromClient[2];
				window.writeModelData(modelTransforms);                                               /* store the data to the window */
				int singleVecLength = (length - 3) / 3;                                                 /* each vectors length are equal so the length for each (playback) vector is (total length - 3(modelx modelz modelrot))/3 */
				for (int i = 3; i < singleVecLength + 3; i++)
				{
					keycodeVec.push_back((int)messageFromClient.at(i));                        /* push received data in following vectors */
				}
				for (int i = singleVecLength + 3; i < singleVecLength * 2 + 3; i++)
				{
					timeVec.push_back((int)messageFromClient.at(i));
				}
				for (int i = singleVecLength * 2 + 3; i < singleVecLength * 3 + 3; i++)
				{
					eventTypeVec.push_back(messageFromClient.at(i));
				}
				saveToFile(); /* save the data to the .txt file */
			}
		} /* the message received below is passed from another client,if the message received are the same, change the other state value, pass it to the window, and it'll move the premade red cube */
		else if (str == "stop")
		{
			otherState = 0;   window.saveOtherState(otherState);
		}
		else if (str == "front")
		{
			otherState = 1;  window.saveOtherState(otherState);
		}
		else if (str == "back")
		{
			otherState = 2;  window.saveOtherState(otherState);
		}
		else if (str == "left")
		{
			otherState = 3;  window.saveOtherState(otherState);
		}
		else if (str == "right")
		{
			otherState = 4;  window.saveOtherState(otherState);
		}
	}


	return(0);
}


void record(int kfrcd)                                                                        /* the parameter is the keycode of the key for recording(supposed to be DWORD but i tested that int works fine without warnings while float doesn't,so int it is),for now its just hardcoded to 0x70 which is f1 */
{
	if (keycodeVec.size() <= 0 && releaseFlag == false)                                   /* if the f1 key havent been released its not recording */

	{
		if (GetAsyncKeyState(kfrcd) == 0)
		{
			releaseFlag = true;                                                 /* change the release Flag to true so can go to the else part */
			starttime = GetTickCount();                                       /* save the start time of recording(which is when the key is released) */
			window.storeVariables();                                                /* store model initial states */
			cout << "released";
		}
	}
	else {
		if (GetAsyncKeyState(kfrcd) & 0x8000)
		{
			releaseFlag = false;                                                    /* if the f1 keys pressed again, stop recording */
			for (int i = 8; i < 255; i++)
			{
				keydownFlags[i] = false;                                        /* reset the keydown Flags */
			}
			recordFlag = 0;
			saveToFile();                                                           /* change recording Flag to 0(ready to playback or record again) */
			cout << "stopped recording";
			return;
		}


		for (int i = 8; i < 255; i++)                                                 /* 8-255 are keys on keyboard  , didnt use virtual keycodes like 0x30 here simply because i dont have any ideas of how to put that in a loop, but the parameter of */
		{                                                                               /* getasynckeystate accepting is an int anyways */
			if (GetAsyncKeyState(i) & 0x8000)                                   /* if get the key pressed */
			{
				if (!keydownFlags[i] && i != kfrcd)                           /* if the key wasn't pressed already from previous, and the key isn't f1 */
				{
					cout << "getkey" << "  " << i << endl << endl;          /* print the key got */
					keycodeVec.push_back(i);                              /* push the virtual keycode to the list */
					timeVec.push_back(GetTickCount() - starttime);        /* push the time(has past)since the start of recording to the time list */
					eventTypeVec.push_back(true);                         /* push true to the Flags list, which means that this is a key pressed instead of released */
					keydownFlags[i] = 1;                                    /* change the Flag to 1 which means this key is pressed now so wont push all the stuff to lists over and over again */
				}
			}

			if (GetAsyncKeyState(i) == 0 && i != kfrcd)                         /* if the key is not pressed and its not f1 */

			{
				if (keydownFlags[i] == 1)                                     /* if the key is pressed(previously) */
				{
					cout << "key released" << "  " << i << endl << endl;

					keycodeVec.push_back(i);                              /* push the key to list again */
					timeVec.push_back(GetTickCount() - starttime);        /* push the time (has past since start of recording) to the time list */
					eventTypeVec.push_back(false);                        /* push false to Flags list which means this is a key released instead of pressed */
					keydownFlags[i] = 0;                                    /* change the Flag to 0 which means this key is now released instead of being pressed */
				}
			}
		}
	}
}


void playbackFunc()                                                             /* playback */
{
	if (keycodeVec.size() > 0)                                            /* if any key have been pressed during record, playback */
	{
		cout << timeVec.at(0);
		window.playbackInit();                                          /* bring the model back to initial state when start recording */
		for (size_t i = 0; i < keycodeVec.size(); i++)                /* before the last key is pressed */
		{
			if (i == 0)
			{                                                       /* for the first key , sleep for the exact time(pushed in to list.) before simulate the press , sleep is like holding the keys state(if the key is pressed by that time, it'll hold the key until release is called) */
				Sleep(timeVec.at(0));
			}
			else {
				Sleep(timeVec.at(i) - timeVec.at(i - 1)); /* for keys after the first key, sleep for time-previous time */
			}
			if (eventTypeVec.at(i) == true)                     /* if the matched Flags list states that this event is the keys being pressed, simulate it by using keybd_event */
			{
				keybd_event(keycodeVec.at(i),
					keycodeVec.at(i),
					KEYEVENTF_EXTENDEDKEY | 0,
					0);
			}
			else if (eventTypeVec.at(i) == false)              /* if opposite, simulate keyboard release */


			{
				keybd_event(keycodeVec.at(i),
					keycodeVec.at(i),
					KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP,
					0);
			}


			/*
			* theres this thing that the keyboard events(from keyboard,not from program) are not disabled while playback is happening, so if you pressed /released the same key included in playback, it'd messup
			* these keyboard events should be replaced in to functions when implementing games
			*/


			/* this part exits when all events has been gone through */
		}
	}
	else {
		return;                 /* if theres no keyboard event while recording , simply return, no playback */
	}

	for (int i = 8; i < 255; i++) /* after playback has done if there are still any keys not released, release them */
	{
		if (GetAsyncKeyState(i) & 0x8000)
		{
			keybd_event(keycodeVec.at(i),
				keycodeVec.at(i),
				KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP,
				0);
		}
	}

	recordFlag = 0;                                         /* change the record Flag to 0 , so its ready to */
}


void checkRecordingRelatedKeys()                                /* do different things based on value of recordFlag, pretty much the "game loop" for the keyboard recording */
{
	while (recordFlag == 2)                               /* while record Flag is 2 , which means there haven't been any recording done before. so playback is not available. can only record */

	{
		if (GetAsyncKeyState(startKey) & 0x8000)    /* if f1 pressed, change record Flag to 1 */

		{
			recordFlag = 1;

			cout << "changed Flag to 1 " << endl;
		}
	}

	while (recordFlag == 1)
	{
		/* record passed the hardcoded f1 in..... */
		record(startKey);
	}
	while (recordFlag == 0)                               /* while the record Flag is 0 which meant is ready for playback or recording again */

	{
		if (GetAsyncKeyState(playKey) & 0x8000)     /* if f2(hardcoded as well) is pressed, change it to someother value(5 is just some random value i picked...) so can only go through the playback, not playing, not recording again until playback has end */
		{
			recordFlag = 5;
			cout << "changed Flag to 5 " << endl;
			playbackFunc();                         /* playback */
		}
		if (GetAsyncKeyState(startKey) & 0x8000)    /* if f1 is pressed while recordFlag is 0, which meant the user wants to record again */

		{
			if (doneReleaseFlag == true)          /* done release Flag is true(after the release of f1(after pressing it....)) */
			{
				recordFlag = 1;                 /* change the record Flag, clear the data in 3 vectors */

				keycodeVec.clear();
				timeVec.clear();
				eventTypeVec.clear();
				doneReleaseFlag = false;        /* reset the Flag to false */
				cout << "data cleared and recording again" << endl;
			}
		}
		else if (GetAsyncKeyState(startKey) <= 0)
		{
			doneReleaseFlag = true;                 /* after the key is pressed, when the key is released, set the release Flag to true */
		}
	}
}


void checkThread()
{
	while (1)
	{
		checkRecordingRelatedKeys(); /* in while loop to go through over and over again */
	}
}


int winsockStartupAndRun()
{
	/* winsock startups */
	WSAData wsadata;
	WORD	dllversion = MAKEWORD(2, 1);
	if (WSAStartup(dllversion, &wsadata) != 0)                          /* if return value is not zero, its an error pop out the message box */

	{
		MessageBoxA(NULL, "winsockstartupfaled", "error", MB_OK | MB_ICONERROR);
	}
	SOCKADDR_IN	addr;                                                   /* address to bind listening socket to */
	int		addrlen = sizeof(addr);                                 /* whenever bind the address to the socket, need a pointer to this integer(a integer that holds the size) */
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");                     /* ip, should put server ip in here */
	addr.sin_port = htons(1111);                                /* port */
	addr.sin_family = AF_INET;                                      /* ipv4socket */


	connection = socket(AF_INET, SOCK_STREAM, NULL);                    /*create socket */ 
	if (connect(connection, (SOCKADDR *)&addr, addrlen) != 0)
	{
		/* if return isnt 0, means connection failed, error */
		MessageBoxA(NULL, "No Connection", "ERROR", MB_OK | MB_ICONERROR);
		return(0);
	}

	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)clientThread, NULL, NULL, NULL);                                    /* receive message thread */


	while (true)
	{
		if (tKeyDown == false && gKeyDown == false && fKeyDown == false && hKeyDown == false && allKeyUp == false)    /* if none of tgfh is down , send these 2 messages to the server(once), it'll pass the message(only the action one) to other clients */
		{
			char buffer[256] = "othermovement";                                                                     /* other movement so that the server knows that the next message will be about to the premade red cube movement */
			send(connection, buffer, sizeof(buffer), NULL);                                                       /* send message to server */
			char action[256] = "stop";                                                                              /* action message */
			send(connection, action, sizeof(action), NULL);                                                       /* send message to server */

			allKeyUp = true;                                                                                        /* change flag so that the messages would only be sent once */
		}

		if (GetAsyncKeyState('T') && tKeyDown == false)                                                             /* if t is down send following messages once to server ,the server will send the message(only the action one) to other clients */
		{
			char buffer[256] = "othermovement";                                                                     /* other movement so that the server knows that the next message will be about to the premade red cube movement */
			send(connection, buffer, sizeof(buffer), NULL);                                                       /* send message to server */

			char action[256] = "back";                                                                              /* action message */
			send(connection, action, sizeof(action), NULL);                                                       /* send message to server */
			allKeyUp = false;                                                                                /* there are key pressed so set the flag to false */
			tKeyDown = true;                                                                                 /* change flag so that the messages would only be sent once */
		}
		else if (GetAsyncKeyState('T') == 0 && tKeyDown == true)
		{
			tKeyDown = false;                                                                                       /* if key released, change flag back */
		}
		if (GetAsyncKeyState('G') && gKeyDown == false)                                                             /* if g is down send following messages once to server ,the server will send the message(only the action one) to other clients */
		{
			char buffer[256] = "othermovement";                                                                     /* other movement so that the server knows that the next message will be about to the premade red cube movement */
			send(connection, buffer, sizeof(buffer), NULL);                                                       /* send message to server */
			char action[256] = "front";                                                                             /* action message */
			send(connection, action, sizeof(action), NULL);                                                       /* send message to server */
			gKeyDown = true;                                                                                 /* change flag so that the messages would only be sent once */
			allKeyUp = false;                                                                                /* there are key pressed so set the flag to false */
		}
		else if (GetAsyncKeyState('G') == 0 && gKeyDown == true)
		{
			gKeyDown = false;                                                                                       /* if key released, change flag back */
		}
		else if (GetAsyncKeyState('F') && fKeyDown == false)                                                       /* if f is down send following messages once to server ,the server will send the message(only the action one) to other clients */

		{
			char buffer[256] = "othermovement";                                                                     /* other movement so that the server knows that the next message will be about to the premade red cube movement */
			send(connection, buffer, sizeof(buffer), NULL);                                                       /* send message to server */
			char action[256] = "left";                                                                              /* action message */
			send(connection, action, sizeof(action), NULL);                                                       /* send message to server */
			fKeyDown = true;                                                                                 /* change flag so that the messages would only be sent once */
			allKeyUp = false;                                                                                /* there are key pressed so set the flag to false */
		}
		else if (GetAsyncKeyState('F') == 0 && fKeyDown == true)
		{
			fKeyDown = false;                                                                                       /* if key released, change flag back */
		}
		else if (GetAsyncKeyState('H') && hKeyDown == false)                                                       /* if h is down send following messages once to server ,the server will send the message(only the action one) to other clients */
		{
			char buffer[256] = "othermovement";                                                                     /* other movement so that the server knows that the next message will be about to the premade red cube movement */
			send(connection, buffer, sizeof(buffer), NULL);                                                       /* send message to server */
			char action[256] = "right";                                                                             /* action message */
			send(connection, action, sizeof(action), NULL);                                                       /* send message to server */
			allKeyUp = false;                                                                                /* there are key pressed so set the flag to false */
			hKeyDown = true;                                                                                 /* change flag so that the messages would only be sent once */
		}
		else if (GetAsyncKeyState('H') == 0 && hKeyDown == true)
		{
			hKeyDown = false;                                                                                       /* if key released, change flag back */
		}


		if (GetAsyncKeyState('Z') && sendKeyDown == false)                                                          /* z to send playback data */

		{
			char buffer1[256] = "sync";                                                                             /* sync, first buffer sent , means that data sent after this will be about playback */
			send(connection, (char *)buffer1, sizeof(buffer1), NULL);                                            /* send to server */

			allData.clear();                                                                                        /* clear the alldata buffer in case theres any data in there */

			allData.push_back(modelTransforms.x);                                                                 /* push the model posx posz and rotation to be the first 3 elements of the all data vector */
			allData.push_back(modelTransforms.z);
			allData.push_back(modelTransforms.rot);
			for (size_t i = 0; i < keycodeVec.size(); i++)
			{
				allData.push_back((float)keycodeVec.at(i));                                                /* loop through all the playback data vectors and push them to the all data vector */
			}
			for (size_t i = 0; i < keycodeVec.size(); i++)
			{
				allData.push_back((float)timeVec.at(i));
			}
			for (size_t i = 0; i < keycodeVec.size(); i++)
			{
				allData.push_back(eventTypeVec.at(i));
			}
			int p = allData.size();                                                 /* size of all data vector */
			send(connection, (char *)&p, sizeof(int), NULL);                     /* send the length value to the server */

			send(connection, (char *)&allData[0], sizeof(float) * p, NULL);      /* Send all playback data to server */
			sendKeyDown = true;                                                     /* change the flag so the messages only sends once before the key is released and pressed again */
		}
		else if (GetAsyncKeyState('Z') == 0 && sendKeyDown == true)
		{
			sendKeyDown = false;                                                    /* change the flag back when the key is released */
		}
	}


	return(0);                                                                              /* should never get here */
}


int main(int argc, char** argv)
{
	thread	t(checkThread);                                                               /* in thread so not contradicting with the game loop of opengl */
	thread	t2(moveModel);                                                                /* pass the state value to window */
	thread	t1(winsockStartupAndRun);                                                     /* networking related thread */
	loadFromFile();                                                                         /* load playback data from file */
																							/* for winsock startups and create/run the client thread */
	window.Create_Window(argc, argv);
	return(0);
}


