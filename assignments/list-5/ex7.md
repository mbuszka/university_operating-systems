### Przekazywane komunikatów
Procesy komunikują się ze sobą wysyłając sobie wzajemnie wiadomości
minimalna implementacja wymaga dwóch funkcji:
- `send(dst, msg)`
- `recv(src, msg)`

komunikat może mieć dowolną strukturę, w zależności od potrzeb i stopnia
skomplikowania systemu. Przykładowo
    
    struct message {
      msg_id_t     id;
      msg_type_t   type;
      msg_box_t    dst;
      msg_box_t    src;
      char         data[N];
    }

#### Synchronizacja
Zarówno `send` jak i `recv` mogą być blokujące lub nie. Daje nam to kilka
możliwości rozwiązania problemu synchronizacji

1. `send` oraz `recv` blokują - w tym przypadku mamy do czynienia z tzw punktami
   schadzek (rendezvous) w których proces wysyłając wiadomość musi zaczekać aż inny
   proces "przyjdzie" i ją od niego odbierze

2. `send` nie blokuje, `recv` blokuje - naturalna kombinacja w której wytwórca wiadomości
   może generować jedną po drugiej nie przejmując się tempem ich odbierania, natomiast
   adresat musi zaczekać na wiadomość jeżeli chce ją odebrać

3. `send` oraz `recv` nie blokują - oba procesy mogą decydować co się dzieje gdy nie ma
   wiadomości, jednakże jest niebezpieczeństwo przegapienia wiadomości jeżeli najpierw
   jeden z nich próbował ją odebrać, a dopiero potem drugi ją wysłał

Porównując to z semaforami druga konfiguracja zachowuje się jak semafor zliczający,
natomiast trzecia działa podobnie do współdzielonej pamięci

#### Adresowanie
- **Skrzynki pocztowe** - W tym wariancie `send` dodaje wiadomość do kolejki w konkretnej 
  skrzynce pocztowej, natomiast `recv` pobiera wiadomość z konkretnej skrzynki
- **Bezpośrednie** - Tutaj proces musi znać identyfikator odbiorcy, a odbiorca musi znać
  identyfikator nadawcy (lub odebrać wiadomość od wszystkich)


