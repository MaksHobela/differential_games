import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

# -----------------------------
# Початкові дані
# -----------------------------
E0 = np.array([0, 0, 0])       # початкова точка втікача
u = np.array([1, 0.5, 0.2])    # напрямок втікача
u = u / np.linalg.norm(u)       # нормалізація

P0 = [np.array([5, -2, 1]),
      np.array([-3, 4, 2]),
      np.array([2, 5, -1])]   # початкові точки переслідувачів

beta = [1.5, 1.2, 1.3]          # швидкості переслідувачів / втікача

# -----------------------------
# Розрахунок r_i і сфер Аполлонія
# -----------------------------
fig = plt.figure(figsize=(10,8))
ax = fig.add_subplot(111, projection='3d')

# Візуалізація траєкторії втікача
t_vals = np.linspace(0, 10, 500)
E_traj = np.array([E0 + t*u for t in t_vals])
ax.plot(E_traj[:,0], E_traj[:,1], E_traj[:,2], 'r-', label='Evader path', linewidth=2)

# Візуалізація переслідувачів і сфер Аполлонія
for i in range(3):
    r_i = E0 - P0[i]

    # час перехоплення
    A = 1 - beta[i]**2
    B = 2 * np.dot(r_i, u)
    C = np.dot(r_i, r_i)
    discr = B**2 - 4*A*C
    if discr < 0:
        print(f"P{i+1} cannot catch Evader on this line.")
        continue
    t1 = (-B + np.sqrt(discr)) / (2*A)
    t2 = (-B - np.sqrt(discr)) / (2*A)
    t_cross = [t for t in [t1, t2] if t > 0]
    
    # точки перехоплення
    X_cross = [E0 + t*u for t in t_cross]

    # графіка точок перехоплення
    for X in X_cross:
        ax.scatter(X[0], X[1], X[2], s=100, label=f'Intercept P{i+1}', marker='o')
    
    # сфера Аполлонія
    # радіус = |X - E0| * beta_i
    # для візуалізації беремо t_max
    t_max = 10
    center = (E0 + beta[i]**2 * P0[i]) / (1 + beta[i]**2)  # приблизно
    radius = beta[i]*t_max / np.sqrt(1 + beta[i]**2)       # приблизно
    u_sphere, v_sphere = np.mgrid[0:2*np.pi:30j, 0:np.pi:15j]
    x_sphere = center[0] + radius*np.cos(u_sphere)*np.sin(v_sphere)
    y_sphere = center[1] + radius*np.sin(u_sphere)*np.sin(v_sphere)
    z_sphere = center[2] + radius*np.cos(v_sphere)
    ax.plot_wireframe(x_sphere, y_sphere, z_sphere, color='gray', alpha=0.3)
    
    # переслідувач
    ax.scatter(P0[i][0], P0[i][1], P0[i][2], s=80, label=f'P{i+1} start', marker='^')

# -----------------------------
# Налаштування графіка
# -----------------------------
ax.set_xlabel('X')
ax.set_ylabel('Y')
ax.set_zlabel('Z')
ax.legend()
ax.set_title('3D Evader & Pursuers with Apollonius Spheres')
plt.show()