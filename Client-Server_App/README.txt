Mosessohn Vlad - 322CA - Tema2 - PC

Starea aplicatiei:
Aplicatia client-server implementata functioneaza optim atat din punctul de
vedere al memoriei(am verificat cu valgrind sa nu am erori si memory leak-uri),
cat si din punctul de vedere al corectitudinii functionarii. Am testat
aplicatia astfel: am conectat mai multi clienti la server si i-am abonat la
diferite topicuri pe ficare. Cand clientul este abonat cu sf = 1 la un topic si
se deconecteaza(introduce comanda exit), acesta va primi ulterior(la
reconectare) toate mesajele trimise in legatura cu acel topic in lipsa sa
odata cu reconectarea. Cand clientul este abonat cu sf = 0 la un topic,
acesta primeste mesajele de la topicul respectiv doar cand este online.
Singura eroare a aplicatiei pe care am observat-o este cand un utilizator se
conecteaza la server , iar ulterior se conecteaza la mai multe topicuri,
printre care si <ana_announce_string>. Daca se deconecteaza, iar intre timp
se mai trimit mai mult de 2 mesaje pe fiecare topic, si se reconecteaza iar,
mesajele vor aparea toate ok, dar dupa mesajul cu topicul <ana_announce_string>
va aparea un endline uneori doar. Nu inteleg de ce se intampla acest lucru, dar
in afara de acest aspect, aplicatia ruleaza ok.

Utilizarea aplicatiei(Comenzi pentru rulare):
1. make
2. ./server <port> (ex: ./server 8080)
3. (in alt terminal) ./subscriber <id> <ip> <port>
	(ex: ./subscriber vlad 127.0.0.1 8080)
4. mai conectam cati clienti dorim ca mai sus doar ca in terminale diferite
5. abonam fiecare client la ce topic dorim (subscribe <topic> <sf>)
	(ex: subscribe a_non_negative_int 1)
6.in alt terminal rulam comanda pusa deja la dispozitie pentru a trimite mesaje
	(ex: ./python3 udp_cilent.py 127.0.0.1 8080)
7. fiecare client va primi mesajele pentru topicele la care s-a abonat
8. ulterior poate fi incercata si comanda exit din fiecare client
	(aici poate fi testata si funcionalitatea de store&forwarding a prog.)
9. pentru a se dezabona de la un topic clientul foloseste comanda:
	unsubscribe <topic> (ex: unsubscribe a_non_negative_int) 

Explicarea codului:
1.subscriber.cpp
Prima parte a codului reprezina initierea protocolului cu socketi pentru un
client tcp, care este insotita de o verificare a datelor primite de la
tastatura in argv(ip, port) si a initierii conexiunii. Cand clientul incearca
sa se conecteze la server, pentru a evita conectarea a 2 clienti simultan cu
acelasi id, trimit ID-ul clientului preluat din argv la server imediat dupa ce
se conecteaza pentru ca serverul sa verifice daca este deja cineva online cu
ID-ul respectiv. Daca ID-ul nu este valid(este deja cineva conectat), clientul
care a incercat sa se conecteze va primi cu functia recv() din while mesajul
"ID already taken. You will be disconnected.". Apoi setam o variabila auxiliara
flag cu false pentru a o folosi sa stim cand trebuie sa iesim din while(1).
In while dam select(), pentru care avem 2 cazuri. In cazul in care select este
pe 0 clientul introduce o comanda de la tastatura(exit, subscribe, unsubscribe)
Daca este exit setam flag cu true pentru a inchide conexiunea cu serverul. In
celelalte 2 cazuri parsam o copie a comanezii pentru a o trimite la server doar
daca este valida. Daca am trims-o cu succes, afisam pe ecran feedback-ul
corespunzator(subscribed <topic> sau unsubscribed <topic>). In cazul in care 
select este setat pe sockfd inseamna ca a primit un measaj de la server.
Mesajul vine deja construit corect indiferent de situatuie, asa ca il afisam.
Il afisam fara endline deoarece vine deja cu endline.
2.server.cpp
Am inceput implementarea serverului pornind de la combinarea laboratoarelor cu
clienti tcp si clienti udp. Am folosit cateva map-uri pentru a retine datele
despre clienti(explicate bine in cod). Similar cu subscriber.cpp, ne folosim
de o variabilia auxiliara ex_flag pentru a inchide conexiunea cand este 
introdusa comanda exit. In while folosim select(). Daca suntem pe cazul de tcp
inseamna ca am primit o cerere de conectare a unui client tcp si ii dam accept.
Dupa ce actualizam fdmax, trecem prin map-ul cu id-urile clientilor conectati
deja si verificam daca ID-ul pe care l-am primit cu recv este deja in map.
Daca l-am gasit, ii trimitem clientului inapoi mesajul "ID already taken. 
You will be disconnected.".
Aici trebuie sa restauram si fdmaxul. Daca ID-ul este ok afisam in server linia
de feedback indicata in enuntul temei. Tot la conectare parcurgem toate
topicele la care era eventual abonat clientul si ii trimitem toate mesajele
trimise cand respectivul client era deconectat pentru toate abonarile sale
stocate deja intr-un map, daca exista. Daca suntem in select pe cazul udp,
primim intreg mesajul in buffer, pentru ca apoi sa incepem sa il parsam.
retinem direct topicul in topic[] si tipul de date in dataType. Apoi in
functie de dataType construim mesajul pe care trebuie sa il trimitem la toti
clientii abonati, iar la final il mutam inapoi in buffer pentru a-l trimite.
Pentru trimiterea efectiva, parcurgem clientii si vedem ce client este abonat
sau nu la respectivul topic. Daca un client este aboant dar nu este online,
inseram mesajul in messToBeSent pentru a i-l trimite la o eventuala
reconectare. Acest map functioneaza ca o stiva de mesaje ce asteapta sa fie 
trimise la un anumit client(fiind mapat fiecare client cu setul sau de mesaje
in asteptare sa fie trimise).
Daca suntem pe cazul 0 cu select-ul inseamna ca serverul a primit o comanda de
la tastatura. Daca este exit, setam flag_ex si inchidem toate conexiunile la
final. Mai exista un singur caz de select, si anume cand serverul primeste o
comanda de la unul din clientii tcp. In functie de ce comanda a primit, adaugam
sau stergem din map-uri topicul respectiv, dar dupa ce facem parsarea comenzii.
Daca topicul primit este deja prezent in vectorul de topice din map-ul clientului
nu il mai introducem.
Daca topicul nu este prezent in vectorul de topice din map al clientului, cand 
vrem sa dam unsubscribe nu este nicio problema deoarece cautam topicul in acest
vector.

Probleme intampinate in implementare:
1. O problema majora de care m-am lovit este faptul ca std::cout-ul nu functiona
efectiv cum trebuie daca nu avea si un endline la final. Daca nu avea endline,
cout-ul se realiza cu intarziere de un mesaj. Mi-a luat destul de mult timp
sa ma prind de unde vine intarzierea.
2. Aceasta tema mi s-a parut a fi mai mult de gestionarea si stocarea datelor,
intrucat am acordat cel mai mult timp gandirii la o varianta cat mai eficienta
a retinarii datelor despre fiecare client.
