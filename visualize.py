import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import numpy as np

df = pd.read_csv("trajectories.csv")

p_columns = [col for col in df.columns if col.startswith('p') and col.endswith('_x')]
num_p = len(p_columns)
e_columns = [col for col in df.columns if col.startswith('e') and col.endswith('_x')]
num_e = len(e_columns)

fig = plt.figure(figsize=(12, 9))
ax = fig.add_subplot(111, projection='3d')
ax.set_title(f'Pursuers vs {num_e} escapers', fontsize=16)
ax.set_xlabel('X (м)')
ax.set_ylabel('Y (м)')
ax.set_zlabel('Z (м)')

p_colors = ['#1E90FF', '#4169E1', '#0000CD', '#00008B', '#191970', '#00BFFF']
lines_p = []
points_p = []
for i in range(num_p):
    color = p_colors[i % len(p_colors)]
    line, = ax.plot([], [], [], color=color, linewidth=2.5, label=f'Pursuer {i+1}')
    point, = ax.plot([], [], [], color=color, marker='o', markersize=9)
    lines_p.append(line)
    points_p.append(point)

e_colors = ['#FF0000', '#DC143C', '#FF4500', '#B22222', '#FF1493', '#C71585']
lines_e = []
points_e = []
for i in range(num_e):
    color = e_colors[i % len(e_colors)]
    line, = ax.plot([], [], [], color=color, linewidth=3.5, label=f'Escaper {i+1}')
    point, = ax.plot([], [], [], color=color, marker='o', markersize=11)
    lines_e.append(line)
    points_e.append(point)

time_text = ax.text2D(0.05, 0.95, "", transform=ax.transAxes, fontsize=14, color='white',
                      bbox=dict(facecolor='black', alpha=0.8))

ax.legend(loc='upper left')
max_range = 17000
ax.set_xlim(0, max_range)
ax.set_ylim(0, max_range)
ax.set_zlim(0, 2000)

def animate(frame):
    t_sec = frame * 0.5
    time_text.set_text(f'Time: {t_sec:.1f} s')

    # ППО
    for i in range(num_p):
        x = df[f'p{i}_x'][:frame+1]
        y = df[f'p{i}_y'][:frame+1]
        z = df[f'p{i}_z'][:frame+1]
        valid = ~(x.isna() | y.isna() | z.isna())
        if valid.any():
            lines_p[i].set_data_3d(x[valid], y[valid], z[valid])
            if valid.iloc[-1]:
                points_p[i].set_data_3d([x.iloc[-1]], [y.iloc[-1]], [z.iloc[-1]])
            else:
                points_p[i].set_data_3d([], [], [])
        else:
            lines_p[i].set_data_3d([], [], [])
            points_p[i].set_data_3d([], [], [])

    for i in range(num_e):
        x = df[f'e{i}_x'][:frame+1]
        y = df[f'e{i}_y'][:frame+1]
        z = df[f'e{i}_z'][:frame+1]
        valid = ~(x.isna() | y.isna() | z.isna())
        if valid.any():
            lines_e[i].set_data_3d(x[valid], y[valid], z[valid])
            if valid.iloc[-1]:
                points_e[i].set_data_3d([x.iloc[-1]], [y.iloc[-1]], [z.iloc[-1]])
            else:
                points_e[i].set_data_3d([], [], [])
        else:
            lines_e[i].set_data_3d([], [], [])
            points_e[i].set_data_3d([], [], [])

    return lines_p + points_p + lines_e + points_e + [time_text]

ani = FuncAnimation(fig, animate, frames=len(df), interval=40, blit=False, repeat=True)
plt.show()