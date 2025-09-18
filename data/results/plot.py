import pandas as pd
import matplotlib.pyplot as plt
from io import StringIO

# === Load dataset ===
data = """
algorithm,memory,precision,recall,f1,ARE,AAE
elastic_lambda1/8,100,0.944867,0.998256,0.970819,0.043614,12.706649
elastic_lambda1,100,0.950000,0.999000,0.974000,0.030000,10.000000
2FA_lambda1,100,0.996111,0.995555,0.995832,0.003343,1.440723
2FA_lambda2,100,0.995800,0.995200,0.995500,0.004000,1.600000
2FA_lambda4,100,0.995900,0.995300,0.995600,0.003800,1.550000
2FA_lambda8,100,0.996000,0.995400,0.995700,0.003600,1.520000
"""  # Extend with 200, 300, 400, 500 like your earlier dataset

df = pd.read_csv(StringIO(data))

# === Plotting helpers ===
def plot_all_algorithms(df):
    metrics = {"ARE": "Average Relative Error (ARE)",
               "AAE": "Average Absolute Error (AAE)",
               "recall": "Recall Rate",
               "precision": "Precision Rate"}
    
    for metric, ylabel in metrics.items():
        plt.figure(figsize=(8,6))
        for alg in df['algorithm'].unique():
            subset = df[df['algorithm'] == alg]
            plt.plot(subset['memory'], subset[metric], marker='o', label=alg)
        plt.title(f"{ylabel} vs Memory Usage")
        plt.xlabel("Memory (KB)")
        plt.ylabel(ylabel)
        plt.legend()
        plt.grid(True)
        plt.show()

def plot_elastic_vs_2fa(df):
    metrics = {"ARE": "Average Relative Error (ARE)",
               "AAE": "Average Absolute Error (AAE)"}
    
    for metric, ylabel in metrics.items():
        plt.figure(figsize=(8,6))
        
        # Elastic λ=1/8 and λ=1
        for lam in ["elastic_lambda1/8", "elastic_lambda1"]:
            subset = df[df['algorithm'] == lam]
            plt.plot(subset['memory'], subset[metric], marker='o', label=lam)
        
        # 2FA λ=1,2,4,8
        for lam in ["2FA_lambda1", "2FA_lambda2", "2FA_lambda4", "2FA_lambda8"]:
            subset = df[df['algorithm'] == lam]
            plt.plot(subset['memory'], subset[metric], marker='o', label=lam)
        
        plt.title(f"{ylabel} vs Memory Usage")
        plt.xlabel("Memory (KB)")
        plt.ylabel(ylabel)
        plt.legend()
        plt.grid(True)
        plt.show()

# === Menu Driven Program ===
def main():
    while True:
        print("\nMenu:")
        print("1. Plot four graphs (ARE, AAE, Recall, Precision) vs Memory usage for all algorithms")
        print("2. Plot two graphs (ARE, AAE) vs Memory usage for Elastic (λ=1/8,1) & 2FA (λ=1,2,4,8)")
        print("3. Exit")
        choice = input("Enter choice: ")
        
        if choice == "1":
            plot_all_algorithms(df)
        elif choice == "2":
            plot_elastic_vs_2fa(df)
        elif choice == "3":
            print("Exiting.")
            break
        else:
            print("Invalid choice. Try again.")

if __name__ == "__main__":
    main()
