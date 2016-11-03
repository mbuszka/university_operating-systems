**Sekcja krytyczna** - fragment kodu w którym program korzysta ze współdzielonych zasobów
(pamięć/IO/urządzenia)

### Założenia
- tylko jeden proces może znajdować się w sekcji krytycznej (związanej z konkretnym zasobem)
- nie można nic zakładać o prędkości i ilości procesorów
- proces może przebywać w sekcji krytycznej jedynie przez skończony czas
- proces który zatrzymałby się poza sekcją krytyczną nie może wpłynąć na inne procesy
- jeżeli żaden inny proces nie znajduje się w sekcji krytycznej, to można wejść do niej
  bez oczekiwania

### Rozwiązania

#### Wyłączenie przerwań
- działa jedynie przy jednym procesorze
- nie pozwala na wydajne wykorzystanie procesora
- najprostsze rozwiązanie

#### Instrukcje atomowe
- instrukcja która może wykonać dwa zadania niepodzielnie w ramach wykonania jednej instrukcji
- `XCHG` - zamienia wartość rejestru z wartością w pamięci
```
XCHG *register, *memory
  tmp = *memory
  *memory = *register
  *register = tmp
```

- `compare_and_swap`
```
compare_and_swap *memory, test, new
  old = *memory
  if old == test
    *memory = new
  return old
```

#### Pamięć transkacyjna
Rozwiązanie problemu współdzielonej pamięci za pomocą transakcji, czyli fragmentu kodu który
logicznie wykonuje się w tym samym czasie (atomowo). Transakcja powodzi się, jeżeli w trakcie
jej wykonywania pamięć której używała nie zmieniła się, w przeciwnym wypadku jest ona przerwana
i może zostać rozpoczęta od nowa

#### Blokada wirująca
Blokada w której proces aktywnie testuje, czy może wejść do sekcji krytycznej

    int bolt; \\ globalna zmienna, która mówi czy ktoś znajduje się w sekcji krytycznej

    void spinlock(bolt) {
      while (compare_and_swap(&bolt, 0, 1) == 1) \\ czekaj aż będzie można wejść do sekcji krytycznej
      \\ sekcja krytyczna
      bolt = 0;
    }


