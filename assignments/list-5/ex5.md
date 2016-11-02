### Aktywne czekanie 
proces ciągle sprawdza czy semafor już się podniósł
### Uśpienie 
proces usypia się do momentu podniesienia semafora

Zauważmy, że w przypadku krótkiego oczekiwania na semafor lepsze może być czekanie
aktywne, jako, że nie wymaga przejścia do jądra

### Semafory adaptacyjne

Pozwalają na przeprowadzanie operacji na mutexie w przestrzeni użytkownika o ile nie
ma innych procesów które aktualnie czekają na wznowienie

    wake(addr)
      if(compare_and_swap(addr, 0, 1) == 0 )
        return
      else
        addr ++
        FUTEX_WAKE(addr, 1)

    wait(addr)
      val = compare_and_swap(addr, 1, 0)
      if(val == 1)
        return
      else
        addr = -1
        FUTEX_WAIT(addr, -1)


