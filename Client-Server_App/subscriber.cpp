#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <map>
#include <vector>
using namespace std;
#define BUFLEN 1560
#define MAX_CLIENTS __INT_MAX__
bool un_subscribe(char buffer[]) {
  char copie[81];
  strcpy(copie, buffer);
  char *aux;
  aux = strtok(copie, " ");
  aux = strtok(nullptr, " ");

  string str(aux);
  if (buffer[0] == 's') {
    cout << "subscribed " << str << '\n';
    return true;
    // aici trebuie sa afisam si \n deoarece strtok-ul
    // nu retine si \n pentru ca dupa topic mai urmeaza si sf-ul
  }
  if (buffer[0] == 'u') {
    cout << "unscubscribed " << str;
    return true;
    // aici nu mai trebuie sa afisam si \n deoarece strtok-ul il ia
    // si pe el
  }
  return false;
}

int main(int argc, char *argv[]) {
  vector<string> allClientsID;
  int sockfd, bytesRecv, ret, i;
  sockaddr_in serv_addr;
  char buffer[BUFLEN];
  bool flag = false;

  if (argc != 4) {
    cout << "Usage: ./subscriber <ID> <IP_SERVER> <PORT_SERVER>. \n";
  }

  if (strlen(argv[1]) > 10) {
    cout << "ID too big.\n";
  }

  fd_set read_fds;  // multimea de citire folosita in select()
  fd_set tmp_fds;   // multime folosita temporar
  int fdmax;        // valoare maxima fd din multimea read_fds

  FD_ZERO(&read_fds);
  FD_ZERO(&tmp_fds);

  // Read stdin
  FD_SET(0, &read_fds);

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    cout << "Opening socket error.\n";
  }
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(atoi(argv[3]));
  ret = inet_aton(argv[2], &serv_addr.sin_addr);
  if (ret == 0) {
    cout << "Wrong IP.\n";
  }
  ret = connect(sockfd, (sockaddr *)&serv_addr, sizeof(serv_addr));
  if (ret < 0) {
    cout << "Connecting to server problem.\n";
  }
  if (send(sockfd, argv[1], strlen(argv[1]), 0) < 0) {
    cout << "Sending ID error.\n";
  }
  // trimitem ID-ul la server imediat dupe ce ne conectam ca serverul
  // sa stie cine tocmai s-a conectat
  FD_SET(sockfd, &read_fds);
  fdmax = sockfd;

  int aux = 1;
  setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &aux, sizeof(aux) == 0);

  while (flag == false) {
    tmp_fds = read_fds;

    ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
    if (ret < 0) {
      cout << "Selecting problem\n";
    }

    for (i = 0; i <= fdmax; i += fdmax) {
      if (FD_ISSET(i, &tmp_fds)) {
        if (i == 0) {
          memset(buffer, 0, BUFLEN);
          fgets(buffer, BUFLEN - 1, stdin);
          // citim comanda de la tastatura
          if (strcmp(buffer, "exit\n") == 0) {
            flag = true;
            break;
          }
          bool ok = true;
          char aux[80];
          strcpy(aux, buffer);
          char *tok;
          tok = strtok(aux, " ");
          // despartim copia bufferului in cuvinte sa nu stricam bufferul
          if (tok == nullptr) {
            ok = false;
            cout << "usage (un)subscribe <topic> (<sf>)" << '\n';
            break;
          }
          if (strcmp(tok, "subscribe") != 0 &&
              strcmp(tok, "unsubscribe") != 0) {
            // verificam ca primul cuvand din comanda sa fie subscribe sau
            // unsubscribe
            ok = false;
            cout << "usage (un)subscribe <topic> (<sf>)" << '\n';
            break;
          }
          tok = strtok(nullptr, " ");
          if (tok == nullptr || strlen(tok) > 51) {
            ok = false;
            cout << "usage (un)subscribe <topic> (<sf>)" << '\n';
            break;
          }
          if (buffer[0] == 's') {
            tok = strtok(nullptr, " ");
            if (tok == nullptr) {
              // verificam sa primim si sf-ul
              ok = false;
              cout << "usage subscribe <topic> <sf>" << '\n';
              break;
            }
            if (strcmp(tok, "0\n") != 0 && strcmp(tok, "1\n") != 0) {
              // verificam sa fie 0 sau 1 sf-ul
              ok = false;
              cout << "usage subscribe <topic> <sf>" << '\n';
              break;
            }
          }
          if (ok == true) {
            // comanda este valida, o trimitem catre server
            if (send(sockfd, buffer, strlen(buffer), 0) >= 0) {
              bool sout = un_subscribe(buffer);
              if (sout == false) {
                cout << "printing subscribed topic error" << '\n';
              }
            }
          }
        }
        if (i == sockfd) {
          memset(buffer, 0, BUFLEN);
          // primim mesajul din server
          bytesRecv = recv(sockfd, buffer, BUFLEN, 0);
          if (bytesRecv < 0) {
            cout << "Error receiving data from the server.\n";
          }
          if (bytesRecv == 0) {
            flag = true;
            break;
          }
          cout << buffer;
        }
      }
    }
  }
  close(sockfd);
  return 0;
}
