**Zakleszczenie** (ang. *deadlock*) - sytucaja w której kilka procesów czeka na siebie nawzajem
by kontynuować działanie
- wyłączność - zasób jest dostępny tylko dla jednego procesu
- posiadanie - proces może trzymać jeden zasób czekając na inny
- brak wywłaszczania - zasób może być oddany tylko dobrowolnie
- zapętlone oczekiwanie - istnieje taka kombinacja procesów w której każdy z nich 
  posiada zasób na który kolejny oczekuje

**Niejawne zakleszczenie** (ang. *livelock*) - sytuacja w której kilka procesów naprzemiennie zmienia
stan w odpowiedzi na zmiany pozostałych procesów, nie robiąc nic pożytecznego. W odróżnieniu od
zakleszczenia, procesy nie są zablokowane, lecz cały czas wykonują obliczenia, ale nie idą do przodu

**Głodzenie** (ang. *starvation*) - sytuacja w której proces który jest gotowy nie jest nigdy wybrany
przez planistę do bycia uruchomionym

###### Przeciwdziałanie
- posiadanie - można kazać procesom alokować wszystkie zasoby jednocześnie, blokując je do momentu
  gdy wszystkie są dostępne
- wywłaszczanie - jeżeli proces posiada jakieś zasoby i nie może wziąć kolejnego, musi zwolnić te posiadane
  lub w drugiej wersji, proces który akurat posiada zasób i inny go potrzebuje, może zostać zmuszony do
  oddania zasobu
- zapętlone oczekiwanie - można zdefiniować liniowy porządek na wszystkich zasobach i pozwalać na zabieranie
  zasobów które są "większe" od tych posiadanych

