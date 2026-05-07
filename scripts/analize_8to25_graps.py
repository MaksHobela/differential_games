import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import re
import io
import os
import glob

path = './build'
all_files = glob.glob(os.path.join(path, "results_np_*.csv"))

if os.path.exists(os.path.join(path, 'final_all_data.csv')):
    all_files.append(os.path.join(path, 'final_all_data.csv'))

li = []
for filename in all_files:
    try:
        with open(filename, 'r', encoding='utf-8') as f:
            content = f.read()

        fixed_content = content.replace('escaped,', 'escaped |')
        temp_df = pd.read_csv(io.StringIO(fixed_content))

        if not temp_df.empty:
            li.append(temp_df)
    except Exception as e:
        print(f"Error in file {filename}: {e}")

df = pd.concat(li, axis=0, ignore_index=True)

df['Np'] = pd.to_numeric(df['Np'], errors='coerce')
df = df.dropna(subset=['Np']).copy()
df['Np'] = df['Np'].astype(int)

def extract_captured(text):
    text = str(text)
    if "All 10 breached" in text: return 0
    match = re.search(r'(\d+)\s+captured', text)
    return int(match.group(1)) if match else 0

df['RealCaptures'] = df['Outcome'].apply(extract_captured)
df['IsHardFail'] = (df['RealCaptures'] < 5).astype(int)

os.makedirs('analysis_results', exist_ok=True)

scenarios = ['all'] + df['EscaperType'].unique().tolist()

for sc in scenarios:
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 12))

    plot_data = df if sc == 'all' else df[df['EscaperType'] == sc]

    eff_data = plot_data.groupby(['Np', 'Strategy'], as_index=False)['RealCaptures'].mean()
    fail_counts = plot_data.groupby(['Np', 'Strategy'], as_index=False)['IsHardFail'].sum()

    sns.lineplot(data=eff_data, x='Np', y='RealCaptures', hue='Strategy',
                 marker='o', ax=ax1, linewidth=2)
    ax1.set_title(f'СЕРЕДНЯ ЕФЕКТИВНІСТЬ: {sc.upper()}', fontsize=14, fontweight='bold')
    ax1.set_ylabel('Сер. кількість спійманих (з 10)')
    ax1.set_ylim(-0.5, 10.5)
    
    all_nps = sorted(df['Np'].unique())
    if len(all_nps) > 15:
        ax1.set_xticks(all_nps[::2])
    else:
        ax1.set_xticks(all_nps)

    ax1.grid(True, alpha=0.3)

    sns.barplot(data=fail_counts, x='Np', y='IsHardFail', hue='Strategy', ax=ax2)
    ax2.set_title('КІЛЬКІСТЬ "ЖОРСТКИХ ПРОВТИКІВ" (спіймано < 5)', fontsize=12)
    ax2.set_ylabel('К-ть невдалих запусків')
    ax2.grid(True, axis='y', alpha=0.3)

    plt.tight_layout()
    plt.savefig(f'./analysis_results/fixed_plot_{sc}.png')
    plt.close()

print("All good!")