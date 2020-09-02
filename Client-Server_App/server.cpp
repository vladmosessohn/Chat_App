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
int main(int argc, char *argv[]) {
  map<string, vector<string> > topicsByEachClient;
  //    id      topicele la care e abonat
  map<string, vector<int> > sfByEachTopic;
  //    id       toate sf-urile pt fiecare topic 0/1
  map<string, int> socketByEachClient;
  //  id    socket
  map<string, bool> onlineEachClient;
  //   id     true/false daca e online clientul
  map<string, vector<int> > lastMessPosByClient;
  //   id      pozitiile ultimelor mesaje trimise cu clientul online
  map<string, vector<string> > messToBeSent;
  //   id      mesajele pe care trebuie sa le trimitem
  int socktcp, sockudp, newsockfd, received, ret;
  char buffer[BUFLEN];
  bool flag_ex = false;
  struct sockaddr_in addrtcp, addrudp;
  socklen_t addr_len = sizeof(sockaddr);

  fd_set read_fds;  // multimea de citire folosita in select()
  fd_set tmp_fds;   // multime folosita temporar
  int fdmax;        // valoare maxima fd din multimea read_fds

  if (argc != 2) {
    cout << "Usage: ./server <PORT>.\n";
  }

  socktcp = socket(AF_INET, SOCK_STREAM, 0);
  if (socktcp < 0) {
    cout << "Creating tcp socket problem.\n";
    return 0;
  }

  sockudp = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockudp < 0) {
    cout << "Creating udp socket problem.\n";
  }

  if (atoi(argv[1]) == 0) {
    cout << "Please select another port number.\n";
  }

  addrtcp.sin_family = AF_INET;
  addrtcp.sin_port = htons(atoi(argv[1]));
  addrtcp.sin_addr.s_addr = INADDR_ANY;

  addrudp.sin_family = AF_INET;
  addrudp.sin_port = htons(atoi(argv[1]));
  addrudp.sin_addr.s_addr = INADDR_ANY;

  ret = bind(socktcp, (sockaddr *)&addrtcp, sizeof(sockaddr));
  if (ret < 0) {
    cout << "Binding tcp socket problem.Address already in use.\n";
  }
  ret = bind(sockudp, (sockaddr *)&addrudp, sizeof(sockaddr));
  if (ret < 0) {
    cout << "Binding udp socket problem.Address already in use.\n";
  }

  ret = listen(socktcp, MAX_CLIENTS);
  if (ret < 0) {
    cout << "Listening problem.\n";
  }

  FD_ZERO(&read_fds);
  FD_SET(sockudp, &read_fds);
  FD_SET(socktcp, &read_fds);
  FD_SET(0, &read_fds);
  fdmax = socktcp;

  int flag = 1;
  setsockopt(socktcp, SOL_SOCKET, TCP_NODELAY, &flag, sizeof(flag) == 0);

  while (flag_ex == false) {
    tmp_fds = read_fds;
    ret = select(fdmax + 1, &tmp_fds, nullptr, nullptr, nullptr);
    if (ret < 0) {
      cout << "Selecting problem.\n";
    }
    for (int i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &tmp_fds)) {
        if (i == socktcp) {
          newsockfd = accept(socktcp, (sockaddr *)&addrtcp, &addr_len);
          if (newsockfd < 0) {
            cout << "Accepting problem.\n";
          }
          // adaugam noul cocket la multimea de fd
          FD_SET(newsockfd, &read_fds);
          int fdmaxaux = fdmax;
          if (newsockfd > fdmax) {
            fdmax = newsockfd;
          }
          memset(buffer, 0, BUFLEN);
          received = recv(newsockfd, buffer, sizeof(buffer), 0);
          if (received > 0) {
            bool flag = true;
            for (auto k : socketByEachClient) {
              // vrem sa vedem daca ID-ul este deja luat si il cautam
              if (k.first.compare(buffer) == 0 &&
                  onlineEachClient[buffer] == true) {
                char message[50] =
                    "ID already taken. You will be disconnected.\n";
                flag = false;
                if (send(newsockfd, message, strlen(message), 0) < 0) {
                  cout << "Sending notification error to client problem.\n";
                }
                // trimitem un mesaj clientului ca ID-ul este deja folosit
                close(newsockfd);
                // inchidem conexiunea cu ID-ul duplicat
                if (newsockfd == fdmax) {
                  fdmax = fdmaxaux;
                }
                // reparam fdmax-ul daca este nevoie
              }
            }
            if (flag == true) {
              cout << "New client " << buffer << " connected from "
                   << inet_ntoa(addrtcp.sin_addr) << ":"
                   << ntohs(addrtcp.sin_port) << "." << endl;
              socketByEachClient[buffer] = newsockfd;
              onlineEachClient[buffer] = true;
              // daca ID-ul este valid il adaugam la map impreuna cu socketul
              // aferent
              int sz = topicsByEachClient[buffer].size();
              for (int x = 0; x < sz; x++) {
                if (sfByEachTopic[buffer][x] == 1) {
                  for (auto x : messToBeSent[buffer]) {
                    if (send(newsockfd, x.c_str(), strlen(x.c_str()), 0) < 0) {
                      // cand clientul se logheaza din nou ii trimitem automat
                      // toate mesajele pe care le-am stocat pentru el
                      cout << "Sending data to client problem.\n";
                    }
                  }
                  messToBeSent[buffer].clear();
                }
              }
            }
          } else {
            cout << "No data received from subscriber" << '\n';
          }
        } else if (i == sockudp) {
          ret = recvfrom(sockudp, buffer, BUFLEN, 0, (sockaddr *)&addrudp,
                         &addr_len);
          // primim mesajul de la clientul UDP
          if (ret < 0) {
            cout << "No message received from UDP client\n";
          }
          char topic[80];
          strcpy(topic, buffer);
          string currentTopic(buffer);
          // vom forma mesajul astfel incat sa fie gata facut si bun de trimis
          // clientilor
          char message[BUFLEN];
          strcpy(message, inet_ntoa(addrudp.sin_addr));
          strcat(message, ":");
          strcat(message, to_string(ntohs(addrudp.sin_port)).c_str());
          strcat(message, " - ");
          strcat(message, topic);
          strcat(message, " - ");
          // incepem sa formam mesajul in variabila auxiliara message
          uint8_t dataType = *(uint8_t *)(buffer + 50);
          // luam tipul de data
          if (dataType == 0) {
            strcat(message, "INT - ");
            if (*(uint8_t *)(buffer + 51) == 1) {
              strcat(message,
                     "-");  // daca bitul de semn este negativ punem minus
                            // direct in message fara sa ne mai complica
            }
            strcat(message,
                   to_string(ntohl(*(uint32_t *)(buffer + 52))).c_str());
            // dupa punem valoarea efectiva
          }
          if (dataType == 1) {
            strcat(message, "SHORT_REAL - ");
            char aux[1500];
            snprintf(aux, sizeof(aux), "%.2f",
                     ntohs(*(uint16_t *)(buffer + 51)) / 100.0);
            strcat(message, aux);  // concatenam stringul la message, dar intai
                                   // il convertim la char*
          }
          if (dataType == 2) {
            char aux[1500];
            strcat(message, "FLOAT - ");
            if (*(uint8_t *)(buffer + 51) == 1) {
              strcat(message,
                     "-");  // daca este numar negativ , punem minus in message
            }

            if (*(uint8_t *)(buffer + 56) >= 1) {
              float fl = 1;
              for (int x = 0; x < *(uint8_t *)(buffer + 56); x++) {
                fl = fl * 10.0;  // calculam 10^cat ne trebuie
              }
              snprintf(aux, sizeof(aux), "%.4f",
                       ntohl(*(uint32_t *)(buffer + 52)) / fl);
              // mutam in aux rezultatul convertit la char*
            } else {
              snprintf(aux, sizeof(aux), "%d",
                       ntohl(*(uint32_t *)(buffer + 52)));
              // mutam in aux rezultatul convertit la char*
            }
            strcat(message, aux);

            // concatenam la message aux-ul
          }
          if (dataType == 3) {
            strcat(message, buffer + 51);  // luam efectiv sirul de caractere si
                                           // il concatenam la message
          }

          cout << message << endl;
          strcpy(buffer, message);
          // mutam din nou tot mesajul construit in buffer
          for (auto x : topicsByEachClient) {
            vector<string>::iterator it;
            it = find(x.second.begin(), x.second.end(), topic);
            if (it != x.second.end()) {
              int pos = -1;
              // cautam al catelea topic este din lista de topice a fiecarui
              // client
              int sz = topicsByEachClient[x.first].size();
              for (int k = 0; k < sz; k++) {
                if (topicsByEachClient[x.first][k] == currentTopic) {
                  pos = k;
                }
              }
              if (onlineEachClient[x.first] == true) {
                strcat(buffer, "\n");
                // daca nu concatenez la fiecare mesaj cate un endline, cand un
                // client se reconecteaza si are de primit mai multe mesaje,
                // mesajele se vor primi toate concatenate de asta concatenam la
                // fiecare cate un endline direct
                if (send(socketByEachClient[x.first], buffer, strlen(buffer),
                         0) < 0) {
                  cout << "Sending data to client problem.\n";
                }
              } else {
                if (sfByEachTopic[x.first][pos] == 1) {
                  // nu mai facem verificare pentru pos >= 0 deoarece stim sigur
                  // ca respectivul client este abonat la la topic pentru ca
                  // l-am cautat cu find
                  strcat(buffer, "\n");
                  messToBeSent[x.first].push_back(buffer);
                  // adaugam in vectorul clientului curent mesajul la stocat
                  // daca cilentul nu este online si sf-ul pentru topicul curent
                  // este 1
                }
              }
            }
          }

        } else if (i == 0) {
          // daca vrem sa dam exit din server
          fgets(buffer, BUFLEN - 1, stdin);
          if (strcmp(buffer, "exit\n") == 0) {
            flag_ex = true;
            break;
          }
        } else {
          // primim comanda de la unul din clienti
          memset(buffer, 0, BUFLEN);
          received = recv(i, buffer, sizeof(buffer), 0);
          if (received < 0) {
            cout << "No information received from client.\n";
          }

          if (received == 0) {
            // conexiunea s-a inchis
            for (auto j : socketByEachClient) {
              string id_aux = j.first;
              if (j.second == i && onlineEachClient[id_aux] == true) {
                // gasim ID-ul socketului care da disconnect
                cout << "Client " << j.first << " disconnected.\n";
                onlineEachClient[id_aux] = false;
              }
            }
            close(i);
            FD_CLR(i, &read_fds);
          } else {
          	// parsam comanzile primite
            string id;
            char *aux;
            strcpy(buffer + strlen(buffer) - 1, " ");
            aux = strtok(buffer, " ");
            aux = strtok(nullptr, " ");

            for (auto j : socketByEachClient) {
              // cautam ID-ul socketului respectiv
              string id_aux = j.first;
              if (j.second == i && onlineEachClient[id_aux] == true) {
                id = id_aux;
              }
            }
            // am aflat id-ul clientului tcp

            if (buffer[0] == 's') {
              // am aflat id-ul
              vector<string>::iterator it;
              it = find(topicsByEachClient[id].begin(),
                        topicsByEachClient[id].end(), aux);
              // daca topicul este deja il lista de topice al clientului
              // respectiv nu il mai adaugam
              if (it == topicsByEachClient[id].end()) {
                topicsByEachClient[id].push_back(aux);
                lastMessPosByClient[id].push_back(0);
                // adaugam si topicul si sf-ul aferent in map-uri(sf-ul mai jos
                // in functie de cat e)
                aux = strtok(nullptr, " ");
                if (aux[0] == '0') {
                  sfByEachTopic[id].push_back(0);
                }
                if (aux[0] == '1') {
                  sfByEachTopic[id].push_back(1);
                }
                // l am abonat practic pe client la topicul respectiv
              }
            }

            if (buffer[0] == 'u') {
              int pos = -1;
              // aflam ce index are in lista topicul pe care vrem sa l stergem
              for (auto ik : topicsByEachClient) {
                int sz = ik.second.size();
                for (int j = 0; j < sz; j++) {
                  if (ik.second[j] == aux) {
                    pos = j;
                  }
                }
              }
              if (pos >= 0) {
                // stergem si topicul si sf-ul din map-urile utilizate
                topicsByEachClient[id].erase(topicsByEachClient[id].begin() +
                                             pos);
                sfByEachTopic[id].erase(sfByEachTopic[id].begin() + pos);
              }
            }
          }
        }
      }
    }
  }
  for (int i = 0; i <= fdmax; i++) {
    close(i);
  }
  // inchidem toate conexiunile
  return 0;
}