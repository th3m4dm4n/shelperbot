#include <iostream>
#include <cstring>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctime>
#include <curl/curl.h>
#include <ctime>
#include <jsmn.h>

#define MAXDATASIZE 256
#define FLAGCOUNT 3

#include "curlbuffer.cpp"
#include "4chanurl.cpp"
#include "4chanposttext.cpp"
#include "4chanimage.cpp"
#include "8chanurl.cpp"
#include "8chanposttext.cpp"

using namespace std;

void handleMessage( string Message)
{
}

int handlePossibleFlag( string PossibleFlag)
{
	if ( PossibleFlag.compare( "--no-image-text") == 0)
	{
		return 1;
	}
	else
	{
		return -1;
	}
}

bool findFlag( int Flags[FLAGCOUNT], int TargetFlag)
{
	for ( int i = 0; i < FLAGCOUNT; i++)
	{
		if ( Flags[i] == TargetFlag)
		{
			return true;
		}
	}
	return false;
}

bool sendData( int BotSocket, string Message)
{
	int BytesSent = send( BotSocket, Message.c_str(), Message.length(), 0);
	cout << Message << endl;
	if ( BytesSent == 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

char* prepareCommand( string Command, string Message)
{
	Message.insert( 0, Command);
	Message.append( "\r\n");
	char* Message_out = ( char*) Message.c_str();
	return Message_out;
}

void sayMessage( int BotSocket, string Message, string Channel)
{
	Channel.append( " :");
	Channel.append( Message);
	sendData( BotSocket, prepareCommand( "PRIVMSG ", Channel));
}

int connectSocket( char* TargetName, char* TargetPort)
{
	struct addrinfo StuffAboutTarget, *TargetInfo;

	memset( &StuffAboutTarget, 0, sizeof StuffAboutTarget);

	StuffAboutTarget.ai_family = AF_UNSPEC;
	StuffAboutTarget.ai_socktype = SOCK_STREAM;
	
	int ProbsWithTarget = getaddrinfo( TargetName, TargetPort, &StuffAboutTarget, &TargetInfo);
	if ( ProbsWithTarget != 0)
	{
		cerr << "getaddrinfo problem: " << gai_strerror( ProbsWithTarget) << endl;
		exit(0);
	}

	int Socket = socket( TargetInfo->ai_family, TargetInfo->ai_socktype, TargetInfo->ai_protocol);
	if ( Socket < 0)
	{
		cerr << "Error initializing socket\n";
		exit(0);
	}

	if ( connect( Socket, TargetInfo->ai_addr, TargetInfo->ai_addrlen) < 0)
	{
		close( Socket);
		cerr << "Error connecting to target\n";
		exit(0);
	}

	freeaddrinfo( TargetInfo);

	return Socket;
}


int main( int argc, char** argv)
{
	int Flags[FLAGCOUNT];
	for ( int i = 0; i < FLAGCOUNT; i++)
	{
		Flags[i] = -1;
	}
	int f = 0;
	
	string argv_s[argc];
	for ( int j = 0; j < argc; j++)
	{
		argv_s[j] = ( string) argv[j];
	}
	
	for ( int ArgsRead = 1; ArgsRead < argc; ArgsRead++)
	{
		if ( argv_s[ArgsRead].find( "--") != -1)
		{
			Flags[f] = handlePossibleFlag( argv_s[ArgsRead]);
			argv[ArgsRead] = ( char*) argv_s[ArgsRead + 1].c_str();
			f++;
			argc--;
		}
	}
	if ( argc != 5)
	{
		cout << "usage: " << argv[0] << " server port nickname channel\n";
		exit(0);
	}
	for ( int i = 0; i < FLAGCOUNT; i++)
	{
		cout << Flags[i] << endl;
	}
	
	char* IRCServer = argv[1]; //the first argument will be the target name
	char* IRCPort = argv[2]; //the second argument will be the target port
	string BotNick = string ( argv[3]);
	string TargetChannel = string ( argv[4]);
	
	int BotSocket = connectSocket( IRCServer, IRCPort);
	
	int RecieveCount = 0;
	int ByteCount;
	char Buffer[MAXDATASIZE];

	while(1)
	{
		RecieveCount++;
		

		if ( RecieveCount == 2)
		{
			sendData( BotSocket, prepareCommand( "NICK ", BotNick));
			string BotNick_user = BotNick;
			BotNick_user.append(" 0 * :");
			BotNick_user.append( BotNick);
			sendData( BotSocket, prepareCommand( "USER ", BotNick_user));
		}
		if ( RecieveCount == 10)
		{
			cout << TargetChannel << endl;
			sendData( BotSocket, prepareCommand( "JOIN ", TargetChannel));
		}
		ByteCount = recv( BotSocket, Buffer, MAXDATASIZE - 1, 0);
		Buffer[ByteCount] = '\0';//we string now
		string Buffer_s = string (Buffer);
		cout << Buffer_s;


		handleMessage( Buffer_s);

		if ( Buffer_s.find( "PING", 0) != -1)
		{ 
			Buffer_s.erase( 0, 6);
			sendData( BotSocket, prepareCommand( "PONG :", Buffer_s));
		}
		string BotNickCommandFormat = BotNick;
	//	cout << "My name is " << BotNick << endl;
		BotNickCommandFormat.append( "...IRC ");
	//	cout << "I respond to " << BotNickCommandFormat << endl;
		if ( Buffer_s.find( BotNickCommandFormat, 0) != -1)
		{ 
			if ( Buffer_s.find( "NICK ") != -1)
			{
				string BotNickNew = Buffer_s;
				BotNickNew.erase( 0, BotNickNew.find( "NICK ") + 5);
				BotNickNew.erase( BotNickNew.find( "\r"), BotNickNew.find( "\r"));
				cout << "Nick changed to " << BotNickNew << endl;
				BotNick = BotNickNew;
			}
			
			string BotCommand = Buffer_s;
			BotCommand.erase( 0, Buffer_s.find( BotNickCommandFormat, 0) + BotNickCommandFormat.size());
			sendData( BotSocket, BotCommand);
		//	sayMessage( BotSocket, BotCommand, TargetChannel);
		}
	
		if ( Buffer_s.find( "://boards.4chan.org/") != -1)
		{
			struct fourchanpost_t PostInfo = getFourChanPostInfo( Buffer_s);
			string FourChanPostText = grabFourChanPostText( PostInfo);	
			
			if ( FourChanPostText != "\r\b\r")
			{
				sayMessage( BotSocket, FourChanPostText, TargetChannel);
			}
		}
		
		if ( Buffer_s.find( "://8chan.co/") != -1)
		{
			struct eightchanpost_t PostInfo = getEightChanPostInfo( Buffer_s);
			string EightChanPostText = grabEightChanPostText( PostInfo);	
			
			if ( EightChanPostText != "\r\b\r")
			{
				sayMessage( BotSocket, EightChanPostText, TargetChannel);
			}
		}

		string BotKick = "KICK ";
		BotKick.append( TargetChannel);
		if ( Buffer_s.find( BotKick) != -1)
		{
			sleep( 3);
			string RejoinCommand = "JOIN ";
			RejoinCommand.append( TargetChannel);
			cout << RejoinCommand << endl;
			sendData( BotSocket, prepareCommand( RejoinCommand, ""));
		}

		if ( Buffer_s.find( "://i.4cdn.org/") != -1)
		{
			
			struct ImageInfoResults Results = getFourChanImageInfo( Buffer_s);
			struct fourchanpost_t ImagePostTarget = Results.ImageInfo;
			string Message = Results.Message;
			if ( findFlag( Flags, 1) == false)
			{
				Message.append( " ---> ");
				Message.append( grabFourChanPostText( ImagePostTarget));
			}
			sayMessage( BotSocket, Message, TargetChannel); 
		}
		
		string BotNickPrivMsg = "PRIVMSG ";
		BotNickPrivMsg.append( BotNick);
		if ( ( Buffer_s.find( "PRIVMSG", 0) != -1) && ( Buffer_s.find( BotNick, Buffer_s.find( "PRIVMSG", 0)) != -1) && ( Buffer_s.find( BotNickPrivMsg, 0) == -1))
		{
			srand( time( NULL));
			string ReplyList[6];
			ReplyList[0] = "hi punk";
			ReplyList[1] = "ok bub";
			ReplyList[2] = "sure thing nerd";
			ReplyList[3] = "absolutely";
			ReplyList[4] = "well of course";
			ReplyList[5] = "hello newfriend";
			sayMessage( BotSocket, (string ) ReplyList[rand() % 6], TargetChannel);
		}


		if ( ByteCount == 0)
		{
			cout << "CONNECTION CLOSED\n";
			break;
		}
	}



}
