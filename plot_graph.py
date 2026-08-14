import matplotlib.pyplot as plt

def read_summary(file):
    clients = []
    latency = []
    cpu = []
    vmrss = []
    pss = []

    with open(file, 'r') as f:
        next(f)
        for line in f:
            parts = line.split()
            if len(parts) >= 5:
                clients.append(int(parts[0]))
                latency.append(float(parts[1]))
                cpu.append(float(parts[2]))
                vmrss.append(float(parts[3]))
                pss.append(float(parts[4]))

    return clients, latency, cpu, vmrss, pss


fork = read_summary("fork_summary.txt")
thread = read_summary("thread_summary.txt")
select = read_summary("select_summary.txt")

# ===== Latency Graph =====
plt.figure()
plt.plot(fork[0], fork[1], marker='o', label="Fork")
plt.plot(thread[0], thread[1], marker='o', label="Thread")
plt.plot(select[0], select[1], marker='o', label="Select")
plt.xlabel("Number of Clients")
plt.ylabel("Average Latency (microseconds)")
plt.title("Latency vs Clients")
plt.legend()
plt.grid()
plt.savefig("latency_vs_clients.png")


# ===== CPU Graph =====
plt.figure()
plt.plot(fork[0], fork[2], marker='o', label="Fork")
plt.plot(thread[0], thread[2], marker='o', label="Thread")
plt.plot(select[0], select[2], marker='o', label="Select")
plt.xlabel("Number of Clients")
plt.ylabel("Average CPU Usage (%)")
plt.title("CPU vs Clients")
plt.legend()
plt.grid()
plt.savefig("cpu_vs_clients.png")


# ===== VmRSS Graph =====
plt.figure()
plt.plot(fork[0], fork[3], marker='o', label="Fork")
plt.plot(thread[0], thread[3], marker='o', label="Thread")
plt.plot(select[0], select[3], marker='o', label="Select")
plt.xlabel("Number of Clients")
plt.ylabel("Average VmRSS (KB)")
plt.title("VmRSS vs Clients")
plt.legend()
plt.grid()
plt.savefig("vmrss_vs_clients.png")


# ===== PSS Graph =====
plt.figure()
plt.plot(fork[0], fork[4], marker='o', label="Fork")
plt.plot(thread[0], thread[4], marker='o', label="Thread")
plt.plot(select[0], select[4], marker='o', label="Select")
plt.xlabel("Number of Clients")
plt.ylabel("Average PSS (KB)")
plt.title("PSS vs Clients")
plt.legend()
plt.grid()
plt.savefig("pss_vs_clients.png")

plt.show()