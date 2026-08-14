# =========================
# Compiler Settings
# =========================

CC = gcc
CFLAGS = -Wall -pthread

# =========================
# Targets
# =========================

all: discovery server_fork server_thread server_select client loadtest

# =========================
# Discovery Server
# =========================

discovery: discovery_server.c
	$(CC) discovery_server.c -o discovery $(CFLAGS)

# =========================
# Fork Server
# =========================

server_fork: server_fork.c history.c monitor.c
	$(CC) server_fork.c history.c monitor.c -o server_fork $(CFLAGS)

# =========================
# Thread Server
# =========================

server_thread: server_thread.c server_core_thread_select.c history.c monitor.c
	$(CC) server_thread.c server_core_thread_select.c history.c monitor.c -o server_thread $(CFLAGS)

# =========================
# Select Server
# =========================

server_select: server_select.c server_core_thread_select.c history.c monitor.c
	$(CC) server_select.c server_core_thread_select.c history.c monitor.c -o server_select $(CFLAGS)

# =========================
# Client
# =========================

client: chat_client.c
	$(CC) chat_client.c -o client $(CFLAGS)

# =========================
# Load Test
# =========================

loadtest: loadtest.c
	$(CC) loadtest.c -o loadtest $(CFLAGS)

# =========================
# Clean Everything
# =========================

clean:
	rm -f discovery server_fork server_thread server_select client loadtest
	rm -f *.txt
	rm -f *.png
	rm -f *.log
	rm -f chat_history.json

# =========================
# Rebuild Everything
# =========================

rebuild: clean all