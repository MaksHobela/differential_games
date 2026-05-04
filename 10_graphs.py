import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from mpl_toolkits.mplot3d import Axes3D

# --- 1. ЗАВАНТАЖЕННЯ ТА ПІДГОТОВКА ---
df = pd.read_csv('./build/log_Chess.csv')
step_skip = 20
df_sampled = df[df['step'] % step_skip == 0].copy()

escapers = df_sampled[df_sampled['type'] == 'escaper']
pursuers = df_sampled[df_sampled['type'] == 'pursuer']

fig = plt.figure(figsize=(16, 8))
ax1 = fig.add_subplot(121, projection='3d')
ax2 = fig.add_subplot(122)

# --- 2. ПОБУДОВА 3D ТРАЄКТОРІЙ ---
for eid in escapers['id'].unique():
    data = escapers[escapers['id'] == eid]
    ax1.plot(data['x'], data['y'], data['z'], label=f'Escaper {eid}', linewidth=2)

for pid in pursuers['id'].unique():
    data = pursuers[pursuers['id'] == pid]
    ax1.plot(data['x'], data['y'], data['z'], linestyle='--', label=f'Pursuer {pid}')

ax1.set_title('3D Траєкторії')


# --- ПАРАМЕТРИ СИМУЛЯЦІЇ ---
v_p = 86.11   # Швидкість переслідувача (м/с)
dt = 0.01      # Часовий крок (с)
dist_per_step = v_p * dt # Відстань за один крок (~0.86м)

# --- АНАЛІЗ ВІДСТАНЕЙ (ПРАВА ПАНЕЛЬ) ---
results_for_csv = []

for pid in pursuers['id'].unique():
    p_data = pursuers[pursuers['id'] == pid]
    
    current_p_travelled = [] # Пройдений шлях переслідувачем (OX)
    current_p_distances = []  # Відстань до цілі (OY)
    
    for _, row in p_data.iterrows():
        step_escapers = escapers[escapers['step'] == row['step']]
        
        if not step_escapers.empty:
            # 1. Рахуємо пройдену дистанцію від початку (OX)
            # Ми множимо номер кроку на відстань за один крок
            travelled = row['step'] * dist_per_step
            
            # 2. Рахуємо поточну відстань до цілі (OY)
            dx = step_escapers['x'] - row['x']
            dy = step_escapers['y'] - row['y']
            dz = step_escapers['z'] - row['z']
            min_dist = np.sqrt(dx**2 + dy**2 + dz**2).min()
            
            current_p_travelled.append(travelled)
            current_p_distances.append(max(min_dist, 1e-3))
            
            results_for_csv.append({
                'travelled_m': travelled,
                'pursuer_id': pid,
                'dist_to_target': min_dist
            })
    
    # Малюємо лінію
    line, = ax2.plot(current_p_travelled, current_p_distances, label=f'P{pid}', alpha=0.7)
    
    # ДОДАЄМО ТОЧКУ В КІНЦІ (Остання позиція переслідувача)
    if current_p_travelled:
        ax2.scatter(current_p_travelled[-1], current_p_distances[-1], 
                    color=line.get_color(), s=50, edgecolors='black', zorder=5)

# --- НАЛАШТУВАННЯ ГРАФІКА ---
ax2.set_yscale('log')
ax2.axhline(y=10, color='red', linestyle='--', alpha=0.5, label='Threshold 10m')

ax2.set_xlabel('Travelled distance by Pursuer (Meters)') # Тепер тут метри!
ax2.set_ylabel('Distance to Target (Meters, Log Scale)')
ax2.set_title('Зближення: Пройдений шлях vs Відстань до цілі')

# Робимо шкалу Y зручною для читання
import matplotlib.ticker as ticker
ax2.yaxis.set_major_formatter(ticker.ScalarFormatter())
ax2.yaxis.get_major_formatter().set_scientific(False)

ax2.grid(True, which="both", ls="-", alpha=0.3)
ax2.legend(loc='upper left', bbox_to_anchor=(1, 1))

plt.tight_layout()
plt.show()
plt.savefig('simulation_report.png', dpi=200)




# # --- 3. АНАЛІЗ ВІДСТАНЕЙ (ЛОГАРИФМІЧНА ШКАЛА) ---
# results_for_csv = []

# for pid in pursuers['id'].unique():
#     p_data = pursuers[pursuers['id'] == pid]
#     current_p_steps = []
#     current_p_distances = []
    
#     for _, row in p_data.iterrows():
#         step_escapers = escapers[escapers['step'] == row['step']]
#         if not step_escapers.empty:
#             dx = step_escapers['x'] - row['x']
#             dy = step_escapers['y'] - row['y']
#             dz = step_escapers['z'] - row['z']
#             min_dist = np.sqrt(dx**2 + dy**2 + dz**2).min()
            
#             # Додаємо дуже мале число (epsilon), щоб уникнути log(0)
#             current_p_steps.append(row['step'])
#             current_p_distances.append(max(min_dist, 1e-3)) 
            
#             results_for_csv.append({'step': row['step'], 'pursuer_id': pid, 'min_distance': min_dist})
    
#     ax2.plot(current_p_steps, current_p_distances, label=f'P{pid}')

# # Налаштування логарифмічної шкали
# ax2.set_yscale('log') # ОСЬ ЦЕЙ РЯДОК

# ax2.set_xlabel('Step')
# ax2.set_ylabel('Distance (log scale)')
# ax2.set_title('Динаміка зближення (Log scale)')
# ax2.grid(True, which="both", ls="-", alpha=0.5) # Додаємо сітку для обох рівнів шкали
# ax2.legend()

# # --- 4. ЕКСПОРТ ---
# pd.DataFrame(results_for_csv).to_csv('interception_analysis.csv', index=False)

# plt.tight_layout()
# plt.show()




# # # будуємо симуляції з файлу


# import pandas as pd
# import matplotlib.pyplot as plt
# import numpy as np
# from mpl_toolkits.mplot3d import Axes3D

# # --- 1. ЗАВАНТАЖЕННЯ ТА ПІДГОТОВКА ---
# df = pd.read_csv('log_Arc.csv')

# # Децимація (кожен 20-й крок) для продуктивності візуалізації
# step_skip = 20
# df_sampled = df[df['step'] % step_skip == 0].copy()

# # Розділяємо на типи для зручності
# escapers = df_sampled[df_sampled['type'] == 'escaper']
# pursuers = df_sampled[df_sampled['type'] == 'pursuer']

# # Створюємо фігуру
# fig = plt.figure(figsize=(16, 8))
# ax1 = fig.add_subplot(121, projection='3d') # 3D траєкторії
# ax2 = fig.add_subplot(122)                   # Графік відстаней

# # --- 2. ПОБУДОВА 3D ТРАЄКТОРІЙ (ЛІВА ПАНЕЛЬ) ---
# # Утікачі (Escapers)
# for eid in escapers['id'].unique():
#     data = escapers[escapers['id'] == eid]
#     ax1.plot(data['x'], data['y'], data['z'], label=f'Escaper {eid}', linewidth=2, alpha=0.9)
#     ax1.scatter(data['x'].iloc[0], data['y'].iloc[0], data['z'].iloc[0], marker='o', s=30)

# # Переслідувачі (Pursuers)
# for pid in pursuers['id'].unique():
#     data = pursuers[pursuers['id'] == pid]
#     ax1.plot(data['x'], data['y'], data['z'], linestyle='--', label=f'Pursuer {pid}', alpha=0.7)

# ax1.set_xlabel('X')
# ax1.set_ylabel('Y')
# ax1.set_zlabel('Z')
# ax1.set_title('3D Траєкторії зближення')
# ax1.legend(loc='upper left', bbox_to_anchor=(1.05, 1), fontsize='small')

# # --- 3. АНАЛІЗ ВІДСТАНЕЙ ТА ЗБЕРЕЖЕННЯ (ПРАВА ПАНЕЛЬ) ---
# results_for_csv = []

# for pid in pursuers['id'].unique():
#     p_data = pursuers[pursuers['id'] == pid]
    
#     # Списки для малювання поточного переслідувача
#     current_p_steps = []
#     current_p_distances = []
    
#     for _, row in p_data.iterrows():
#         # Шукаємо утікачів на тому ж кроці
#         step_escapers = escapers[escapers['step'] == row['step']]
        
#         if not step_escapers.empty:
#             # Математика відстаней до всіх цілей
#             dx = step_escapers['x'] - row['x']
#             dy = step_escapers['y'] - row['y']
#             dz = step_escapers['z'] - row['z']
#             min_dist = np.sqrt(dx**2 + dy**2 + dz**2).min()
            
#             # Зберігаємо для графіка
#             current_p_steps.append(row['step'])
#             current_p_distances.append(min_dist)
            
#             # Зберігаємо для CSV
#             results_for_csv.append({
#                 'step': row['step'],
#                 'pursuer_id': pid,
#                 'min_distance': min_dist
#             })
    
#     # Малюємо лінію на правому графіку
#     ax2.plot(current_p_steps, current_p_distances, label=f'P{pid} dist to target')

# # Налаштування правого графіка
# ax2.set_xlabel('Step')
# ax2.set_ylabel('Distance')
# ax2.set_title('Динаміка зближення (Аналіз)')
# ax2.grid(True, linestyle='--', alpha=0.6)
# ax2.legend(loc='upper right', fontsize='small')

# # --- 4. ЕКСПОРТ ТА ВИВІД ---
# # Зберігаємо результати аналізу в файл
# df_final_results = pd.DataFrame(results_for_csv)
# df_final_results.to_csv('interception_analysis.csv', index=False)

# print(f"Аналіз завершено!")
# print(f"1. Дані збережено в: 'interception_analysis.csv'")
# print(f"2. Побудовано графіків для {len(pursuers['id'].unique())} переслідувачів.")

# plt.tight_layout()
# # Можна розкоментувати наступний рядок, щоб графік автоматично зберігався як картинка
# # plt.savefig('simulation_report.png', dpi=200)
# plt.show()



# # import pandas as pd
# # import matplotlib.pyplot as plt
# # import numpy as np
# # from mpl_toolkits.mplot3d import Axes3D

# # # 1. Завантаження даних
# # # Припустимо, файл називається 'simulation_data.csv'
# # df = pd.read_csv('log_Arc.csv')

# # # 2. Децимація (беремо кожен 20-й крок, щоб не перевантажувати графік)
# # step_skip = 20
# # df_sampled = df[df['step'] % step_skip == 0]

# # # Створюємо фігуру з двома підграфіками: Траєкторії та Відстані
# # fig = plt.figure(figsize=(15, 7))
# # ax1 = fig.add_subplot(121, projection='3d') # 3D сцена
# # ax2 = fig.add_subplot(122)                   # Графік відстаней

# # # Розділяємо на типи
# # escapers = df_sampled[df_sampled['type'] == 'escaper']
# # pursuers = df_sampled[df_sampled['type'] == 'pursuer']

# # # 3. Побудова траєкторій утікачів
# # for eid in escapers['id'].unique():
# #     data = escapers[escapers['id'] == eid]
# #     ax1.plot(data['x'], data['y'], data['z'], label=f'Escaper {eid}', linewidth=3, alpha=0.8)
# #     # Початкова точка
# #     ax1.scatter(data['x'].iloc[0], data['y'].iloc[0], data['z'].iloc[0], marker='o')

# # # 4. Побудова траєкторій переслідувачів
# # for pid in pursuers['id'].unique():
# #     data = pursuers[pursuers['id'] == pid]
# #     ax1.plot(data['x'], data['y'], data['z'], linestyle='--', label=f'Pursuer {pid}', alpha=0.6)

# # ax1.set_xlabel('X')
# # ax1.set_ylabel('Y')
# # ax1.set_zlabel('Z')
# # ax1.set_title('3D Траєкторії зближення')
# # ax1.legend(loc='upper left', bbox_to_anchor=(1.05, 1))

# # # 5. Аналіз відстані (Оптимальність)
# # # Для кожного переслідувача знайдемо відстань до його найближчого утікача в кожний момент часу

# # # --- ПІДГОТОВКА СПИСКУ ДЛЯ ЗБЕРЕЖЕННЯ ДАНИХ ---
# # # ... (завантаження даних залишається таким самим)

# # # --- НОВА ЧАСТИНА: ОБЧИСЛЕННЯ ТА ЗБЕРЕЖЕННЯ ---
# # results = []

# # for pid in pursuers['id'].unique():
# #     p_data = pursuers[pursuers['id'] == pid]
    
# #     for _, row in p_data.iterrows():
# #         # Знаходимо утікачів саме на цьому кроці
# #         current_escapers = escapers[escapers['step'] == row['step']]
        
# #         if not current_escapers.empty:
# #             # Обчислюємо відстані до всіх утікачів одночасно (векторно)
# #             dists = np.sqrt(
# #                 (current_escapers['x'] - row['x'])**2 + 
# #                 (current_escapers['y'] - row['y'])**2 + 
# #                 (current_escapers['z'] - row['z'])**2
# #             )
# #             min_dist = dists.min()
            
# #             results.append({
# #                 'step': row['step'],
# #                 'pursuer_id': pid,
# #                 'distance': min_dist
# #             })

# # # Зберігаємо в файл
# # df_results = pd.DataFrame(results)
# # df_results.to_csv('interception_analysis.csv', index=False)
# # print("Дані аналізу збережено у файл 'interception_analysis.csv'")

# # # --- ТЕПЕР ВІЗУАЛІЗАЦІЯ ---
# # # (використовуємо вже готовий df_results для ax2)
# # for pid in df_results['pursuer_id'].unique():
# #     subset = df_results[df_results['pursuer_id'] == pid]
# #     ax2.plot(subset['step'], subset['distance'], label=f'P{pid}')

# # # ... (решта коду для оформлення ax2)

# # # Далі може йти plt.show(), але файл вже буде записаний


# # # for pid in pursuers['id'].unique():
# # #     p_data = df_sampled[df_sampled['id'] == pid]
# # #     p_data = p_data[p_data['type'] == 'pursuer']
    
# # #     distances = []
# # #     steps = p_data['step'].values
    
# # #     for _, row in p_data.iterrows():
# # #         # Беремо координати всіх утікачів на цьому ж кроці
# # #         curr_step_escapers = df_sampled[(df_sampled['step'] == row['step']) & (df_sampled['type'] == 'escaper')]
        
# # #         # Рахуємо мінімальну відстань до будь-якого з них
# # #         dist = np.sqrt((curr_step_escapers['x'] - row['x'])**2 + 
# # #                        (curr_step_escapers['y'] - row['y'])**2 + 
# # #                        (curr_step_escapers['z'] - row['z'])**2).min()
# # #         distances.append(dist)
    
# #     ax2.plot(steps, distances, label=f'P{pid} dist to target')

# # ax2.set_xlabel('Step')
# # ax2.set_ylabel('Distance')
# # ax2.set_title('Динаміка зближення (Відстань)')
# # ax2.grid(True, linestyle='--', alpha=0.7)

# # plt.tight_layout()
# # plt.show()