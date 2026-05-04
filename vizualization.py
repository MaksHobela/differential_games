import pandas as pd
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

def visualize():
    try:
        df = pd.read_csv('simulation_log.csv')
    except:
        print("Файл не знайдено. Спочатку запусти C++ програму.")
        return

    fig = plt.figure(figsize=(10, 8))
    ax = fig.add_subplot(111, projection='3d')

    # Малюємо траєкторії втікачів (синім)
    for agent_id in df[df['type'] == 'escaper']['id'].unique():
        data = df[(df['type'] == 'escaper') & (df['id'] == agent_id)]
        ax.plot(data['x'], data['y'], data['z'], label=f'Escaper {agent_id}', color='blue', alpha=0.6)

    # Малюємо траєкторії переслідувачів (червоним)
    for agent_id in df[df['type'] == 'pursuer']['id'].unique():
        data = df[(df['type'] == 'pursuer') & (df['id'] == agent_id)]
        ax.plot(data['x'], data['y'], data['z'], label=f'Pursuer {agent_id}', color='red', alpha=0.6)

    ax.set_xlabel('X')
    ax.set_ylabel('Y')
    ax.set_zlabel('Z')
    ax.set_title('3D Pursuit Simulation')
    # Обмежимо легенду, якщо агентів забагато
    if len(df['id'].unique()) < 10:
        ax.legend()
    
    plt.show()

if __name__ == "__main__":
    visualize()