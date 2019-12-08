  

		send.c:
	-am implementat functiile:
		checkSum -- face xor intre toti octetii din payload
			 -- o vom folosi pentru a verifica in recv daca mesajul care a fost primit in recv este corect
		initMyMsg-- initializeaza cu mesaj de tipul mymsg (declarat in mylib.h) 
			 -- voi pastra aici nrPachetului, tipul sau (poate fi:
				H-contine numele fisierului si bdp-ul;
				D-contine date din fisier; 
				A-ACK;
				N-NACK;)
		copyToPayload -- copiaza in campul payload al mesajului de tip msg datele salvate intr-un mymsg
			      -- calzuleaza checkSum

	-in main:
		La inceput:
			-calculez timeout	
			-calculez bdp ul dupa formula
			-deschid fisierul si ii calculez size-ul
			-tin minte in variabilele de_trimis nr pachetului pe care il trimit
			-calculez numarul de mesaje
			-imi creez un vector de mesaje de lungime bdp (buffer_sender)

		-voi trimite un prim pachet de tipul H (contine numele fisierului si valoarea bdp-ului)
		-completez fereastra de la 1 pana la bdp cu ce citesc din fisier (daca am ce citi), apelez initMyMsg si copyToPayload si trimit mesajul
		-apoi intr-un for de la 0 la nr_mesaje-bdp 
			1.astept confirmare pana o primesc, verific tipul (daca e NACK trimit iar)
			2.citesc in continuare din fisier, folosesc cele 2 functii pentru a crea mesajul de trimis si il trimit
		-in ultimul for astept ultimele confirmari, daca nu se primeste se retrimite mesajul urmeaza sa se verifice daca este A sau N
		-in final trimit un pachet de tip T care este cel de terminare 


		recv.c
	-gasim functiile din send.c si copyToMymsg
	
	-mai intai cream cele 2 tipuri de confirmari (ACK->A si NACK->N) si le pastram intr un vector confirmari;
	-primeste un prim pachet ce contine bdp; calculam checkSum si verificam daca este egala cu sum-ul calculat in send.c
	-in caz contrar trimit u nack
	-imi creez un vector de bdp elemente buffer_recv unde urmeaza sa stochez pachetele, iar in vectorul a_ajuns tin minte daca a fost primit
pachetul corespunzator
	-in while primesc cate un mesaj, apelez verifSum pentru a vedea daca mesajul este corect, in caz contrar trimit NACK
	-daca este corect, ma asigur ca nu este mesajul de terminare, caz in care ies din while
	-pastrez in nr numarul pachetului, verific daca se afla in interiorul ferestrei
	-daca nu este cel asteptat si nu am mai trimis NACK trimit din nou
	-daca nu a mai fost primit acest mesaj il pastram in buffer_recv
	-daca este cel asteptat verific daca este pachetul H(numele fisierului), caz in care creez fisierul de iesire
	-daca este de tip D scriu in fisier si trimit ACK
