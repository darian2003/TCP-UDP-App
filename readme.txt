TEMA 2 PROTOCOALE DE COMUNICATII - BALAGIU DARIAN - 324CB

helper.h :

In acest fisier am definit structurile de client TCP, prtocoalele de UDP si TCP precum si mesajul de subscriptie.
Am folosit si macro-ul DIE pe care l-am copiat din laborator.

Structura clientului TCP include identificatorul sau unic - id; file descriptorul socketului sau si statusul activitatii sale.
De asemenea, fiecare client TCP va avea un vector propriu de topics la care este abonat. Acest vector va fi modificat in urma procesarii 
comenzilor de "subscribe" si "unsubscribe". Pentru a respecta conditia de Store&Forwrd, am ales ca mesajele topicilor pentru care campul SF 
este setat la 1 sa se salveze sub forma unei cozi, care este la baza creata drept o lista simpla inlantuita.

Campul data_type din struct udp_message reprezinta tipul de date care pot fi transmise prin acest protocol si este codificat folosing tabelul din PDF.
Structura de mesaj TCP doar encapsuleaza mesajul UDP de transmis, precum si datele clientului UDP 
care il transmite (de aici avem nevoie pentru afisare de IP-ul si portul clientului UDP).

Structura mesajului de subscriptie retine comanda dorita de catre client(subscribe/unsubscribe), topicul la care vrea sa se aboneze si flagul de Store&Forward.

queue.h queue.c list.h list.c :
Precum am precizat mai sus, mesajele catre utilizatori sunt retinute intr-o coada implementata printr-o lista simpla inlantuita. 
Implementarea este standard, urmareste doar functiile minimale de manipulare a elementelor din lista si este inspirata de pe StackOverflow.com.

server.c :

Serverul functioneaza astfel:
- Deschid socketul pentru conectiunea cu clientul UDP si cel pasiv de "listen" pentru clientii TCP.
- Pentru multiplexarea asincrona de I/O a serverului folosesc functia poll()
- Setez pollii standard de TCP, UDP si STDIN.
- In cadrul unui while, monitorizez primirea mesajelor pe toti pollii pe care ii am inclusi in vectorul pfds.
- Apelul functiei poll() este realizat parametrul timeout negatuv, intrucat ne dorim ca functia 
sa returneze doar atunci cand primim un mesaj pe orice socket.
- Pe socketul de STDIN urmarim doar primirea comenzii de exit, care rezulta inn ichiderea tuturor socketilor 
monitorizati din cadrul vectorului pfds.
- Pe socketl UDP doar primi mesaje de la clientii UDP pe care le redirectionam tuturor clientilor TCP abonati
la acel topic si activi, si eventual le adaugam in coada de mesaje daca SF este setat la 1.
- Pe socketul TCP de listen primim cereri de conectare la server din partea clientilor TCP. 
Aici, cream un nou socket preventiv si individual noului client TCP pe care vom primi ID-ul sau. 
In urma unor verificari ce privesc daca un client cu acel ID deja exista/este inactiv, vom pastra sau vom inchide acest socket creat preventiv. 
- Am creat functia create_client care creeaza adauga socketul nou in pfds in cazul in care ID-ul primit 
de la client este unul nou si functia reconnect_client care reseteaza pe ACTIVE campul "status" al 
clientului care era deconectat  si ii rettrimite tpate mesajele stocate in coada cat timp acesta a fost deconectat, iar apoi goloeste intreaga coada.
- In final, in cazul in care se primesc date pe socketii individuali ai clientilor TCP, 
procesam comenzile de tip "subscribe" si "unsubscribe", in urma carora printam la STDOUT 
textele aferente si modificam vectorii de topics ai clientilor care trimit aceste cereri.


subscriber.c :

Subscriberul functioneaza astfel:
- Subscriberul are implementat o functie care parseaza structura de mesaj udp primit si care
respecta protocoalele si codificarile specificate in cerinta in functie de cele 4 tipuri de date transmise.
- Completam in structura server_address informatiile aferente serverului, apoi ne conectam 
la acesta prin connect() la socketul serverului pe care se face listen(). connect() este 
o functie blocanta care asteapta sa primeasca acept() din partea serverului, dar intrucat 
numarul de conectiuni simultane la server va fi mic, fiecare apel de connect() trimis de catre subscriber va returna imediat. 
- In urma realizarii conectiunii cu serverul, subscriberul trimite serverului id-ul sau,
urmand ca acesta sa gestioneze ceea ce trebuie sa faca cu el (un client cu acest id este deja conectat/acest id este nou/clientul cu acest id este inactiv).
De aceasta data, id-ul trimis de catre client va fi receptionat de catre server pe noul socket creat in urma apelului accept().
- In urma procesarii ID-ului, subscriberul apeleaza functia blocanta recv(), asteptand confirmarea serverului.
Daca clientul cu ID-ul respcetiv se afla deja conectat, atunci serverul trimie subscriberului un mesaj
cu textul "close" care indica faptul ca sbscriberul trebuie sa inchida socketul de comunicare si sa retunreze (sa inchida aplicatia)
- La fel ca si la server, pentru multiplexarea asincrona de I/O, subscriberul foloseste 
functia poll(), insa doar pe doua porturi: socketul legat la server si STDIN.
- Subscriberul poate primi de la server mesaje de tip UDP pe care le va parsa folosind
funtia descrisa mai sus "printTCP" sau poate primi semnalul de oprire al serverului 
care va rezulta in inchiderea propriului socket si in oprirea programului.
- De la STDIN, subscriberul poate primi comenzi de subscribe si unsubscribe pe care le va redirectiona catre server.
