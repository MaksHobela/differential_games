def shift_y_coordinates(input_file, output_file):
    with open(input_file, 'r') as f_in, open(output_file, 'w') as f_out:
        for line in f_in:
            if not line.strip(): continue
            
            parts = line.strip().split(',')
            # Перші дві частини: назва стратегії та номер кроку
            strategy, step = parts[0], parts[1]
            coords_data = parts[2:]
            
            new_coords = []
            for item in coords_data:
                xyz = item.split()
                if len(xyz) == 3:
                    x, y, z = float(xyz[0]), float(xyz[1]), float(xyz[2])
                    # Додаємо +1000 до Y
                    new_coords.append(f"{x:.2f} {y+1000.0:.2f} {z:.2f}")
            
            # Збираємо рядок назад
            f_out.write(f"{strategy},{step}," + ",".join(new_coords) + "\n")

    print(f"Готово! Новий файл збережено: {output_file}")

# Використання
shift_y_coordinates("../escaper_scenarios.csv", "./2000_escaper_scenarios.csv")