import random
import math

def save_scenarios():
    count_e = 10
    variants_per_type = 25
    filename = "./escaper_scenarios.csv"
    
    # Межі для втікачів
    Y_START, Y_END = 50, 2050
    Z_MIN, Z_MAX = 200, 700
    X_CENTER = 1000
    
    with open(filename, "w") as f:
        # Заголовок для зручності (опціонально)
        # f.write("type,scenario_id,coordinates_x_y_z_separated_by_space\n")
        
        for s_type in ["wedge", "swarm", "stairs", "random"]:
            for v in range(variants_per_type):
                coords = []
                
                if s_type == "wedge":  # 1. КЛИН (V-подібна форма)
                    # Вершина клина випадкова в межах центру
                    apex_y = random.uniform(Y_START + 500, Y_END - 500)
                    z_base = random.uniform(Z_MIN, Z_MAX)
                    for i in range(count_e):
                        side = 1 if i % 2 == 0 else -1
                        step = (i // 2) * 150 # розмах крил
                        x = X_CENTER + (side * step * 0.5)
                        y = apex_y + (step) # йдуть хвостом назад
                        z = z_base
                        coords.append(f"{x:.2f} {y:.2f} {z:.2f}")

                elif s_type == "swarm":  # 2. РІЙ (Купно в центрі)
                    center_y = random.uniform(Y_START + 200, Y_END - 200)
                    center_z = random.uniform(Z_MIN + 100, Z_MAX - 100)
                    for i in range(count_e):
                        x = X_CENTER + random.uniform(-100, 100)
                        y = center_y + random.uniform(-150, 150)
                        z = center_z + random.uniform(-100, 100)
                        coords.append(f"{x:.2f} {y:.2f} {z:.2f}")

                elif s_type == "stairs":  # 3. СХОДИНКИ (Діагональ по Y та Z)
                    start_y = random.uniform(Y_START, Y_START + 500)
                    start_z = random.uniform(Z_MIN, Z_MIN + 100)
                    y_step = (Y_END - start_y) / count_e
                    z_step = (Z_MAX - start_z) / count_e
                    for i in range(count_e):
                        x = X_CENTER + (random.uniform(-50, 50))
                        y = start_y + (i * y_step)
                        z = start_z + (i * z_step)
                        coords.append(f"{x:.2f} {y:.2f} {z:.2f}")

                elif s_type == "random":  # 4. РАНДОМ
                    for i in range(count_e):
                        x = random.uniform(950, 1050)
                        y = random.uniform(Y_START, Y_END)
                        z = random.uniform(Z_MIN, Z_MAX)
                        coords.append(f"{x:.2f} {y:.2f} {z:.2f}")

                # Записуємо тип, ID та всі координати в один рядок
                line = f"{s_type},{v}," + ",".join(coords)
                f.write(line + "\n")

    print(f"Файл {filename} згенеровано. Всього сценаріїв: 100.")

if __name__ == "__main__":
    save_scenarios()