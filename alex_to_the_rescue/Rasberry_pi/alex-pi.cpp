#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdint.h>
#include "packet.h"
#include "serial.h"
#include "serialize.h"
#include "constants.h"
#include <termios.h>

#define PORT_NAME			"/dev/ttyACM0"
#define BAUD_RATE			B9600

int exitFlag=0;
sem_t _xmitSema;

void handleError(TResult error)
{
	switch(error)
	{
		case PACKET_BAD:
			printf("ERROR: Bad Magic Number\n");
			break;

		case PACKET_CHECKSUM_BAD:
			printf("ERROR: Bad checksum\n");
			break;

		default:
			printf("ERROR: UNKNOWN ERROR\n");
	}
}

void handleStatus(TPacket *packet)
{
	printf("\n ------- ALEX STATUS REPORT ------- \n\n");
	printf("Left Forward Ticks:\t\t%d\n", packet->params[0]);
	printf("Right Forward Ticks:\t\t%d\n", packet->params[1]);
	printf("Left Reverse Ticks:\t\t%d\n", packet->params[2]);
	printf("Right Reverse Ticks:\t\t%d\n", packet->params[3]);
	printf("Left Forward Ticks Turns:\t%d\n", packet->params[4]);
	printf("Right Forward Ticks Turns:\t%d\n", packet->params[5]);
	printf("Left Reverse Ticks Turns:\t%d\n", packet->params[6]);
	printf("Right Reverse Ticks Turns:\t%d\n", packet->params[7]);
	printf("Forward Distance:\t\t%d\n", packet->params[8]);
	printf("Reverse Distance:\t\t%d\n", packet->params[9]);
	printf("\n---------------------------------------\n\n");
}

void handleDist(TPacket *packet) {
	uint32_t distance = packet->params[0];
	printf("Ultrasonic Distance:\t\t%d cm\n", distance);
}

void handleColour(TPacket *packet){
	uint32_t red = packet->params[0];
	uint32_t green = packet->params[1];
	uint32_t blue = packet->params[2];

	printf("\n --------- ALEX COLOR SENSOR --------- \n\n");
	printf("Red (R) frequency:\t%d\n", red);
	printf("Green (G) frequency:\t%d\n", green);
	printf("Blue (B) frequency:\t%d\n", blue);

	uint32_t mapred;
	uint32_t mapgreen;
	uint32_t mapblue;

	if(red<=100){
		mapred = 0;
	}
	else if(red>100 && red<=200){
		mapred = 1;
	}
	else{
		mapred = 2;
	}
	if(green<=100){
		mapgreen = 0;
	}
	else if(green>100 && green<=200){
		mapgreen = 1;
	}
	else{
		mapgreen = 2;
	}
	if(blue <= 100){
		mapblue = 0;
	}
	else if(blue>100 && blue<=200){
		mapblue = 1;
	}
	else{
		mapblue = 2;
	}
	if(mapred == 2 && mapgreen == 2 && mapblue == 2){
		printf("White\n");
	}

	else if(red > green){
		printf("Red\n");
	}
	else if(green > red){		printf("Green\n");
	}
	else{
		printf("IDK\n");
	}
}

void handleResponse(TPacket *packet)
{
	// The response code is stored in command
	switch(packet->command)
	{
		case RESP_OK:
			printf("Command OK\n");
			break;

		case RESP_STATUS:
			handleStatus(packet);
			break;
		case RESP_DIST:
			handleDist(packet);
			break;
		case RESP_COLOUR:
			handleColour(packet);
			break;
		default:
			printf("Arduino is confused\n");
	}
}

void handleErrorResponse(TPacket *packet)
{
	// The error code is returned in command
	switch(packet->command)
	{
		case RESP_BAD_PACKET:
			printf("Arduino received bad magic number\n");
			break;

		case RESP_BAD_CHECKSUM:
			printf("Arduino received bad checksum\n");
			break;

		case RESP_BAD_COMMAND:
			printf("Arduino received bad command\n");
			break;

		case RESP_BAD_RESPONSE:
			printf("Arduino received unexpected response\n");
			break;

		default:
			printf("Arduino reports a weird error\n");
	}
}

void handleMessage(TPacket *packet)
{
	printf("Message from Alex: %s\n", packet->data);
}

void handlePacket(TPacket *packet)
{
	switch(packet->packetType)
	{
		case PACKET_TYPE_COMMAND:
			// Only we send command packets, so ignore
			break;

		case PACKET_TYPE_RESPONSE:
			handleResponse(packet);
			break;

		case PACKET_TYPE_ERROR:
			handleErrorResponse(packet);
			break;

		case PACKET_TYPE_MESSAGE:
			handleMessage(packet);
			break;
	}
}

void sendPacket(TPacket *packet)
{
	char buffer[PACKET_SIZE];
	int len = serialize(buffer, packet, sizeof(TPacket));

	serialWrite(buffer, len);
}

void *receiveThread(void *p)
{
	char buffer[PACKET_SIZE];
	int len;
	TPacket packet;
	TResult result;
	int counter=0;

	while(1)
	{
		len = serialRead(buffer);
		counter+=len;
		if(len > 0)
		{
			result = deserialize(buffer, len, &packet);

			if(result == PACKET_OK)
			{
				counter=0;
				handlePacket(&packet);
				printf("Packet ok\n");
			}
			else 
				if(result != PACKET_INCOMPLETE)
				{
					printf("PACKET ERROR\n");
					handleError(result);
				}
		}
	}
}

void flushInput()
{
	char c;

	while((c = getchar()) != '\n' && c != EOF);
}

/*void getParams(TPacket *commandPacket)
  {
  printf("Enter distance/angle in cm/degrees (e.g. 50) and power in %% (e.g. 75) separated by space.\n");
  printf("E.g. 50 75 means go at 50 cm at 75%% power for forward/backward, or 50 degrees left or right turn at 75%%  power\n");
  scanf("%d %d", &commandPacket->params[0], &commandPacket->params[1]);
  flushInput();
  }*/

void sendCommand(char command)
{
	TPacket commandPacket;

	commandPacket.packetType = PACKET_TYPE_COMMAND;

	switch(command)
	{
		case 'w':
		case 'W':
			//getParams(&commandPacket);
			commandPacket.command = COMMAND_FORWARD;
			sendPacket(&commandPacket);
			break;
		case 'f':
		case 'F':
			commandPacket.command = COMMAND_NUDGE_FORWARD;
			sendPacket(&commandPacket);
			break;
		case 'e':
		case 'E':
			commandPacket.command = COMMAND_SPEED_FORWARD;
			sendPacket(&commandPacket);
			break;

		case 's':
		case 'S':
			//getParams(&commandPacket);
			commandPacket.command = COMMAND_REVERSE;
			sendPacket(&commandPacket);
			break;

		case 'b':
		case 'B':
			commandPacket.command = COMMAND_NUDGE_REVERSE;
			sendPacket(&commandPacket);
			break;
		
		case 'z':
		case 'Z':
			commandPacket.command = COMMAND_SPEED_REVERSE;
			sendPacket(&commandPacket);
			break;

		case 'd':
		case 'D':
			//getParams(&commandPacket);
			commandPacket.command = COMMAND_TURN_LEFT;
			sendPacket(&commandPacket);
			break;
                case 'k':
		case 'K':
			//getParams(&commandPacket);
			commandPacket.command = COMMAND_SPEED_LEFT;
			sendPacket(&commandPacket);
			break;

		case 'a':
		case 'A':
			//getParams(&commandPacket);
			commandPacket.command = COMMAND_TURN_RIGHT;
			sendPacket(&commandPacket);
			break;
		case 'j':
		case 'J':
			//getParams(&commandPacket);
			commandPacket.command = COMMAND_SPEED_RIGHT;
			sendPacket(&commandPacket);
			break;

		case 'x':
		case 'X':
			commandPacket.command = COMMAND_STOP;
			sendPacket(&commandPacket);
			break;

		case 'g':
		case 'G':
			printf("GET DISTANCE\n");
			commandPacket.command = COMMAND_DIST;
			sendPacket(&commandPacket);
			break;

		case 'c':
		case 'C':
			commandPacket.command = COMMAND_COLOUR;
			sendPacket(&commandPacket);
			break;
		case 'p':
		case 'P':
			commandPacket.command = COMMAND_BUZZER;
			sendPacket(&commandPacket);
			break;

		
		case 'q':
		case 'Q':
			exitFlag=1;
			break;

		default:
			printf("Bad command\n");

	}
}

int main()
{
	// Connect to the Arduino
	startSerial(PORT_NAME, BAUD_RATE, 8, 'N', 1, 5);

	// Sleep for two seconds
	printf("WAITING TWO SECONDS FOR ARDUINO TO REBOOT\n");
	sleep(2);
	printf("DONE\n");

	// Spawn receiver thread
	pthread_t recv;

	pthread_create(&recv, NULL, receiveThread, NULL);

	// Send a hello packet
	TPacket helloPacket;

	helloPacket.packetType = PACKET_TYPE_HELLO;
	sendPacket(&helloPacket);

	struct termios old_tio, new_tio;
	tcgetattr(STDIN_FILENO, &old_tio);
	char ch;
	new_tio = old_tio;
	new_tio.c_lflag &= (~ICANON & ~ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

	while(!exitFlag)
	{
		char ch;
		printf("Command (W=forward, S=reverse, A=turn left, D=turn right, G=get distance, C=Check Colour, F=Nudge forward, B=Nudge reverse)\n");
		//scanf("%c", &ch);
		ch = getchar();

		// Purge extraneous characters from input stream
		//flushInput();

		sendCommand(ch);
	}
	tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);

	printf("Closing connection to Arduino.\n"); endSerial();
}
