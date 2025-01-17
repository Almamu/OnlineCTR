#include <iostream>
#include <cstdlib>
#include <string>
#include <ctime>

#include <SDL_net.h>

using namespace std;

int PORT = 1234;        
int sendLength = 0;
const unsigned short BUFFER_SIZE = 512;            
unsigned short MAX_SOCKETS;              
unsigned short MAX_CONNECTIONS;

bool isServer = false;
bool isClient = false;

const char *host;
char*    serverName;
IPaddress serverIP;             
SDLNet_SocketSet socketSet;
TCPsocket mySocket;              
TCPsocket clientSocket[8]; 
bool      socketIsFree[8]; 

char sendBuf[BUFFER_SIZE];  
char recvBuf[BUFFER_SIZE];
int receivedByteCount = 0;           
int clientCount = 0;                 
bool shutdownServer = false;   

#include <stdio.h>
#include <Windows.h>
#include <tlhelp32.h>

DWORD GetProcId()
{
	PROCESSENTRY32   pe32;
	HANDLE         hSnapshot = NULL;

	pe32.dwSize = sizeof(PROCESSENTRY32);
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (Process32First(hSnapshot, &pe32))
	{
		do
		{
			char* test[28];
			for (int i = 0; i < 18; i++)
				test[i] = (char*)&pe32.szExeFile + i;

			if (*test[0] == 'e' &&
				*test[2] == 'P' &&
				*test[4] == 'S' &&
				*test[6] == 'X' &&
				*test[8] == 'e' &&
				*test[10] == '.' &&
				*test[12] == 'e' &&
				*test[14] == 'x' &&
				*test[16] == 'e')

				return pe32.th32ProcessID;

		} while (Process32Next(hSnapshot, &pe32));
	}

	if (hSnapshot != INVALID_HANDLE_VALUE)
		CloseHandle(hSnapshot);
	return 0;
}

uintptr_t GetModuleBaseAddress(DWORD procId, const wchar_t* modName)
{
	uintptr_t modBaseAddr = 0;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
	if (hSnap != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 modEntry;
		modEntry.dwSize = sizeof(modEntry);
		if (Module32First(hSnap, &modEntry))
		{
			do
			{
				if (!_wcsicmp(modEntry.szModule, modName))
				{
					modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
					break;
				}
			} while (Module32Next(hSnap, &modEntry));
		}
	}
	CloseHandle(hSnap);
	return modBaseAddr;
}

unsigned int P1_StartPos[18 * 3] =
{
	// Crash Cove
	4294801664, 193792, 4293823744,

	// Roos Tubes
	4294893056, 2816, 447744,

	// Tiger Temple
	4293429504, 1536, 4294644480,

	// Coco Park
	61184, 45568, 4293474560,

	// Mystery Caves
	264192, 15360, 3618560,

	// Blizzard Bluff
	4294601984, 91648, 76800,

	// Sewer Speedway
	4294836992, 0, 75008,

	// Dingo Canyon
	126208, 589312, 224512,

	// Papu Pyramid
	343552, 4294922240, 4293427200,

	// Dragon Mines
	26112, 353792, 802304,

	// Polar Pass
	404992, 23808, 4294895360,

	// Cortex Castle
	820480, 0, 1644800,

	// Tiny Arena
	4293426176, 4294965760, 211712,

	// Hot Air Skyway
	67328, 0, 4294895360,

	// N Gin Labs
	2918400, 0, 905984,

	// Oxide Station
	1512192, 3840, 672512,

	// Slide Coliseum
	429824, 0, 4292237056,

	// Turbo Track
	1260800, 5888, 881664
};

// the number of nodes in each path
// there are 3 paths on each track
short numNodesInPaths[18 * 3] =
{
	// Crash Cove Path 1
	227,

	// Crash Cove Path 2
	239,

	// Crash Cove Path 3
	239,

	// Roos Tubes Path 1
	239,

	// Roos Tubes Path 2
	234,

	// Roos Tubes Path 3
	231,

	// Tiger Temple Path 1
	309,

	// Tiger Temple Path 2
	312,

	// Tiger Temple Path 3
	328,

	// Coco Park Path 1
	207,

	// Coco Park Path 2
	223,

	// Coco Park Path 3
	232,

	// Mystery Caves Path 1
	387,

	// Mystery Caves Path 2
	376,

	// Mystery Caves Path 3
	408,

	// Blizzard Bluff Path 1
	254,

	// Blizzard Bluff Path 2
	250,

	// Blizzard Bluff Path 3
	283,

	// Sewer Speedway Path 1
	365,

	// Sewer Speedway Path 2
	369,

	// Sewer Speedway Path 3
	327,

	// Dingo Canyon Path 1
	269,

	// Dingo Canyon Path 2
	247,

	// Dingo Canyon Path 3
	246,

	// Papu Pyramid Path 1
	287,

	// Papu Pyramid Path 2
	277,

	// Papu Pyramid Path 3
	292,

	// Dragon Mines Path 1
	268,

	// Dragon Mines Path 2
	272,

	// Dragon Mines Path 3
	285,

	// Polar Pass Path 1
	520,

	// Polar Pass Path 2
	516,

	// Polar Pass Path 3
	511,

	// Cortex Castle Path 1
	420,

	// Cortex Castle Path 2
	398,

	// Cortex Castle Path 3
	412,

	// Tiny Arena Path 1
	660,

	// Tiny Arena Path 2
	632,

	// Tiny Arena Path 3
	656,

	// Hot Air Skyway Path 1
	520,

	// Hot Air Skyway Path 2
	512,

	// Hot Air Skyway Path 3
	504,

	// N Gin Labs Path 1
	440,

	// N Gin Labs Path 2
	422,

	// N Gin Labs Path 3
	405,

	// Oxide Station Path 1
	648,

	// Oxide Station Path 2
	626,

	// Oxide Station Path 3
	635,

	// Slide Coliseum Path 1
	328,

	// Slide Coliseum Path 2
	309,

	// Slide Coliseum Path 3
	329,

	// Turbo Track Path 1
	299,

	// Turbo Track Path 2
	313,

	// Turbo Track Path 3
	341,
};

unsigned char gameStatePrev; // 0x161A871
unsigned char gameStateCurr; // 0x161A871
unsigned char weaponPrev; // relative to posX
unsigned char weaponCurr; // relative to posX
unsigned char trackID; // 0x163671A
unsigned char trackVideoID; // 0x16379C8
unsigned long long menuState;

// Distance to Finish for Player 1
// X + 0x1B4

unsigned int P1xAddr = -1;
unsigned int NavAddr1 = -1;
unsigned int NavAddr2 = -1;
unsigned int NavAddr3 = -1;

// AI[n]x = P1x - 0x354 - 0x670 * n
// where n = 0 - 6 (for positions 2 - 8)
unsigned int StartLinePos[8][3]; 

// ID[0] is the server's character
short characterIDs[8];

bool inRace = false; // in client

int main(int argc, char **argv)
{
	/*struct
	{
		short a;
		short b;
		short c;
	}z;

	// X Y Z of first nav point
	z.a = -548;
	z.b = 0;
	z.c = -57;

	unsigned char x[6];
	memcpy(x, &z, 6);
	for (int i = 0; i < 6; i++)
		printf("%02X ", x[i]);

	printf("\n");
	system("pause");*/

	//=======================================================================


	// Open the ePSXe.exe process
	DWORD procID = GetProcId();
	DWORD baseAddress = GetModuleBaseAddress(procID, L"ePSXe.exe");

	printf("%08X\n", baseAddress);

	if (!procID)
	{
		printf("Failed to find ePSXe.exe\n");
		printf("open ePSXe.exe\n");
		printf("and try again\n");

		system("pause");
		exit(0);
	}

	HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procID);

	// Unlock all cars and tracks immediately
	unsigned long long value = 18446744073709551615;
	WriteProcessMemory(handle, (PBYTE*)(baseAddress + 0xB1070C), &value, sizeof(value), 0);

	int choice = 0;
	printf("1: Server\n");
	printf("2: Client\n");
	printf("Enter: ");
	scanf("%d", &choice);

	if (choice == 1)
	{
		isServer = true;
		MAX_SOCKETS = 9;
		MAX_CONNECTIONS = MAX_SOCKETS - 1;
		serverName = nullptr;
	}
	else
	{
		isClient = true;
		MAX_SOCKETS = 1;
		MAX_CONNECTIONS = 1;
		printf("Enter IP or URL: ");
		serverName = (char*)malloc(80);
		scanf("%79s", serverName);
	}

	printf("Enter Port: ");
	scanf("%d", &PORT);

	// initialize SDL_Net
	SDLNet_Init();

	// initialize socket set
	socketSet = SDLNet_AllocSocketSet(MAX_SOCKETS);

	// set all connections to NULL
	for (int loop = 0; loop < MAX_CONNECTIONS; loop++)
	{
		clientSocket[loop] = NULL;
		socketIsFree[loop] = true; // Set all our sockets to be free (i.e. available for use for new client connections)
	}

	int hostResolved = SDLNet_ResolveHost(&serverIP, serverName, PORT);

	// if you are server, this opens a server socket on your PC
	// if you are client, this opens a connection socket to server
	TCPsocket mySocket = SDLNet_TCP_Open(&serverIP);
	SDLNet_TCP_AddSocket(socketSet, mySocket);

	// Main loop...
	do
	{
		// If you are server
		if (isServer)
		{
			SDLNet_CheckSockets(socketSet, 0);

			// if we see activity on the server socket,
			// which is NOT a client socket, it means
			// somebody is trying to connect
			if (SDLNet_SocketReady(mySocket) != 0)
			{
				if (clientCount < MAX_CONNECTIONS)
				{
					printf("Found connection\n");

					int freeSpot = -99;
					for (int loop = 0; loop < MAX_CONNECTIONS; loop++)
					{
						if (socketIsFree[loop] == true)
						{
							//cout << "Found a free spot at element: " << loop << endl;
							socketIsFree[loop] = false; // Set the socket to be taken
							freeSpot = loop;            // Keep the location to add our connection at that index in the array of client sockets
							break;                      // Break out of the loop straight away
						}
					}

					clientSocket[freeSpot] = SDLNet_TCP_Accept(mySocket);
					SDLNet_TCP_AddSocket(socketSet, clientSocket[freeSpot]);
					clientCount++;

					int len = sprintf(sendBuf, "You are client %d", clientCount);
					SDLNet_TCP_Send(clientSocket[freeSpot], (void *)sendBuf, len+1);
				}
			}

			// check each client for message
			for (int clientNumber = 0; clientNumber < MAX_CONNECTIONS; clientNumber++)
			{
				// if we see activity on the client sockets,
				// it means somebody sent us a message
				if (SDLNet_SocketReady(clientSocket[clientNumber]) != 0)
				{
					// Clearingi before Recv will erase any message
					// that you may want to send (trackID or Start)

					// clear before Recv
					memset(recvBuf, 0, BUFFER_SIZE);
					receivedByteCount = SDLNet_TCP_Recv(clientSocket[clientNumber], recvBuf, BUFFER_SIZE);

					if (receivedByteCount == -1)
						printf("Error: %s", SDLNet_GetError());

					// Get Character ID from Client
					int messageID = 4;
					int kartID = 0;
					if (sscanf(recvBuf, "%d", &messageID) == 1)
					{
						// 3 means Position Message
						if (messageID == 4)
						{
							if (sscanf(recvBuf, "%d %d", &messageID, &kartID) == 2)
							{
								// Get characterID for this player
								// for characters 0 - 7:
								// CharacterID[i] : 0x1608EA4 + 2 * i
								short kartID_short = (short)kartID;
								characterIDs[1] = kartID_short;
							}
						}
					}
					
					// print message if I got one
					//if(receivedByteCount > 0)
					//	printf("From Client #%d: %s\n", clientCount, recvBuf);

					// clear before Send
					//memset(sendBuf, 0, BUFFER_SIZE);
					//sendLength = sprintf(sendBuf, "HelloFromServer");
					int x = SDLNet_TCP_Send(clientSocket[clientNumber], sendBuf, sendLength+1);

					//printf("Sending: %s\n", sendBuf);

					if(x == -1)
						printf("Error: %s", SDLNet_GetError());

					// Disconnect client
					// Put this somewhere

					/*SDLNet_TCP_DelSocket(socketSet, clientSocket[clientNumber]);
					SDLNet_TCP_Close(clientSocket[clientNumber]);
					clientSocket[clientNumber] = NULL;
					socketIsFree[clientNumber] = true;
					clientCount--;*/
				}
			}
		}

		if (isClient)
		{
			SDLNet_CheckSockets(socketSet, 0);

			if (SDLNet_SocketReady(mySocket) != 0)
			{
				// clear before Recv
				memset(recvBuf, 0, BUFFER_SIZE);
				receivedByteCount = SDLNet_TCP_Recv(mySocket, recvBuf, BUFFER_SIZE);

				if (receivedByteCount == -1)
					printf("Error: %s", SDLNet_GetError());


				// print message if I got one
				//if (receivedByteCount != 0)
				//	printf("From Server: %s\n", recvBuf);

				// clear before Send
				//memset(sendBuf, 0, BUFFER_SIZE);
				//sendLength = sprintf(sendBuf, "HelloFromClient");

				int x = SDLNet_TCP_Send(mySocket, sendBuf, sendLength + 1);

				//printf("Sending: %s\n", sendBuf);

				if (x == -1)
					printf("Error: %s", SDLNet_GetError());
			}
		}

		// Check to see if you are in the track selection menu
		bool inTrackSelection = false;
		ReadProcessMemory(handle, (PBYTE*)(baseAddress + 0xB0F8AC), &inTrackSelection, sizeof(inTrackSelection), 0);

		// Unlock all cars and tracks immediately
		unsigned long long value = 18446744073709551615;
		WriteProcessMemory(handle, (PBYTE*)(baseAddress + 0xB1070C), &value, sizeof(value), 0);

		// Get characterID for this player
		// for characters 0 - 7:
		// CharacterID[i] : 0x1608EA4 + 2 * i
		ReadProcessMemory(handle, (PBYTE*)(baseAddress + 0xB08EA4), &characterIDs[0], sizeof(short), 0);
		
		// if you're in the track selection menu
		if (inTrackSelection)
		{
			P1xAddr = -1;
			inRace = false;

			// if you are the server
			if (isServer)
			{
				// check if lapRowSelector is open
				bool lapRowSelectorOpen = false;
				ReadProcessMemory(handle, (PBYTE*)(baseAddress + 0xB379CC), &lapRowSelectorOpen, sizeof(lapRowSelectorOpen), 0);

				// if lap selector is closed
				if (!lapRowSelectorOpen)
				{
					// Get Track ID, send it to clients
					ReadProcessMemory(handle, (PBYTE*)(baseAddress + 0xB3671A), &trackID, sizeof(trackID), 0);

					//printf("Sending to clients: Track %d\n", trackID);

					// 0 means Track Message
					memset(sendBuf, 0, BUFFER_SIZE);
					sendLength = sprintf(sendBuf, "0 %d %d ", trackID, (int)characterIDs[0]);
				}

				// if lap selector is open
				else
				{
					unsigned char menuA = 0;
					unsigned char menuB = 0;
					ReadProcessMemory(handle, (PBYTE*)(baseAddress + 0xB379CE), &menuA, sizeof(menuA), 0);
					ReadProcessMemory(handle, (PBYTE*)(baseAddress + 0xB379D0), &menuB, sizeof(menuB), 0);

					// if race is starting
					if (menuA == 2 && menuB == 1)
					{
						// start the race, tell all clients to start
						P1xAddr = -1;
						NavAddr1 = -1;
						NavAddr2 = -1;
						NavAddr3 = -1;

						printf("Sending to clients: Start Race with X amount of players and Array of characters\n");

						// 2 means Start Message
						memset(sendBuf, 0, BUFFER_SIZE);
						sendLength = sprintf(sendBuf, "2 ");
					}

					// if lap is being chosen
					else
					{
						// 0 -> 3 laps
						// 1 -> 5 laps
						// 2 -> 7 laps
						unsigned char lapRowSelected = 0;
						ReadProcessMemory(handle, (PBYTE*)(baseAddress + 0xB0F940), &lapRowSelected, sizeof(lapRowSelected), 0);

						// 1 means Lap Message
						memset(sendBuf, 0, BUFFER_SIZE);
						sendLength = sprintf(sendBuf, "1 %d", lapRowSelected);
					}
				}
			}

			// if you are the client
			if (isClient)
			{
				// 0 means Track Message
				memset(sendBuf, 0, BUFFER_SIZE);
				sendLength = sprintf(sendBuf, "4 %d ", (int)characterIDs[0]);

				int messageID = 0;
				if (sscanf(recvBuf, "%d", &messageID) == 1)
				{
					// if you get a new trackID
					if (messageID == 0)
					{
						int trackIdFromBuf = 0;
						int kartID = 0;
						if (sscanf(recvBuf, "%d %d %d", &messageID, &trackIdFromBuf, &kartID) == 3)
						{
							// convert to one byte
							char trackByte = (char)trackIdFromBuf;
							
							char zero = 0;
							char ogTrackByte = 0;
							char OneNineOne = 191;
							char TwoFiveFive = 255;

							// Get characterID for this player
							// for characters 0 - 7:
							// CharacterID[i] : 0x1608EA4 + 2 * i
							short kartID_short = (short)kartID;
							characterIDs[1] = kartID_short;

							// close the lapRowSelector
							WriteProcessMemory(handle, (PBYTE*)(baseAddress + 0xB379CC), &zero, sizeof(char), 0);

							// Get original track byte
							ReadProcessMemory(handle, (PBYTE*)(baseAddress + 0xB3671A), &ogTrackByte, sizeof(char), 0);

							// set Text+Map address 
							WriteProcessMemory(handle, (PBYTE*)(baseAddress + 0xB3671A), &trackByte, sizeof(char), 0);

							// set Video Address
							WriteProcessMemory(handle, (PBYTE*)(baseAddress + 0xB379C8), &trackByte, sizeof(char), 0);

							// This is an attempt to update the video
							// in the track-selection menu. It currently
							// does not work, just ignore it for now

							// This does not work
							// Come back to it
							if (ogTrackByte != trackByte)
							{
								// progress of video in menu
								char videoProgress[3] = { 1, 1, 1 };

								// keep hitting "down" until video refreshes and sets to zero
								while (videoProgress[0] != 0 || videoProgress[1] != 0 || videoProgress[2] != 0)
								{
									// read to see the new memory, 12 bytes, 3 ints
									ReadProcessMemory(handle, (PBYTE*)(baseAddress + 0xB20C48), &videoProgress[0], sizeof(char), 0); // first int
									ReadProcessMemory(handle, (PBYTE*)(baseAddress + 0xB20C4C), &videoProgress[1], sizeof(char), 0); // next int
									ReadProcessMemory(handle, (PBYTE*)(baseAddress + 0xB20C50), &videoProgress[2], sizeof(char), 0); // next int

									// Hit the 'Down' button on controller
									WriteProcessMemory(handle, (PBYTE*)(baseAddress + 0x20603D), &OneNineOne, sizeof(char), 0);
									WriteProcessMemory(handle, (PBYTE*)(baseAddress + 0x22C58D), &OneNineOne, sizeof(char), 0);
									WriteProcessMemory(handle, (PBYTE*)(baseAddress + 0x99DDA8), &OneNineOne, sizeof(char), 0);
									WriteProcessMemory(handle, (PBYTE*)(baseAddress + 0xB18AF8), &OneNineOne, sizeof(char), 0);
									WriteProcessMemory(handle, (PBYTE*)(baseAddress + 0xB21630), &OneNineOne, sizeof(char), 0);
								}
							}
						}
					}

					// Not Done
					// if you get a lapRow message 
					if (messageID == 1)
					{
						int lapIdFromBuf = 0;
						if (sscanf(recvBuf, "%d %d", &messageID, &lapIdFromBuf) == 2)
						{
							// open the lapRowSelector menu, 
							char one = 1;
							WriteProcessMemory(handle, (PBYTE*)(baseAddress + 0xB379CC), &one, sizeof(char), 0);

							// convert to one byte
							char lapByte = (char)lapIdFromBuf;
							WriteProcessMemory(handle, (PBYTE*)(baseAddress + 0xB0F940), &lapByte, sizeof(lapByte), 0);

							// change the spawn order
							
							// Server:   0 1 2 3 4 5 6 7
							// Client 1: 1 0 2 3 4 5 6 7
							// Client 2: 1 2 0 3 4 5 6 7
							// Client 3: 1 2 3 0 4 5 6 7

							// this will change when we have more than 2 players
							char clientNum = 1;

							// set all positions before client
							for (char i = 0; i < clientNum; i++)
							{
								char value = i + 1;
								WriteProcessMemory(handle, (PBYTE*)(baseAddress + 0xB02F48 + i), &value, sizeof(char), 0);
							}

							// set the client
							char value = clientNum;
							WriteProcessMemory(handle, (PBYTE*)(baseAddress + 0xB02F48 + clientNum), &value, sizeof(char), 0);

							// set all positions after client
							for (char i = clientNum + 1; i < 8; i++)
							{
								char value = i + 1;
								WriteProcessMemory(handle, (PBYTE*)(baseAddress + 0xB02F48 + i), &value, sizeof(char), 0);
							}

							// Write to memory
						}
					}

					// Not Done
					// if you get the message to start
					if (messageID == 2)
					{
						char one = 1;
						char two = 2;

						// set menuA to 2 and menuB to 1,
						WriteProcessMemory(handle, (PBYTE*)(baseAddress + 0xB379CE), &two, sizeof(char), 0);
						WriteProcessMemory(handle, (PBYTE*)(baseAddress + 0xB379D0), &one, sizeof(char), 0);

						// This message will include number of players
						// and array of characterIDs, save it for later
						// Stop looking for messages until later
					}
				}
			}
		}

		// if you're not in the track selection menu
		else
		{
			// set all characters
			for (int i = 1; i < 2; i++)
			{
				char oneByte = (char)characterIDs[i];
				WriteProcessMemory(handle, (PBYTE*)(baseAddress + 0xB08EA4 + 2*i), &oneByte, 1, 0); // 4, for 2 shorts
			}

			gameStatePrev = gameStateCurr;

			// Read gameStateCurr
			ReadProcessMemory(handle, (PBYTE*)(baseAddress + 0xB1A871), &gameStateCurr, sizeof(gameStateCurr), 0);

			// This only happens when loading ends before race, no more bugs
			if (gameStatePrev == 2 && gameStateCurr == 10 && P1xAddr == -1)
			{
				printf("First frame of intro cutscene\n");

				int min = 0;
				int max = 0;

				// However, P1xAddr needs 81920 iterations.
				// Scanning for Path1 requires 96054 iterations.

				// min C40 will fail on coco park
				// max C70 will fail on most tracks
				min = baseAddress + 0xC30000; 
				max = baseAddress + 0xC80000; 

				// Get Start Line Positions
				for (int i = 0; i < max - min; ) // put nothing in the end
				{
					int dataInt[3];
					ReadProcessMemory(handle, (PBYTE*)(min + i + 0x0), &dataInt[0], sizeof(int), 0);
					ReadProcessMemory(handle, (PBYTE*)(min + i + 0x4), &dataInt[1], sizeof(int), 0);
					ReadProcessMemory(handle, (PBYTE*)(min + i + 0x8), &dataInt[2], sizeof(int), 0);

					if (dataInt[0] == P1_StartPos[3 * trackID + 0])
						if (dataInt[1] == P1_StartPos[3 * trackID + 1])
							if (dataInt[2] == P1_StartPos[3 * trackID + 2])
							{
								P1xAddr = min + i;
								printf("Found P1xAddr: %p\n", P1xAddr - baseAddress);
								break;
							}
					i += 4;
				}

				if (P1xAddr == -1)
				{
					printf("Fail to find P1xAddr\n");
					continue;
				}

				// calculate how many total nav points there are
				int totalPoints =
					numNodesInPaths[3 * trackID + 0] +
					numNodesInPaths[3 * trackID + 1] +
					numNodesInPaths[3 * trackID + 2];

				// get the nav addresses
				NavAddr1 = P1xAddr - totalPoints * 20 - 63200;
				NavAddr2 = NavAddr1 + numNodesInPaths[3 * trackID + 0] * 20 + 0x60;
				NavAddr3 = NavAddr2 + numNodesInPaths[3 * trackID + 1] * 20 + 0x60;


				// if you're the client
				if (0)
				{
					// When the loading screen started, you stopped listening for messages

					// Now, start listening for messsages again
				}

				// Set Text
				unsigned char title[] = "Online";
				WriteProcessMemory(handle, (PBYTE*)(baseAddress + 0xB3EC79), &title, 6, 0);

				// Read the original location of player 1
				ReadProcessMemory(handle, (PBYTE*)(P1xAddr + 0), &StartLinePos[0][0], sizeof(int), 0);
				ReadProcessMemory(handle, (PBYTE*)(P1xAddr + 4), &StartLinePos[0][1], sizeof(int), 0);
				ReadProcessMemory(handle, (PBYTE*)(P1xAddr + 8), &StartLinePos[0][2], sizeof(int), 0);
				printf("1: %u %u %u\n", StartLinePos[0][0], StartLinePos[0][1], StartLinePos[0][2]);

				// Read the original location of player 2-8
				for (int i = 0; i < 7; i++)
				{
					// AI[n]x = P1x - 0x354 - 0x670 * n
					ReadProcessMemory(handle, (PBYTE*)(P1xAddr - 0x354 - 0x670 * i + 0), &StartLinePos[i + 1][0], sizeof(int), 0);
					ReadProcessMemory(handle, (PBYTE*)(P1xAddr - 0x354 - 0x670 * i + 4), &StartLinePos[i + 1][1], sizeof(int), 0);
					ReadProcessMemory(handle, (PBYTE*)(P1xAddr - 0x354 - 0x670 * i + 8), &StartLinePos[i + 1][2], sizeof(int), 0);
					printf("%d: %u %u %u\n", i + 2, StartLinePos[i + 1][0], StartLinePos[i + 1][1], StartLinePos[i + 1][2]);
				}

				inRace = true;
			}

			if (inRace)
			{
				// If the race has not started yet
				if (gameStateCurr != 11)
				{
					// If not all racers are ready to start
					if (0)
					{
						// set the countdown traffic-lights to 4000
						// Addresss = 0x161A84C, 2 bytes large
					}
				}

				/*
					Player 1:
					rotX = posX + 0x96 (only for P1)
					rotY = posX + 0xC6 (only for P1)
					There is no RotZ (gimbal lock)
					DistanceToFinish: posX + 0x1B4
					Weapon: posX - 0x29E
				*/

				// Get Current and Previous weapon state
				weaponPrev = weaponCurr;
				ReadProcessMemory(handle, (PBYTE*)(P1xAddr - 0x29E), &weaponCurr, sizeof(char), 0);

				// If you were cycling through weapons, then stopped
				if (weaponPrev == 0x10 && weaponCurr != 0x10)
				{
					// Set weapon to null, so nobody uses weapons
					char NoWeapon = 0xF;
					WriteProcessMemory(handle, (PBYTE*)(P1xAddr - 0x29E), &NoWeapon, sizeof(char), 0);
				}

				// Get Player 1 position
				unsigned int data[3];
				ReadProcessMemory(handle, (PBYTE*)(P1xAddr + 0x0), &data[0], sizeof(int), 0);
				ReadProcessMemory(handle, (PBYTE*)(P1xAddr + 0x4), &data[1], sizeof(int), 0);
				ReadProcessMemory(handle, (PBYTE*)(P1xAddr + 0x8), &data[2], sizeof(int), 0);

				// Divide by 256
				short data2[3];
				data2[0] = data[0] / 256;
				data2[1] = data[1] / 256;
				data2[2] = data[2] / 256;

				// Server sends to client
				// Client sends to server
				// 3 means Position Message
				memset(sendBuf, 0, BUFFER_SIZE);
				sendLength = sprintf(sendBuf, "3 %d %d %d", data2[0], data2[1], data2[2]);

				// Server gets from client
				// Client gets from server
				// Parse the position from the message
				// set all nav points to that position
				int messageID = 0;
				if (sscanf(recvBuf, "%d", &messageID) == 1)
				{
					// 3 means Position Message
					if (messageID == 3)
					{
						if (sscanf(recvBuf, "%d %d %d %d", &messageID, &data2[0], &data2[1], &data2[2]) == 4)
						{
							printf("%d %d %d\n", data2[0], data2[1], data2[2]);

							for (int i = 0; i < numNodesInPaths[3 * trackID + 0]; i++)
							{
								WriteProcessMemory(handle, (PBYTE*)(NavAddr1 + (i * 0x14) + 0), &data2[0], 2, 0);
								WriteProcessMemory(handle, (PBYTE*)(NavAddr1 + (i * 0x14) + 2), &data2[1], 2, 0);
								WriteProcessMemory(handle, (PBYTE*)(NavAddr1 + (i * 0x14) + 4), &data2[2], 2, 0);
							}

							for (int i = 0; i < numNodesInPaths[3 * trackID + 1]; i++)
							{
								WriteProcessMemory(handle, (PBYTE*)(NavAddr2 + (i * 0x14) + 0), &data2[0], 2, 0);
								WriteProcessMemory(handle, (PBYTE*)(NavAddr2 + (i * 0x14) + 2), &data2[1], 2, 0);
								WriteProcessMemory(handle, (PBYTE*)(NavAddr2 + (i * 0x14) + 4), &data2[2], 2, 0);
							}

							for (int i = 0; i < numNodesInPaths[3 * trackID + 2]; i++)
							{
								WriteProcessMemory(handle, (PBYTE*)(NavAddr3 + (i * 0x14) + 0), &data2[0], 2, 0);
								WriteProcessMemory(handle, (PBYTE*)(NavAddr3 + (i * 0x14) + 2), &data2[1], 2, 0);
								WriteProcessMemory(handle, (PBYTE*)(NavAddr3 + (i * 0x14) + 4), &data2[2], 2, 0);
							}
						}
					}
				}

				// Set all players
				for (int i = 0; i < 1; i++)
				{
					// AI[n]x = P1x - 0x354 - 0x670 * n
					int aiX = P1xAddr - 0x354 - 0x670 * i;

					// Set the Nav ID of the player
					// Do not know how yet
				}

				// Don't teleport AI #0 to oblivion, we want
				// that player to be on the track

				// AI
				/*
					int ai_posX = P1xAddr - 0x354 - 0x670 * i;
					i = 0,1,2,3,4,5,6
					for all 7 AIs in the game

					DistanceToFinish: posX - 0x168 (4 bytes)
					This is large when the lap starts and
					small when the lap ends. If this is less 
					than (or equal to) zero, the next lap starts

					Freeze1: posX - 0x1C (4 bytes)
					Freeze2: posX - 0x48 (4 bytes)
					Related to velocity?

					Track Progress: posX - 0x4C (2 bytes)
					This determines if you're in 1st, 2nd, 3rd, etc.
					This is used to trigger rotation, steering,
					power sliding, changing color to match environment
					(like the Coco Park tunnel)
				*/

				// Move unwanted players into oblivion
				for (int i = 1; i < 7; i++)
				{
					int Gone = 99999999;
					int aiX = P1xAddr - 0x354 - 0x670 * i;

					WriteProcessMemory(handle, (PBYTE*)(aiX + 0), &Gone, sizeof(int), 0);
					WriteProcessMemory(handle, (PBYTE*)(aiX + 4), &Gone, sizeof(int), 0);
					WriteProcessMemory(handle, (PBYTE*)(aiX + 8), &Gone, sizeof(int), 0);
				}

				// read all positions from all clients
				// send all positions to all clients
				// write positions to the emulator
			}
		}
		
		// 60fps means 16ms per frame
		// sleep for 2ms so that network ins't clogged
		Sleep(2);

	} while (shutdownServer == false); // End of main loop

	SDLNet_FreeSocketSet(socketSet);
	SDLNet_TCP_Close(mySocket);
	SDLNet_Quit();
	return 0;
}