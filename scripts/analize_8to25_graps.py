import argparse
import subprocess
import pandas as pd
import matplotlib.pyplot as plt
import re
import io
import os
import glob
import csv

parser = argparse.ArgumentParser(description='Generate efficiency graphs from simulation results.')
parser.add_argument('--png-dir', default='./png', help='Directory to save PNG graphs')
parser.add_argument('--max-np', type=int, default=None, help='Maximum Np value to include in the graph')
parser.add_argument('--show', action='store_true', help='Show the generated graph')
parser.add_argument('-L', action='store_true', help='Include Line strategy')
parser.add_argument('-C', action='store_true', help='Include Chess strategy')
parser.add_argument('-A', action='store_true', help='Include Arc strategy')
args = parser.parse_args()

selected_strategies = []
if args.L:
    selected_strategies.append('Line')
if args.C:
    selected_strategies.append('Chess')
if args.A:
    selected_strategies.append('Arc')
if not selected_strategies:
    selected_strategies = ['Line', 'Chess', 'Arc']

script_dir = os.path.dirname(os.path.abspath(__file__))
path = os.path.join(script_dir, 'build', 'results')
if not os.path.exists(path):
    path = os.path.join(script_dir, 'build')

# 1. Завантаження та чистка
# file_path = './build/final_all_data.csv'
# with open(file_path, 'r', encoding='utf-8') as f:
#     content = f.read()

# fixed_content = content.replace('escaped,', 'escaped |')
# df = pd.read_csv(io.StringIO(fixed_content))

all_files = glob.glob(os.path.join(path, "results_np_*.csv"))

# Додаємо основний файл, якщо він існує і містить дані 8-25
if os.path.exists(os.path.join(path, 'final_all_data.csv')):
    all_files.append(os.path.join(path, 'final_all_data.csv'))

if not all_files:
    raise SystemExit(f"No simulation result files found in '{path}'. Please run the simulation first or check the build/results folder.")

li = []
for filename in all_files:
    try:
        with open(filename, 'r', encoding='utf-8') as f:
            content = f.read()

        temp_df = pd.read_csv(io.StringIO(content), quotechar='"', skipinitialspace=True)
        
        if not temp_df.empty:
            li.append(temp_df)
    except Exception as e:
        print(f"Error in file {filename}: {e}")

# Злиття всіх шматків в один DataFrame
if not li:
    raise SystemExit(f"No valid CSV data could be read from {len(all_files)} files in '{path}'. Check the files for correct CSV formatting.")

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
os.makedirs(args.png_dir, exist_ok=True)

if selected_strategies:
    df = df[df['Strategy'].isin(selected_strategies)].copy()

if args.max_np is not None:
    df = df[df['Np'] <= args.max_np].copy()

plot_data = df

eff_data = plot_data.groupby(['Np', 'Strategy'], as_index=False)['RealCaptures'].mean()
eff_data['Efficiency'] = eff_data['RealCaptures'] / 10.0 * 100.0

fig, ax = plt.subplots(figsize=(12, 8))

for strategy in eff_data['Strategy'].unique():
    strategy_data = eff_data[eff_data['Strategy'] == strategy]
    ax.plot(strategy_data['Np'], strategy_data['Efficiency'], marker='o', linewidth=2, label=strategy)

strategy_tag = 'all' if len(selected_strategies) == 3 else '_'.join(selected_strategies)
ax.set_title(f'Ефективність збиття за кількістю дронів ({strategy_tag})', fontsize=16, fontweight='bold')
ax.set_xlabel('Кількість дронів (Np)')
ax.set_ylabel('Ефективність збиття (%)')
ax.set_ylim(0, 100)
ax.grid(True, alpha=0.3)
all_nps = sorted(plot_data['Np'].unique())
if len(all_nps) > 15:
    ax.set_xticks(all_nps[::2])
else:
    ax.set_xticks(all_nps)

plt.tight_layout()
output_file = os.path.join(args.png_dir, f'efficiency_vs_drones_{strategy_tag}.png')
plt.savefig(output_file, dpi=200)
print(f"Saved graph: {output_file}")

if args.show:
    try:
        plt.show()
    except Exception:
        pass
    try:
        subprocess.run(['xdg-open', output_file], check=False)
    except Exception:
        pass

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
