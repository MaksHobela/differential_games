import pandas as pd
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from matplotlib.animation import FuncAnimation

def animate_simulation(filename, fileout):
    try:
        df = pd.read_csv(filename)
    except:
        print("Файл не знайдено. Спочатку запусти C++ програму.")
        return

    fig = plt.figure(figsize=(12, 10))
    ax = fig.add_subplot(111, projection='3d')

    # Отримуємо списки ID
    escaper_ids = df[df['type'] == 'escaper']['id'].unique()
    pursuer_ids = df[df['type'] == 'pursuer']['id'].unique()
    steps = sorted(df['step'].unique())

    # Словники для зберігання об'єктів графіки (лінії та точки)
    lines = {}
    points = {}

    # Ініціалізація ліній для втікачів (сині)
    for eid in escaper_ids:
        line, = ax.plot([], [], [], color='blue', alpha=0.4, label=f'Escaper {eid}' if len(escaper_ids) < 5 else "")
        point, = ax.plot([], [], [], 'bo', markersize=6)
        lines[f'e_{eid}'] = line
        points[f'e_{eid}'] = point

    # Ініціалізація ліній для переслідувачів (червоні)
    for pid in pursuer_ids:
        line, = ax.plot([], [], [], color='red', alpha=0.3, label=f'Pursuer {pid}' if len(pursuer_ids) < 5 else "")
        point, = ax.plot([], [], [], 'ro', markersize=4)
        lines[f'p_{pid}'] = line
        points[f'p_{pid}'] = point

    # Налаштування осей (беремо мінімуми/максимуми з усіх даних)
    # ax.set_xlim(df['x'].min(), df['x'].max())
    # ax.set_ylim(df['y'].min(), df['y'].max())
    # ax.set_zlim(0, df['z'].max() + 50)
    ax.set_xlim(-50, 1000)
    ax.set_ylim(-1000, 1000)
    ax.set_zlim(0, df['z'].max() + 50)
    
    ax.set_xlabel('X (м)')
    ax.set_ylabel('Y (м)')
    ax.set_zlabel('Z (Висота)')
    ax.set_title('Live 3D Pursuit-Evasion Simulation')

    def update(frame_step):
        # Отримуємо дані до поточного кроку включно
        current_data = df[df['step'] <= frame_step]
        
        # Оновлюємо втікачів
        for eid in escaper_ids:
            agent_history = current_data[(current_data['type'] == 'escaper') & (current_data['id'] == eid)]
            if not agent_history.empty:
                lines[f'e_{eid}'].set_data(agent_history['x'], agent_history['y'])
                lines[f'e_{eid}'].set_3d_properties(agent_history['z'])
                # Точка — остання позиція
                last_pos = agent_history.iloc[-1]
                points[f'e_{eid}'].set_data([last_pos['x']], [last_pos['y']])
                points[f'e_{eid}'].set_3d_properties([last_pos['z']])

        # Оновлюємо переслідувачів
        for pid in pursuer_ids:
            agent_history = current_data[(current_data['type'] == 'pursuer') & (current_data['id'] == pid)]
            if not agent_history.empty:
                lines[f'p_{pid}'].set_data(agent_history['x'], agent_history['y'])
                lines[f'p_{pid}'].set_3d_properties(agent_history['z'])
                last_pos = agent_history.iloc[-1]
                points[f'p_{pid}'].set_data([last_pos['x']], [last_pos['y']])
                points[f'p_{pid}'].set_3d_properties([last_pos['z']])
        
        return list(lines.values()) + list(points.values())

    # Створюємо анімацію
    # interval - затримка між кадрами в мс (50 мс = 20 кадрів/сек)
    ani = FuncAnimation(fig, update, frames=steps, interval=50, blit=False, repeat=False)

    plt.legend(loc='upper left', fontsize='small', ncol=2)
    try:
        # Використовуємо ffmpeg як райтер. 
        # fps=20 (кількість кадрів на секунду)
        # bitrate=1800 (якість відео)
        ani.save(fileout, writer='ffmpeg', fps=20, bitrate=1800)
        print("Відео успішно збережено як 'simulation_video.mp4'")
    except Exception as e:
        print(f"Помилка при збереженні: {e}")
        print("Спробуйте зберегти в .gif, якщо FFmpeg не встановлено:")
        # ani.save('simulation.gif', writer='pillow', fps=20)
    plt.show()
    # plt.savefig("graph.mp4")

if __name__ == "__main__":
    animate_simulation('log_Arc.csv', 'Arc_vid.mp4')
    # animate_simulation('log_Chess.csv', 'Chess_vid.mp4')
    # animate_simulation('log_Line.csv', 'Line_vid.mp4')
