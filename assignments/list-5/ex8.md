### RCU
Technika zachowania spójności współdzielonych danych oparta na następujących założeniach:
- wiele wątków czytających
- co najwyżej jeden wątek zapisującyh
- określamy sekcję krytyczną czytania, w której wątek nie może zostać zablokowany,
  lub zasnąć. Wtedy mamy pewność, że jeżeli żaden wątek się w takiej sekcji nie
  znajduje to możemy bezpiecznie zapisać zmiany
Wykorzystuje się w niej podział na dwie fazy:
- usuwanie elementów - w tej fazie możemy atomowo wyłączyć fragment naszej
  struktury danych, który dalej pozostanie w pamięci, więc wątki czytające
  mogą dalej z niego korzystać
- odzyskanie pamięci - gdy wszystkie wątki wyjdą z sekcji krytycznej można
  usunąć wszelkie dane które zostałyc wyłączone w fazie usuwania

Dobrym przykładem jest zastowanie RCU w drzewiastych strukturach danych,
patrz Tannenbaum str. 149
