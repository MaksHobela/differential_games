import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from mpl_toolkits.mplot3d import Axes3D
import matplotlib.ticker as ticker

df = pd.read_csv('./build/log_Chess.csv')
step_skip = 20
df_sampled = df[df['step'] % step_skip == 0].copy()

escapers = df_sampled[df_sampled['type'] == 'escaper']
pursuers = df_sampled[df_sampled['type'] == 'pursuer']

fig = plt.figure(figsize=(16, 8))
ax1 = fig.add_subplot(121, projection='3d')
ax2 = fig.add_subplot(122)

for eid in escapers['id'].unique():
    data = escapers[escapers['id'] == eid]
    ax1.plot(data['x'], data['y'], data['z'], label=f'Escaper {eid}', linewidth=2)

for pid in pursuers['id'].unique():
    data = pursuers[pursuers['id'] == pid]
    ax1.plot(data['x'], data['y'], data['z'], linestyle='--', label=f'Pursuer {pid}')

ax1.set_title('3D Траєкторії')

v_p = 86.11
dt = 0.01
dist_per_step = v_p * dt

results_for_csv = []

for pid in pursuers['id'].unique():
    p_data = pursuers[pursuers['id'] == pid]
    
    current_p_travelled = []
    current_p_distances = []
    
    for _, row in p_data.iterrows():
        step_escapers = escapers[escapers['step'] == row['step']]
        
        if not step_escapers.empty:
            travelled = row['step'] * dist_per_step
            
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
    
    line, = ax2.plot(current_p_travelled, current_p_distances, label=f'P{pid}', alpha=0.7)
    
    if current_p_travelled:
        ax2.scatter(current_p_travelled[-1], current_p_distances[-1], 
                    color=line.get_color(), s=50, edgecolors='black', zorder=5)

ax2.set_yscale('log')
ax2.axhline(y=10, color='red', linestyle='--', alpha=0.5, label='Threshold 10m')

ax2.set_xlabel('Travelled distance by Pursuer (Meters)')
ax2.set_ylabel('Distance to Target (Meters, Log Scale)')
ax2.set_title('Зближення: Пройдений шлях vs Відстань до цілі')

ax2.yaxis.set_major_formatter(ticker.ScalarFormatter())
ax2.yaxis.get_major_formatter().set_scientific(False)

ax2.grid(True, which="both", ls="-", alpha=0.3)
ax2.legend(loc='upper left', bbox_to_anchor=(1, 1))

plt.tight_layout()
plt.show()
plt.savefig('simulation_report.png', dpi=200)