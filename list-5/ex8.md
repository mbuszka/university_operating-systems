### RCU
Technika zachowania spójności współdzielonych danych oparta na następujących założeniach:  
1. Wiele wątków czytających  
2. Co najwyżej jeden wątek zapisujący  
3. Określamy sekcję krytyczną czytania, w której wątek nie może zostać zablokowany ani zasnąć. Wtedy mamy pewność, że jeżeli żaden wątek się w takiej sekcji nie znajduje, możemy bezpiecznie zapisać zmiany.  

Wykorzystuje się w niej podział na dwie fazy:  
1. Usuwanie elementów - w tej fazie możemy atomicznie wyłączyć fragment naszej
  struktury danych, który dalej pozostanie w pamięci, więc wątki czytające
  mogą dalej z niego korzystać
2. Odzyskanie pamięci - gdy wszystkie wątki wyjdą z sekcji krytycznej można
  usunąć wszelkie dane, które zostały wyłączone w fazie usuwania

Dobrym przykładem jest zastowanie RCU w drzewiastych strukturach danych - patrz Tannenbaum str. 149.
