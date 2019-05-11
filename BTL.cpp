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
	int login, permission;

	ftpClient(SOCKET c) {
		ctrl_sk = c;
		login = permission = -1;
		memset(curdic, 0, 1024);
		strcpy(curdic, "/");
	}
};

SOCKET datasocket, sendata;

void sendResponse(ftpClient client, char* res) {
	send(client.ctrl_sk, res, strlen(res), 0);
}

void sendDir(ftpClient client, char* dir) {
	send(client.data_sk, dir, strlen(dir), 0);
}
void resUser(ftpClient& client) {
	memset(client.name, 0, 1024);
	strcpy(client.name, client.arg);
	sendResponse(client, "331 Please spcify the password.\r\n");
}

void resPass(ftpClient& client) {

	memset(client.pass, 0, 1024);
	strcpy(client.pass, client.arg);

	if (strcmp(client.name, "anonymous") == 0) {
		// dang nhap an danh
		client.login = 1;
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
				break;
			}

		}
		if (!isValid) {
			sendResponse(client, "530 name or password incorrect.\r\n");
			client.login = 0;
			closesocket(client.ctrl_sk);
		}
	}
	sendResponse(client, "230 Login succesfull.\r\n");

}

void resOPTS(ftpClient client) {
	sendResponse(client, "200 Always in UTF8 mode.\r\n");
}

void resPwd(ftpClient client) {
	char dir[1024];
	memset(dir, 0, 1024);
	sprintf(dir, "%d \"%s\"\r\n", 257, client.curdic);
	sendResponse(client, dir);
}

void resCwd(ftpClient& client) {
	
	memset(client.curdic, 0, sizeof(client.curdic));
	strcpy(client.curdic, client.arg);
	sendResponse(client, "250 Directory succesfully changed.\r\n");
}

void resSYST(ftpClient client) {
	sendResponse(client, "215 Windown_NT.\r\n");
}

void resPasv(ftpClient& client) {
	srand(time(NULL));
	int port = 1024 + rand() % (65536 - 1024 + 1);

	char response[1024];
	memset(response, 0, 1024);
	sprintf(response, "227 Entering Passive Mode (127,0,0,1,%d,%d).\r\n", port / 256, port % 256);
	sendResponse(client, response);

	datasocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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

void resList(ftpClient client) {

	sendResponse(client, "150 Here comes the directory listing.\r\n");
	char dir[4096], tmp[1024], command[1024];
	
	memset(command, 0, 1024);
	sprintf(command, "ls -l D:%s > C:/Temp/tmp.txt",client.curdic);
	system(command);

	memset(dir, 0, 4096);
	FILE* fr = fopen("C:/Temp/tmp.txt", "rt");
	fgets(tmp, 1024, fr);

	while (!feof(fr)) {
		memset(tmp, 0, 1024);
		fgets(tmp, 1023, fr);
		strcpy(tmp + strlen(tmp), " ");
		strcpy(dir + strlen(dir), tmp);
	}
	
	sendDir(client, dir);
	closesocket(client.data_sk);

	sendResponse(client, "226 Directory send OK.\r\n");
}

void resSTOR(ftpClient client) {
	FILE* fw;
	size_t nread;
	char buf[1024], path[1024];
	memset(path, 0, 1024);
	sprintf(path, "D:%s/%s", client.curdic, client.arg);
	fw = fopen(path, "wb");
	sendResponse(client, "150 Opening BINARY mode data connection.\r\n");
	memset(buf, 0, 1024);
	long filesize = 0;
	while (recv(client.data_sk, buf, 1023, 0)) {
		fwrite(buf, 1023, 1, fw);
		memset(buf, 0, 1024);
	}
	cout << "kich thuoc file la" << filesize << endl;
//	recv(client.data_sk, buf, 688017, 1);
//	fwrite(buf, sizeof(char), strlen(buf), fw);
	closesocket(client.data_sk);
	fclose(fw);
	sendResponse(client, "226 Transfer file successfully.\r\n");
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

void resSize(ftpClient client) {

	sendResponse(client, "550 Could not get file size.\r\n");
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
		// file khong ton tai
	}

}
void Handle_client(ftpClient& client) {
	if (strcmp(client.verb, "USER") == 0) resUser(client);
	else 
	if (strcmp(client.verb, "PASS") == 0) resPass(client);
	else 
	if(client.login > 0) {
		// dang nhap voi tu cach anonymous
		if (strcmp(client.verb, "OPTS") == 0) resOPTS(client);
		else if (strcmp(client.verb, "PWD") == 0) resPwd(client);
		else if (strcmp(client.verb, "CWD") == 0) resCwd(client);
		else if (strcmp(client.verb, "SYST") == 0) resSYST(client);
		else if (strcmp(client.verb, "PASV") == 0) resPasv(client);
		else if (strcmp(client.verb, "LIST") == 0) resList(client);
		else if (strcmp(client.verb, "TYPE") == 0) resType(client);
		else if (strcmp(client.verb, "FEAT") == 0) resType(client);
		else if (strcmp(client.verb, "SIZE") == 0) resSize(client);
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

		if (strcmp(verb, "QUIT") == 0 && strcmp(verb, "") == 0){

			// to do something
			break;
		}
		else {
			Handle_client(client);
		}
	}

	return 0;
}