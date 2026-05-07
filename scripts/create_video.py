import os
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
from matplotlib.animation import FuncAnimation, writers
from mpl_toolkits.mplot3d import Axes3D
import argparse

script_dir = os.path.dirname(os.path.abspath(__file__))

parser = argparse.ArgumentParser(description='Animate simulation trajectories.')
parser.add_argument('filename', help='CSV log file')
parser.add_argument('output', help='Output video file (MP4)')
parser.add_argument('--scenario', type=int, default=None, help='Scenario ID to visualize (optional)')
args = parser.parse_args()

def resolve_input_path(filename):
    candidate_paths = [
        filename,
        os.path.join(os.getcwd(), filename),
        os.path.join(script_dir, filename),
        os.path.join(script_dir, '..', filename),
        os.path.join(script_dir, '..', 'build', filename),
        os.path.join(script_dir, '..', 'build', 'results', filename)
    ]
    for candidate in candidate_paths:
        if os.path.exists(candidate):
            return candidate
    return None

def animate_simulation(filename, fileout, scenario_id=None):
    resolved = resolve_input_path(filename)
    if resolved is None:
        print(f"Файл '{filename}' не знайдено. Перевірте шлях до CSV-файлу та запустіть програму C++ перед візуалізацією.")
        return
    try:
        df = pd.read_csv(resolved)
    except Exception as e:
        print(f"Не вдалося прочитати файл '{resolved}': {e}")
        return

    if scenario_id is not None:
        df = df[df['scenario_id'] == scenario_id]
        if df.empty:
            print(f"Сценарій {scenario_id} не знайдено в файлі.")
            return

    fig = plt.figure(figsize=(12, 10))
    ax = fig.add_subplot(111, projection='3d')

    escaper_ids = df[df['type'] == 'escaper']['id'].unique()
    pursuer_ids = df[df['type'] == 'pursuer']['id'].unique()
    steps = sorted(df['step'].unique())

    if len(steps) == 0:
        print("Дані не містять жодного кроку. Перевірте CSV або сценарій візуалізації.")
        return
    print(f"Починаю рендер відео для {len(steps)} кроків і {len(escaper_ids)} втікачів / {len(pursuer_ids)} переслідувачів...")

    lines = {}
    points = {}

    for eid in escaper_ids:
        line, = ax.plot([], [], [], color='blue', alpha=0.4, label=f'Escaper {eid}' if len(escaper_ids) < 5 else "")
        point, = ax.plot([], [], [], 'bo', markersize=6)
        lines[f'e_{eid}'] = line
        points[f'e_{eid}'] = point

    for pid in pursuer_ids:
        line, = ax.plot([], [], [], color='red', alpha=0.3, label=f'Pursuer {pid}' if len(pursuer_ids) < 5 else "")
        point, = ax.plot([], [], [], 'ro', markersize=4)
        lines[f'p_{pid}'] = line
        points[f'p_{pid}'] = point

    ax.set_xlim(-50, 1000)
    ax.set_ylim(-1000, 1000)
    ax.set_zlim(0, df['z'].max() + 50)
    
    ax.set_xlabel('X (м)')
    ax.set_ylabel('Y (м)')
    ax.set_zlabel('Z (Висота)')
    ax.set_title('Live 3D Pursuit-Evasion Simulation')

    def update(frame_step):
        current_data = df[df['step'] <= frame_step]
        
        for eid in escaper_ids:
            agent_history = current_data[(current_data['type'] == 'escaper') & (current_data['id'] == eid)]
            if not agent_history.empty:
                lines[f'e_{eid}'].set_data(agent_history['x'], agent_history['y'])
                lines[f'e_{eid}'].set_3d_properties(agent_history['z'])
                last_pos = agent_history.iloc[-1]
                points[f'e_{eid}'].set_data([last_pos['x']], [last_pos['y']])
                points[f'e_{eid}'].set_3d_properties([last_pos['z']])

        for pid in pursuer_ids:
            agent_history = current_data[(current_data['type'] == 'pursuer') & (current_data['id'] == pid)]
            if not agent_history.empty:
                lines[f'p_{pid}'].set_data(agent_history['x'], agent_history['y'])
                lines[f'p_{pid}'].set_3d_properties(agent_history['z'])
                last_pos = agent_history.iloc[-1]
                points[f'p_{pid}'].set_data([last_pos['x']], [last_pos['y']])
                points[f'p_{pid}'].set_3d_properties([last_pos['z']])
        
        return list(lines.values()) + list(points.values())

    ani = FuncAnimation(fig, update, frames=steps, interval=50, blit=False, repeat=False)

    legend_handles = []
    legend_labels = []
    if len(escaper_ids) > 0:
        legend_handles.append(Line2D([0], [0], color='blue', alpha=0.4, linewidth=3))
        legend_labels.append('Escaper')
    if len(pursuer_ids) > 0:
        legend_handles.append(Line2D([0], [0], color='red', alpha=0.3, linewidth=3))
        legend_labels.append('Pursuer')
    if legend_handles:
        ax.legend(handles=legend_handles, labels=legend_labels, loc='upper left', fontsize='small', ncol=2)

    try:
        if writers.is_available('ffmpeg'):
            print(f"Зберігаю відео в '{fileout}' через ffmpeg...")
            ani.save(fileout, writer='ffmpeg', fps=20, bitrate=1800)
            print(f"Відео успішно збережено як '{fileout}'")
        elif writers.is_available('pillow'):
            gif_out = os.path.splitext(fileout)[0] + '.gif'
            print(f"FFmpeg не знайдено, зберігаю GIF як '{gif_out}'...")
            ani.save(gif_out, writer='pillow', fps=20)
            print(f"GIF успішно збережено як '{gif_out}'")
        else:
            print("Немає доступного записувача відео. Встановіть ffmpeg або pillow для збереження анімації.")
    except Exception as e:
        print(f"Помилка при збереженні: {e}")
        if writers.is_available('pillow'):
            gif_out = os.path.splitext(fileout)[0] + '.gif'
            try:
                ani.save(gif_out, writer='pillow', fps=20)
                print(f"Збережено GIF як '{gif_out}'")
            except Exception as e2:
                print(f"Не вдалося зберегти GIF: {e2}")
        else:
            print("Спробуйте встановити pillow або ffmpeg для збереження анімацій.")

    print("Рендер завершено. Відкриваю графік у вікні...")
    plt.show()

if __name__ == "__main__":
    animate_simulation(args.filename, args.output, args.scenario)