Run:
```
cd differential_games
bash run.sh
```

Якщо хочеш зробити чисту збірку:
```
cd differential_games
bash run.sh clean
bash run.sh
```

Щоб запускати тільки одну стратегію:
```
cd differential_games
bash run.sh -L      # тільки Line
bash run.sh -C      # тільки Chess
bash run.sh -A      # тільки Arc
```

За замовчуванням працює в "quick" режимі з меншим набором Np і до 20 сценаріїв. Щоб запустити повноцінний аналіз з до 200 дронами:
```
cd differential_games
bash run.sh --full
```

Щоб створити графік у папці `png/` і показати його на екрані:
```
cd differential_games
bash run.sh --show
```

Комбінації з вибором стратегії:
```
bash run.sh -L --show    # тільки Line
bash run.sh -C --show    # тільки Chess
bash run.sh -A --show    # тільки Arc
```

Якщо файл `escaper_scenarios.csv` відсутній, `run.sh` сам згенерує його з `scripts/generate_escaper_lock.py`.

Після запуску результати зберігаються у каталозі `differential_games/build/results/` у файлах `results_np_<Np>.csv`.

Якщо хочеш запускати без MPI (один процес):
```
cd differential_games/build
./program ../escaper_scenarios.csv
```
