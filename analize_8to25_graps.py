import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import re
import io
import os
import glob

# 1. Завантаження та чистка
# file_path = './build/final_all_data.csv'
# with open(file_path, 'r', encoding='utf-8') as f:
#     content = f.read()

# fixed_content = content.replace('escaped,', 'escaped |')
# df = pd.read_csv(io.StringIO(fixed_content))

path = './build' 
all_files = glob.glob(os.path.join(path, "results_np_*.csv"))

# Додаємо основний файл, якщо він існує і містить дані 8-25
if os.path.exists(os.path.join(path, 'final_all_data.csv')):
    all_files.append(os.path.join(path, 'final_all_data.csv'))

li = []
for filename in all_files:
    try:
        with open(filename, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Виправляємо проблему з комами, яка ламає CSV
        fixed_content = content.replace('escaped,', 'escaped |')
        
        # Використовуємо io.StringIO напряму (це стандарт)
        temp_df = pd.read_csv(io.StringIO(fixed_content))
        
        if not temp_df.empty:
            li.append(temp_df)
    except Exception as e:
        print(f"Error in file {filename}: {e}")

# Злиття всіх шматків в один DataFrame
df = pd.concat(li, axis=0, ignore_index=True)

# Конвертуємо Np в числа і видаляємо сміття
df['Np'] = pd.to_numeric(df['Np'], errors='coerce')
df = df.dropna(subset=['Np']).copy()
df['Np'] = df['Np'].astype(int)

def extract_captured(text):
    text = str(text)
    if "All 10 breached" in text: return 0
    match = re.search(r'(\d+)\s+captured', text)
    return int(match.group(1)) if match else 0

df['RealCaptures'] = df['Outcome'].apply(extract_captured)
# Жорсткий провтик — це коли прорвалося БІЛЬШЕ НІЖ 5 (спіймано < 5)
df['IsHardFail'] = (df['RealCaptures'] < 5).astype(int)

os.makedirs('analysis_results', exist_ok=True)

scenarios = ['all'] + df['EscaperType'].unique().tolist()

for sc in scenarios:
    # Важливо: прибираємо sharex=True, щоб уникнути конфліктів масштабів
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 12))
    
    plot_data = df if sc == 'all' else df[df['EscaperType'] == sc]
    
    # Готуємо дані для верхнього графіка (середнє)
    eff_data = plot_data.groupby(['Np', 'Strategy'], as_index=False)['RealCaptures'].mean()
    
    # Готуємо дані для нижнього графіка (кількість провалів)
    fail_counts = plot_data.groupby(['Np', 'Strategy'], as_index=False)['IsHardFail'].sum()

    # ВЕРХНІЙ: Лінійний графік успіху
    sns.lineplot(data=eff_data, x='Np', y='RealCaptures', hue='Strategy', 
                 marker='o', ax=ax1, linewidth=2)
    ax1.set_title(f'СЕРЕДНЯ ЕФЕКТИВНІСТЬ: {sc.upper()}', fontsize=14, fontweight='bold')
    ax1.set_ylabel('Сер. кількість спійманих (з 10)')
    ax1.set_ylim(-0.5, 10.5)
    # ax1.set_xticks(sorted(df['Np'].unique())) # Явно ставимо мітки
    # ax1.grid(True, alpha=0.3)
    all_nps = sorted(df['Np'].unique())
    if len(all_nps) > 15:
        ax1.set_xticks(all_nps[::2]) # Показуємо кожне друге значення
    else:
        ax1.set_xticks(all_nps)
    
    ax1.grid(True, alpha=0.3)

    # НИЖНІЙ: Стовпчаста діаграма "жорстких провтиків"
    sns.barplot(data=fail_counts, x='Np', y='IsHardFail', hue='Strategy', ax=ax2)
    ax2.set_title('КІЛЬКІСТЬ "ЖОРСТКИХ ПРОВТИКІВ" (спіймано < 5)', fontsize=12)
    ax2.set_ylabel('К-ть невдалих запусків')
    ax2.grid(True, axis='y', alpha=0.3)

    plt.tight_layout()
    plt.savefig(f'./analysis_results/fixed_plot_{sc}.png')
    plt.close()

print("All good!")



# import pandas as pd
# import matplotlib.pyplot as plt
# import seaborn as sns
# import re
# import io
# import os

# # 1. Завантаження та виправлення "битого" CSV
# file_path = './build/final_all_data.csv'
# with open(file_path, 'r', encoding='utf-8') as f:
#     content = f.read()

# fixed_content = content.replace('escaped,', 'escaped |')
# df = pd.read_csv(io.StringIO(fixed_content))

# # 2. Очищення даних
# df = df[df['Np'].astype(str).str.contains('Np') == False].copy()
# df['Np'] = pd.to_numeric(df['Np'])

# def extract_captured(text):
#     text = str(text)
#     if "All 10 breached" in text: return 0
#     match = re.search(r'(\d+)\s+captured', text)
#     return int(match.group(1)) if match else 0

# df['RealCaptures'] = df['Outcome'].apply(extract_captured)

# # Створюємо папку для результатів, щоб не захаращувати проект
# os.makedirs('analysis_results', exist_ok=True)

# # 3. ФУНКЦІЯ ДЛЯ ГЕНЕРАЦІЇ ТАБЛИЦЬ (АВТОМАТИЗОВАНИЙ АНАЛІЗ)
# def save_summary_tables(data, suffix="all"):
#     # Таблиця 1: Середня ефективність (Np vs Strategy)
#     summary = data.groupby(['Np', 'Strategy'])['RealCaptures'].mean().unstack()
#     summary.to_csv(f'analysis_results/mean_efficiency_{suffix}.csv')
    
#     # Таблиця 2: Відсоток провалів (спіймано < 3)
#     data['is_failure'] = data['RealCaptures'] < 3
#     fail_table = data.groupby(['Np', 'Strategy'])['is_failure'].mean().unstack() * 100
#     fail_table.to_csv(f'analysis_results/failure_rate_{suffix}.csv')
#     return summary

# # --- ПОБУДОВА ГРАФІКІВ ---

# # Список сценаріїв: All + 4 специфічних
# scenarios = ['all'] + df['EscaperType'].unique().tolist()

# for sc in scenarios:
#     plt.figure(figsize=(10, 6))
    
#     if sc == 'all':
#         plot_data = df
#         title_suffix = "Усі типи втікачів"
#     else:
#         plot_data = df[df['EscaperType'] == sc]
#         title_suffix = f"Тип втікачів: {sc}"
    
#     # Розрахунок середнього для графіка
#     current_pivot = plot_data.groupby(['Np', 'Strategy'], as_index=False)['RealCaptures'].mean()
    
#     # Малюємо
#     sns.lineplot(data=current_pivot, x='Np', y='RealCaptures', hue='Strategy', marker='o', errorbar=None)
#     plt.title(f'Ефективність перехоплення ({title_suffix})')
#     plt.ylabel('Середнє число спійманих (з 10)')
#     plt.xlabel('Кількість перехоплювачів (Np)')
#     plt.grid(True, alpha=0.3)
#     plt.ylim(0, 10.5) # Фіксуємо шкалу для порівняння
    
#     # Збереження графіка
#     plt.savefig(f'analysis_results/plot_{sc}.png')
#     plt.close() # Закриваємо, щоб не перевантажувати пам'ять
    
#     # Збереження таблиць
#     save_summary_tables(plot_data, suffix=sc)

# print("Аналіз завершено!")
# print("У папці 'analysis_results' створено:")
# print("- 5 графіків (PNG)")
# print("- 10 таблиць (CSV) для автоматизованого аналізу")










# # import pandas as pd
# # import matplotlib.pyplot as plt
# # import seaborn as sns
# # import re
# # import io

# # # 1. Читаємо файл як текст і виправляємо проблему з комами в Outcome
# # file_path = './build/final_all_data.csv'
# # with open(file_path, 'r', encoding='utf-8') as f:
# #     content = f.read()

# # # Замінюємо кому всередині дужок, щоб вона не розбивала колонки
# # # "escaped, 2 captured" -> "escaped | 2 captured"
# # fixed_content = content.replace('escaped,', 'escaped |')

# # # 2. Завантажуємо виправлений текст у Pandas
# # df = pd.read_csv(io.StringIO(fixed_content))

# # def process_and_plot(df):
# #     # Очищуємо від можливих дублікатів заголовків
# #     df = df[df['Np'].astype(str).str.contains('Np') == False].copy()
# #     df['Np'] = pd.to_numeric(df['Np'])
    
# #     # Витягуємо кількість спійманих (тепер після риски '|')
# #     def extract_captured(text):
# #         text = str(text)
# #         if "All 10 breached" in text: return 0
# #         match = re.search(r'(\d+)\s+captured', text)
# #         return int(match.group(1)) if match else 0

# #     df['RealCaptures'] = df['Outcome'].apply(extract_captured)
    
# #     # 3. Агрегація даних
# #     # Середня ефективність
# #     pivot_mean = df.groupby(['Np', 'Strategy', 'EscaperType'], as_index=False)['RealCaptures'].mean()
    
# #     # Відсоток невдач (спіймано менше 3-х)
# #     df['is_failure'] = df['RealCaptures'] < 3
# #     failure_rate = df.groupby(['Np', 'Strategy'], as_index=False)['is_failure'].mean()
# #     failure_rate['failure_percentage'] = failure_rate['is_failure'] * 100

# #     # 4. Побудова графіків
# #     plt.figure(figsize=(12, 10))

# #     # Графік 1: Ефективність
# #     plt.subplot(2, 1, 1)
# #     sns.lineplot(data=pivot_mean, x='Np', y='RealCaptures', hue='Strategy', marker='o', errorbar=None)
# #     plt.title('Ефективність стратегій (Середня кількість збитих цілей)')
# #     plt.ylabel('Спіймано (із 10)')
# #     plt.grid(True, alpha=0.3)

# #     # Графік 2: Провали
# #     plt.subplot(2, 1, 2)
# #     sns.barplot(data=failure_rate, x='Np', y='failure_percentage', hue='Strategy')
# #     plt.title('Надійність: % симуляцій, де спіймали менше 3-х втікачів')
# #     plt.ylabel('% провалів')

# #     plt.tight_layout()
# #     plt.savefig('final_simulation_analysis.png')
# #     plt.show()

# # # Запуск
# # process_and_plot(df)
