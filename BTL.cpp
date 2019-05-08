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
	char verb[1024], curdic[1024];
	SOCKET data_sk, ctrl_sk;
	int login, permission;

	ftpClient(SOCKET c) {
		ctrl_sk = c;
		login = permission = -1;
	}
};

SOCKET datasocket, sendata;

void sendResponse(ftpClient client, char* res) {
	send(client.ctrl_sk, res, strlen(res), 0);
}

void sendDir(ftpClient client, char* dir) {
	send(client.data_sk, dir, strlen(dir), 0);
}
void resUser(ftpClient client) {
	sendResponse(client, "331 Please spcify the password.\r\n");
}

void resPass(ftpClient client) {
	sendResponse(client, "230 Login succesfull.\r\n");

}

void resOPTS(ftpClient client) {
	sendResponse(client, "200 Always in UTF8 mode.\r\n");
}

void resPwd(ftpClient client) {
	sendResponse(client, "257 \"\D:/\" \r\n");
}

void resCwd(ftpClient client) {
	sendResponse(client, "250 Directory succesfully changed.\r\n");
}

void resSYST(ftpClient client) {
	sendResponse(client, "215  Windows_NT.\r\n");
}

void resPasv(ftpClient& client) {

	sendResponse(client, "227 Entering Passive Mode (127,0,0,1,198,47).\r\n");

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
	int clen = sizeof(caddr);
	client.data_sk = accept(datasocket, (sockaddr*)&caddr, &clen);
	cout << "connect data succesfully" << endl;
}

void resList(ftpClient client) {

	sendResponse(client, "150 Here comes the directory listing.\r\n");

	char dir[1024] = "drw-r--r--    1 501      501        688017 Jan 24 13:04 Chuong0\r\n";
	
	sendDir(client, dir);
//	closesocket(datasocket);
	closesocket(client.data_sk);

	sendResponse(client, "226 Directory send OK.\r\n");
}

void resType(ftpClient client) {
	sendResponse(client, "200 Switching to Binary mode.\r\n");
}
void resFeat(ftpClient clen) {
	char start[1024] = "211-Features:\r\n";
	char f1[1024] = "EPRT\r\n"; char f2[1024] = "EPSV\r\n";
	char f3[1024] = "MDTM\r\n"; char f4[1024] = "REST STREAM\r\n";
	char f5[1024] = "SIZE\r\n"; char f6[1024] = "TVFS\r\n";
	char f7[1024] = "UTF8\r\n"; 
	char end[1024] = "211 End.\r\n";

	send(clen.ctrl_sk, start, sizeof(start), 0);
	send(clen.ctrl_sk, f1, sizeof(f1), 0);  send(clen.ctrl_sk, f2, sizeof(f2), 0);
	send(clen.ctrl_sk, f3, sizeof(f3), 0);  send(clen.ctrl_sk, f4, sizeof(f4), 0);
	send(clen.ctrl_sk, f5, sizeof(f5), 0);  send(clen.ctrl_sk, f6, sizeof(f6), 0);
	send(clen.ctrl_sk, f7, sizeof(f7), 0); 
	send(clen.ctrl_sk, end, sizeof(end), 0);
}


void Handle_client(ftpClient& clen) {
	if (strcmp(clen.verb, "USER") == 0) resUser(clen);
	else if (strcmp(clen.verb, "PASS") == 0) resPass(clen);
	else if (strcmp(clen.verb, "OPTS") == 0) resOPTS(clen);
	else if (strcmp(clen.verb, "PWD") == 0) resPwd(clen);
	else if (strcmp(clen.verb, "CWD") == 0) resCwd(clen);
	else if (strcmp(clen.verb, "SYST") == 0) resSYST(clen);
	else if (strcmp(clen.verb, "PASV") == 0) resPasv(clen);
	else if (strcmp(clen.verb, "LIST") == 0) resList(clen);
	else if (strcmp(clen.verb, "TYPE") == 0) resType(clen);
	else if (strcmp(clen.verb, "FEAT") == 0) resType(clen);
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
		memset(client.verb, 0, 1024);

		recv(client.ctrl_sk, buffer, 1023, 0);
		sscanf(buffer, "%s", client.verb);

		if (strcmp(verb, "QUIT") == 0) {

		}
		else {
			Handle_client(client);
		}
	}
}