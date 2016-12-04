**Rywalizacja procesów** - korzystanie z jednego zasobu (np. drukarka, dysk itp) przez jeden lub więcej procesów

    free := is_free(printer)
    if free
      set_using(printer)
      write_to_buffer(printer)
      print(printer)
      set_free(printer)
    end

| t |         A         |        B      |
|---|       -----       |      -----    |
| 1 | `free := ...`     | .             |
| 2 | .                 | `free := ...` |
| 3 | `if free`         | .             |
| 4 | `set_using...`    | .             |
| 5 | `write_to_buffer` | .             |
| 6 | .                 | `if free`     |
| 7 | .                 | `set_using`   |

**Sytuacja wyścigu** ang. *race condition* - sytuacja w której kilka procesów jednocześnie czytają i zapisują do
jednego miejsca w pamięci/pliku/itp
 - wynik takiego działania zależy od konkretnej kolejności wykonania programów
 - często możemy dostać inny wynik przy kolejnych/różnych wywołaniach

**Heisenbug** - problem który znika lub zmienia swe zachowanie gdy próbujemy go odizolować lub zbadać.
Nie zawsze musi być to znacząca zmiana, czasem wystarczy odpalenie programu w debuggerze


