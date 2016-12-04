# Komunikacja między procesami
Sposób na synchronizację i przekazywanie informacji między procesami np.
W potoku unixowym producent musi jakoś informować konsumenta o nowych danych

## Semafory
Jest to współdzielona zmienna zawierająca liczbę naturalną, na której zdefiniowane są trzy operacje
są one atomowe
- `init`
  inicjalizuje nowy semafor

- `up`\\`increment`\\`signal`
  zwiększa licznik o jeden, jeżeli jakiś proces czekał na tym semaforze, zostaje on obudzony

- `down`\\`decrement`\\`wait`
  zmniejsza licznik o jeden, lub jeżeli wynosi on zero, blokuje proces

### Rodzaje semaforów
- zliczające - mają wartości naturalne
- binarne - mają wartość 0 lub 1
- silne - definiują w jakiej kolejności uruchamiane będą procesy czekające na semafor

### Mutex vs semafor binarny
Mutex w przeciwieństwie do semafora binarnego musi zostać odblokowany przez ten sam proces,
który go zablokował (tzn jeżeli proces wykonuje `wait` to musi potem wykonać `signal`

## Potoki
Unixowy pomysł na kompozycję różnych programów i ich komunikację. Z założenia każdy program
ma dostępne standardowe wejście i wyjście, za pomocą potoków można przekierować wyjście
jednego programu do wejścia drugiego, jest to jednokierunkowy strumień danych z jednego
procesu do drugiego

## Gniazda
Pozwalają na dwukierunkową komunikację między procesami, lokalnie lub na różnych komputerach
- datagramowe - gwarantują wysyłanie wiadomości o konkretnej długości, jednakże nie gwarantują kolejności
  ich dostarczenia, ani pewności dostarczenia
- strumieniowe - gwarantują kolejność dostarczania bajtów, oraz pewność ich dostarczenia, wysyłają
  strumień bajtów, a nie pojedyńcze wiadomości



