#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#include <Windows.h>
#include <string.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#define server_addr "127.0.0.1"
#define welcome "220 welcome to ftp server on windown.\r\n"
using namespace std;

DWORD WINAPI ClientThread(LPVOID arg);

struct ftpClient {
	char verb[1024], curdic[1024], arg[1024], name[1024], pass[1024];
	SOCKET data_sk, ctrl_sk;
	int login;

	ftpClient(SOCKET c) {
		ctrl_sk = c;
		login =  -1;
		memset(curdic, 0, 1024);
		strcpy(curdic, "/");
	}
};

void insert(char* source, int index) {
	char* tmp = new char[strlen(source) + 1];
	strcpy(tmp, source + index);
	source[index] = '"';
	strcpy(source + index + 1, tmp);
	free(tmp);
}

void sendResponse(ftpClient client, char* res) {
	send(client.ctrl_sk, res, strlen(res), 0);
}


void resUSER(ftpClient& client) {
	memset(client.name, 0, 1024);
	strcpy(client.name, client.arg);
	sendResponse(client, "331 Please spcify the password.\r\n");
}

void resPASS(ftpClient& client) {
	memset(client.pass, 0, 1024);
	strcpy(client.pass, client.arg);

	if (strcmp(client.name, "anonymous") == 0) {
		// dang nhap an danh
		client.login = 1;
		sendResponse(client, "230 Login succesfull.\r\n");
	}
	else {
		FILE* f = fopen("C:/Temp/login.txt", "rt");
		char line[1024], user[1024], pass[1024];
		bool isValid = false;
		while (!feof(f)) {
			memset(line, 0, 1024); memset(user, 0, 1024); memset(pass, 0, 1024);
			fgets(line, 1023, f);
			sscanf(line, "%s%s", user, pass);
			if (strcmp(user, client.name) == 0 && strcmp(pass, client.pass) == 0) {
				// dang nhap voi tu cach la user
				client.login = 2;
				isValid = true;
				sendResponse(client, "230 Login succesfull.\r\n");
				break;
			}
		}
		if (!isValid) {
			sendResponse(client, "530 name or password incorrect.\r\n");
			client.login = 0;
			closesocket(client.ctrl_sk);
		}
	}

}

void resOPTS(ftpClient client) {
	sendResponse(client, "200 Always in UTF8 mode.\r\n");
}

void resPWD(ftpClient client) {
	char dir[1024];
	memset(dir, 0, 1024);
	sprintf(dir, "%d \"%s\"\r\n", 257, client.curdic);
	sendResponse(client, dir);
}

void resCWD(ftpClient& client) {
	if (strstr(client.arg, "/") == NULL) {
		char path[1024];
		memset(path, 0, 1024);
		sprintf(path, "%s/%s", client.curdic, client.arg);
		strcpy(client.curdic, path);
	}
	else {
		memset(client.curdic, 0, sizeof(client.curdic));
		strcpy(client.curdic, client.arg);
	}
	sendResponse(client, "250 Directory succesfully changed.\r\n");
}

void resSYST(ftpClient client) {
	sendResponse(client, "215 Windown_NT.\r\n");
}

void resPASV(ftpClient& client) {
	srand(time(NULL));
	int port = 1024 + rand() % (65536 - 1024 + 1);

	char response[1024];
	memset(response, 0, 1024);
	sprintf(response, "227 Entering Passive Mode (127,0,0,1,%d,%d).\r\n", port / 256, port % 256);
	sendResponse(client, response);

	SOCKET datasocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKADDR_IN saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.s_addr = inet_addr(server_addr);
	bind(datasocket, (sockaddr*)&saddr, sizeof(saddr));
	listen(datasocket, 10);
	SOCKADDR_IN caddr;
	int clen = sizeof(caddr);
	client.data_sk = accept(datasocket, (sockaddr*)&caddr, &clen);
	cout << "connect data succesfully" << endl;
}

void resLIST(ftpClient client) {

	sendResponse(client, "150 Here comes the directory listing.\r\n");
	char command[1024], path[1024];
	memset(path, 0, 1024);
	strcpy(path, client.curdic);
	// them dau " vao
	int i = 0;
	while (i < strlen(path)) {
		if (path[i] == '/') {
			insert(path, i);
			insert(path, i + 2);
			i = i + 3;
		}
		else i++;
	}
	if (path[strlen(path) - 2] == '/') {
		path[strlen(path) - 1] = 0;
	}
	else if (path[strlen(path) - 1] != '"') {
		strcpy(path + strlen(path), "\"");
	}

	memset(command, 0, 1024);
	sprintf(command, "ls -l \"D:%s > C:/Temp/tmp.txt",path);
	system(command);

	FILE* f = fopen("C:/Temp/tmp.txt", "rt");
	fseek(f, 0, SEEK_END);
	long filesize = ftell(f);
	char* data = (char*)calloc(filesize, 1);
	fseek(f, 0, SEEK_SET);
	fread(data, 1, filesize, f);
	fclose(f);
	
	send(client.data_sk, data, filesize, 0);
	closesocket(client.data_sk);
	sendResponse(client, "226 Directory send OK.\r\n");
}

void resSTOR(ftpClient client) {
	if (client.login == 1) {
		sendResponse(client, "550 Permission denied.\r\n");
		closesocket(client.data_sk);
	}
	else {
		FILE* fw;
		char buf[1024], path[1024];
		memset(path, 0, 1024);
		sprintf(path, "D:%s/%s", client.curdic, client.arg);
		fw = fopen(path, "wb");
		sendResponse(client, "150 Opening BINARY mode data connection.\r\n");
		memset(buf, 0, 1024);
		long filesize = 0;
		while (0 == 0) {
			int n = recv(client.data_sk, buf, 1023, 0);
			filesize += n;
			if (n <= 0) break;
			fwrite(buf, 1023, 1, fw);
			memset(buf, 0, 1024);
		}
		cout << "kich thuoc file nhan duoc la " << filesize << endl;
		closesocket(client.data_sk);
		fclose(fw);
		sendResponse(client, "226 Transfer file successfully.\r\n");
	}
}

void resTYPE(ftpClient client) {
	sendResponse(client, "200 Switching to Binary mode.\r\n");
}

void resFEAT(ftpClient client) {
	sendResponse(client, "211-Features:\r\n");
	sendResponse(client, "OPTS\r\n");
	sendResponse(client, "SIZE\r\n");
	sendResponse(client, "STOR\r\n");
	sendResponse(client, "RETR\r\n");
	sendResponse(client, "TYPE\r\n");
	sendResponse(client, "211 End.\r\n");
}

void resSIZE(ftpClient client) {
	char path[1024], size[1024];
	memset(path, 0, 1024);
	sprintf(path, "D:%s/%s", client.curdic, client.arg);
	FILE* f = fopen(path, "rb");
	if (f != NULL) {
		fseek(f, 0, SEEK_END);
		long filesize = ftell(f);
		char* data = (char*)calloc(filesize, 1);
		fseek(f, 0, SEEK_SET);
		fread(data, 1, filesize, f);
		fclose(f);

		memset(size, 0, 1024);
		sprintf(size, "213 %d\r\n", filesize);
		sendResponse(client, size);
	}
	else sendResponse(client, "550 Could not get file size.\r\n");
}

void resRETR(ftpClient client) {
	char path[1024], status[1024], sendok[1024];
	memset(path, 0, 1024); memset(status, 0, 1024); memset(sendok, 0, 1024);
	sprintf(path, "D:%s/%s", client.curdic, client.arg);

	FILE* f = fopen(path, "rb");
	if (f != NULL) {
		fseek(f, 0, SEEK_END);
		long filesize = ftell(f);
		char* data = (char*)calloc(filesize, 1);
		fseek(f, 0, SEEK_SET);
		fread(data, 1, filesize, f);
		fclose(f);
		
		sprintf(status, "150 Opening BINARY mode data connection for %s (%ld bytes).\r\n", client.arg, filesize);
		sendResponse(client, status);
		send(client.data_sk, data, filesize, 0);
		closesocket(client.data_sk);
		sendResponse(client, "226 Transfer complete.\r\n");
	}
	else {
		sendResponse(client, "550 Failed to open file.\r\n");
	}
}

void Handle_client(ftpClient& client) {
	if (strcmp(client.verb, "USER") == 0) resUSER(client);
	else 
	if (strcmp(client.verb, "PASS") == 0) resPASS(client);
	else 
	if(client.login > 0) {
		if (strcmp(client.verb, "OPTS") == 0) resOPTS(client);
		else if (strcmp(client.verb, "PWD") == 0) resPWD(client);
		else if (strcmp(client.verb, "CWD") == 0) resCWD(client);
		else if (strcmp(client.verb, "SYST") == 0) resSYST(client);
		else if (strcmp(client.verb, "PASV") == 0) resPASV(client);
		else if (strcmp(client.verb, "LIST") == 0) resLIST(client);
		else if (strcmp(client.verb, "TYPE") == 0) resTYPE(client);
		else if (strcmp(client.verb, "FEAT") == 0) resTYPE(client);
		else if (strcmp(client.verb, "SIZE") == 0) resSIZE(client);
		else if (strcmp(client.verb, "RETR") == 0) resRETR(client);
		else if (strcmp(client.verb, "STOR") == 0) resSTOR(client);
	}
	else sendResponse(client, "500 Unknown command.\r\n");
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

		if (client.login == 0) break;

		memset(buffer, 0, 1024);
		memset(client.verb, 0, 1024);
		memset(client.arg, 0, 1024);
		recv(client.ctrl_sk, buffer, 1023, 0);
		if (strlen(buffer) == 0) break;

		char* firstspace = strstr(buffer, " ");
		if (firstspace != NULL) {
			strncpy(client.verb, buffer, firstspace - buffer);
			strcpy(client.arg, firstspace + 1);
			client.arg[strlen(client.arg) - 1] = 0;
			client.arg[strlen(client.arg) - 1] = 0;
			cout << client.verb << " " << client.arg << endl;
		}
		else {
			strcpy(client.verb, buffer);
			client.verb[strlen(client.verb) - 1] = 0;
			client.verb[strlen(client.verb) - 1] = 0;
			cout << client.verb << endl;
		}

		if (strcmp(verb, "QUIT") == 0){
			sendResponse(client, "221 Goodbye.\r\n");
			break;
		}
		else {
			Handle_client(client);
		}
	}
	cout << "Client disconnect" << endl;
	return 0;
}