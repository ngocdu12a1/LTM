#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#include <Windows.h>
#include <string.h>
#include <iostream>


#define server_addr "127.0.0.1"
#define welcome "220 welcome to ftp server on windown.\r\n"
using namespace std;

DWORD WINAPI ClientThread(LPVOID arg);

struct ftpClient {
	char cmd[256], curdic[256];
	SOCKET data_sk, ctrl_sk;
	int login, permission;

	ftpClient(SOCKET c) {
		ctrl_sk = c;
		login = permission = -1;
	}
};

SOCKET datasocket, sendata;

void resUser(SOCKET c) {
	char response[1024] = "331 Please spcify the password.\r\n";
	send(c, response, sizeof(response), 0);
}

void resPass(SOCKET c) {
	char response[1024] = "230 Login succesfull.\r\n";
	send(c, response, sizeof(response), 0);
}

void resOPTS(SOCKET c) {
	char response[1024] = "200 Always in UTF8 mode.\r\n";
	send(c, response, sizeof(response), 0);
}

void resPwd(SOCKET c) {
	char response[1024] = "257 \"\/\" \r\n";
	send(c, response, sizeof(response), 0);
}

void resCwd(SOCKET c) {
	char response[1024] = "250 Directory succesfully changed.\r\n";
	send(c, response, sizeof(response), 0);
}

void resSYST(SOCKET c) {
	char response[1024] = "215  Windows_NT.\r\n";
	send(c, response, sizeof(response), 0);
}

void resPasv(SOCKET c) {

	char response[1024] = "227 Entering Passive Mode (127,0,0,1,198,47).\r\n";
	send(c, response, sizeof(response), 0);


	datasocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	/**
		Reuse host
	*/
	
	SOCKADDR_IN saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(50735);
	saddr.sin_addr.s_addr = inet_addr(server_addr);

	bind(datasocket, (sockaddr*)&saddr, sizeof(saddr));

	int res = 0;
	
	listen(datasocket, 10);


	SOCKADDR_IN caddr;
	int client = sizeof(caddr);

	sendata = accept(datasocket, (sockaddr*)&caddr, &client);

	cout << "connect data succesfully" << endl;
}

void resList(SOCKET c) {
	char response[1024] = "150 Here comes the directory listing.\r\n";
	send(c, response, sizeof(response), 0);

	char dir[1024] = "drwxrwx---+ 1 Administrators              SYSTEM                               0 Nov 23 21:53 '$AV_AVG'\r\n";

	SOCKADDR_IN caddr;
	int client = sizeof(caddr);
	

//	send(datasocket, dir, sizeof(dir), 0);
	send(sendata, dir, sizeof(dir), 0);

//	closesocket(datasocket);
	closesocket(sendata);

	char response1[1024] = "226 Directory send OK.\r\n";
	send(c, response1, sizeof(response1), 0);
}

void resType(SOCKET c) {
	char response[1024] = "200 Switching to Binary mode.\r\n";
	send(c, response, sizeof(response), 0);
}
void resFeat(SOCKET c) {
	char start[1024] = "211-Features:\r\n";
	char f1[1024] = "EPRT\r\n"; char f2[1024] = "EPSV\r\n";
	char f3[1024] = "MDTM\r\n"; char f4[1024] = "REST STREAM\r\n";
	char f5[1024] = "SIZE\r\n"; char f6[1024] = "TVFS\r\n";
	char f7[1024] = "UTF8\r\n"; 
	char end[1024] = "211 End.\r\n";

	send(c, start, sizeof(start), 0);
	send(c, f1, sizeof(f1), 0);  send(c, f2, sizeof(f2), 0);
	send(c, f3, sizeof(f3), 0);  send(c, f4, sizeof(f4), 0);
	send(c, f5, sizeof(f5), 0);  send(c, f6, sizeof(f6), 0);
	send(c, f7, sizeof(f7), 0); 
	send(c, end, sizeof(end), 0);
}
void handle_client(SOCKET c, char cmd[1024]) {
	if (strcmp(cmd, "USER") == 0) resUser(c);
	else if (strcmp(cmd, "PASS") == 0) resPass(c);
    else if (strcmp(cmd, "OPTS") == 0) resOPTS(c);
	else if (strcmp(cmd, "PWD") == 0) resPwd(c);
	else if (strcmp(cmd, "CWD") == 0) resCwd(c);
	else if (strcmp(cmd, "SYST") == 0) resSYST(c);
	else if (strcmp(cmd, "PASV") == 0) resPasv(c);
	else if (strcmp(cmd, "LIST") == 0) resList(c);
	else if (strcmp(cmd, "FEAT") == 0) resFeat(c);
	else if (strcmp(cmd, "TYPE") == 0) resType(c);
}

void Handle_client(ftpClient clen) {

}

int main()
{
	WSADATA DATA;
	WSAStartup(MAKEWORD(2, 2), &DATA);

	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKADDR_IN saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(5000);
	saddr.sin_addr.s_addr = inet_addr(server_addr);

	bind(s, (sockaddr*)&saddr, sizeof(saddr));
	listen(s, 10);

	while (0 == 0) {
		SOCKADDR_IN caddr;
		int clen = sizeof(caddr);
		SOCKET c = accept(s, (sockaddr*)&caddr, &clen);
		DWORD ID = 0;
		CreateThread(NULL, 0, ClientThread, (LPVOID)c, 0, &ID);
	}
}

DWORD WINAPI ClientThread(LPVOID arg) {
	SOCKET c = (SOCKET)arg;
	ftpClient client = ftpClient(c);
	send(client.ctrl_sk, welcome, strlen(welcome), 0);

	char buffer[1024], verb[1024];

	while (0 == 0) {
		memset(buffer, 0, 1024);
		recv(client.ctrl_sk, buffer, 1023, 0);
		sscanf(buffer, "%s", verb);
		if (strcmp(verb, "QUIT") == 0) {

		}
		else {
			Handle_client(client);
		}
	}
}