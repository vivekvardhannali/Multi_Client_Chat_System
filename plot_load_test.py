import matplotlib.pyplot as plt
import numpy as np

# ===== Helper Functions =====

def average_latency(file):
    try:
        with open(file) as f:
            values = [float(line.strip()) for line in f if line.strip()]
        return np.mean(values) if values else 0
    except:
        return 0

def average_cpu_memory_pss(file):
    cpu_vals = []
    vmrss_vals = []
    pss_vals = []

    try:
        with open(file) as f:
            next(f)  # skip header
            for line in f:
                parts = line.split()
                if len(parts) >= 4:
                    cpu_vals.append(float(parts[1]))
                    vmrss_vals.append(float(parts[2]))
                    pss_vals.append(float(parts[3]))
    except:
        return 0, 0, 0

    avg_cpu = np.mean(cpu_vals) if cpu_vals else 0
    avg_vmrss = np.mean(vmrss_vals) if vmrss_vals else 0
    avg_pss = np.mean(pss_vals) if pss_vals else 0

    return avg_cpu, avg_vmrss, avg_pss


# ===== Read Data =====

servers = ["Fork", "Thread", "Select"]

latencies = [
    average_latency("fork_latency.txt"),
    average_latency("thread_latency.txt"),
    average_latency("select_latency.txt")
]

cpu_vals = []
vmrss_vals = []
pss_vals = []

for name in ["fork", "thread", "select"]:
    cpu, vmrss, pss = average_cpu_memory_pss(f"{name}_metrics.txt")
    cpu_vals.append(cpu)
    vmrss_vals.append(vmrss)
    pss_vals.append(pss)


# ===== Plot Latency =====

plt.figure()
plt.plot(servers, latencies, marker='o')
plt.title("Load Test - Average Latency (10 Clients)")
plt.ylabel("Average Latency (microseconds)")
plt.grid(True)
plt.savefig("loadtest_latency.png")


# ===== Plot CPU =====

plt.figure()
plt.plot(servers, cpu_vals, marker='o')
plt.title("Load Test - Average CPU Usage (10 Clients)")
plt.ylabel("Average CPU (%)")
plt.grid(True)
plt.savefig("loadtest_cpu.png")


# ===== Plot VmRSS =====

plt.figure()
plt.plot(servers, vmrss_vals, marker='o')
plt.title("Load Test - Average VmRSS (10 Clients)")
plt.ylabel("Average VmRSS (KB)")
plt.grid(True)
plt.savefig("loadtest_vmrss.png")


# ===== Plot PSS =====

plt.figure()
plt.plot(servers, pss_vals, marker='o')
plt.title("Load Test - Average PSS (10 Clients)")
plt.ylabel("Average PSS (KB)")
plt.grid(True)
plt.savefig("loadtest_pss.png")

plt.show()