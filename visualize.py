import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

df = pd.read_csv("trajectories.csv")

p_columns = [col for col in df.columns if col.startswith('p') and col.endswith('_x')]
num_p = len(p_columns)
e_columns = [col for col in df.columns if col.startswith('e') and col.endswith('_x')]
num_e = len(e_columns)

fig = plt.figure(figsize=(12, 9))
ax = fig.add_subplot(111, projection='3d')
ax.set_title(f'ППО vs {num_e} Шахед(ів) — Реальна 3D анімація', fontsize=16)
ax.set_xlabel('X (м)')
ax.set_ylabel('Y (м)')
ax.set_zlabel('Z (м)')

# ППО — сині
lines_p = []
points_p = []
for i in range(num_p):
    line, = ax.plot([], [], [], 'b-', linewidth=2, label=f'ППО {i+1}')
    point, = ax.plot([], [], [], 'bo', markersize=8)
    lines_p.append(line)
    points_p.append(point)

# Шахеди — різні яскраві кольори
shahed_colors = ['red', 'darkred', 'orangered', 'crimson', 'purple', 'magenta']
lines_e = []
points_e = []
for i in range(num_e):
    color = shahed_colors[i % len(shahed_colors)]
    line, = ax.plot([], [], [], color=color, linewidth=3, label=f'Шахед {i}')
    point, = ax.plot([], [], [], color=color, marker='o', markersize=10)
    lines_e.append(line)
    points_e.append(point)

time_text = ax.text2D(0.05, 0.95, "", transform=ax.transAxes, fontsize=14, color='white',
                      bbox=dict(facecolor='black', alpha=0.7))

ax.legend(loc='upper left')
max_range = max(df.max().max(), 16000)
ax.set_xlim(0, max_range)
ax.set_ylim(0, max_range)
ax.set_zlim(0, 2000)

def animate(frame):
    t_sec = frame * 0.5
    time_text.set_text(f'Час: {t_sec:.1f} с')

    for i in range(num_p):
        x = df[f'p{i}_x'][:frame+1]
        y = df[f'p{i}_y'][:frame+1]
        z = df[f'p{i}_z'][:frame+1]
        lines_p[i].set_data_3d(x, y, z)
        points_p[i].set_data_3d([x.iloc[-1]], [y.iloc[-1]], [z.iloc[-1]])

    for i in range(num_e):
        x = df[f'e{i}_x'][:frame+1]
        y = df[f'e{i}_y'][:frame+1]
        z = df[f'e{i}_z'][:frame+1]
        lines_e[i].set_data_3d(x, y, z)
        points_e[i].set_data_3d([x.iloc[-1]], [y.iloc[-1]], [z.iloc[-1]])

    return lines_p + points_p + lines_e + points_e + [time_text]

ani = FuncAnimation(fig, animate, frames=len(df), interval=40, blit=False, repeat=True)
plt.show()