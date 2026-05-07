import os
import subprocess
import sys
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from mpl_toolkits.mplot3d import Axes3D
import matplotlib.ticker as ticker

NP_MIN = 8
NP_MAX = 40
EXECUTABLE = "./build/program" 
LOG_ROOT = "./surgical_viz"

def run_simulation():
    print(f"--- Starting Surgical Simulation (Np: {NP_MIN}-{NP_MAX}) ---")
    try:
        subprocess.run(["mpirun", "-np", "4", EXECUTABLE, "--surgical"], check=True)
    except subprocess.CalledProcessError:
        print("Error: C++ simulation failed! Check your build folder and executable name.")
        sys.exit(1)

def internal_plotter(csv_path, img_path):
    try:
        df = pd.read_csv(csv_path)
        if df.empty:
            return

        step_skip = 20
        df_sampled = df[df['step'] % step_skip == 0].copy()

        escapers = df_sampled[df_sampled['type'] == 'escaper']
        pursuers = df_sampled[df_sampled['type'] == 'pursuer']

        fig = plt.figure(figsize=(16, 8))
        
        ax1 = fig.add_subplot(121, projection='3d')
        for eid in escapers['id'].unique():
            data = escapers[escapers['id'] == eid]
            ax1.plot(data['x'], data['y'], data['z'], label=f'Escaper {eid}', linewidth=2)

        for pid in pursuers['id'].unique():
            data = pursuers[pursuers['id'] == pid]
            ax1.plot(data['x'], data['y'], data['z'], linestyle='--', label=f'Pursuer {pid}', alpha=0.6)
        
        ax1.set_title(f"3D Траєкторії (Файл: {os.path.basename(csv_path)})")
        ax1.set_xlabel('X'); ax1.set_ylabel('Y'); ax1.set_zlabel('Z')

        ax2 = fig.add_subplot(122)
        v_p = 86.11
        dt = 0.01
        dist_per_step = v_p * dt 

        for pid in pursuers['id'].unique():
            p_data = pursuers[pursuers['id'] == pid]
            travelled = []
            distances = []
            
            for _, row in p_data.iterrows():
                step_esc = escapers[escapers['step'] == row['step']]
                if not step_esc.empty:
                    travelled.append(row['step'] * dist_per_step)
                    dx = step_esc['x'] - row['x']
                    dy = step_esc['y'] - row['y']
                    dz = step_esc['z'] - row['z']
                    min_dist = np.sqrt(dx**2 + dy**2 + dz**2).min()
                    distances.append(max(min_dist, 1e-3))
            
            line, = ax2.plot(travelled, distances, label=f'P{pid}', alpha=0.7)
            if travelled:
                ax2.scatter(travelled[-1], distances[-1], color=line.get_color(), s=40, edgecolors='black')

        ax2.set_yscale('log')
        ax2.axhline(y=10, color='red', linestyle='--', alpha=0.5, label='Threshold 10m')
        ax2.set_xlabel('Пройдена відстань (метри)')
        ax2.set_ylabel('Відстань до цілі (метри, Log Scale)')
        ax2.set_title('Динаміка зближення')
        
        ax2.yaxis.set_major_formatter(ticker.ScalarFormatter())
        ax2.yaxis.get_major_formatter().set_scientific(False)
        ax2.grid(True, which="both", ls="-", alpha=0.2)
        ax2.legend(loc='upper left', bbox_to_anchor=(1, 1), fontsize='small')

        plt.tight_layout()
        
        plt.savefig(img_path, dpi=200)
        plt.close(fig)
        print(f"      [OK] Saved plot to: {img_path}")

    except Exception as e:
        print(f"      [Error] Could not plot {csv_path}: {e}")

def generate_plots():
    print("--- Generating Plots ---")
    if not os.path.exists(LOG_ROOT):
        print(f"Error: Directory {LOG_ROOT} not found!")
        return

    count = 0
    for root, dirs, files in os.walk(LOG_ROOT):
        for file in files:
            if file.endswith(".csv") and "move_" in file:
                csv_path = os.path.join(root, file)
                img_path = csv_path.replace(".csv", ".png")
                
                print(f"Plotting: {csv_path}")
                internal_plotter(csv_path, img_path)
                count += 1
    
    if count == 0:
        print("No matching CSV files found. Check if C++ output matches 'move_*.csv'")

if __name__ == "__main__":
    run_simulation()
    generate_plots()
    print(f"\n[SUCCESS] Process finished. Check folders in '{LOG_ROOT}'")