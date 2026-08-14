#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "monitor.h"

static char log_filename[256];

/* ===== Read total CPU time from /proc/stat ===== */

static long get_pss_kb() {
    FILE *fp = fopen("/proc/self/smaps", "r");
    if (!fp) return 0;

    char line[256];
    long total_pss = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "Pss:", 4) == 0) {
            long value;
            sscanf(line + 4, "%ld", &value);
            total_pss += value;
        }
    }

    fclose(fp);
    return total_pss;  // in KB
}
static unsigned long long get_total_cpu_time() {
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) return 0;

    char line[512];
    fgets(line, sizeof(line), fp);
    fclose(fp);

    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    sscanf(line, "cpu  %llu %llu %llu %llu %llu %llu %llu %llu",
           &user, &nice, &system, &idle,
           &iowait, &irq, &softirq, &steal);

    return user + nice + system + idle +
           iowait + irq + softirq + steal;
}

/* ===== Read process CPU time from /proc/self/stat ===== */
static unsigned long long get_process_cpu_time() {
    FILE *fp = fopen("/proc/self/stat", "r");
    if (!fp) return 0;
unsigned long long utime = 0, stime = 0;
    char buffer[1024];

    fgets(buffer, sizeof(buffer), fp);
    fclose(fp);

    /* utime is field 14, stime is field 15 */
    char *token = strtok(buffer, " ");
    int field = 1;
    while (token != NULL) {
        if (field == 14)
            utime = strtoull(token, NULL, 10);
        if (field == 15) {
            stime = strtoull(token, NULL, 10);
            break;
        }
        token = strtok(NULL, " ");
        field++;
    }

    return utime + stime;
}

/* ===== Read VmRSS from /proc/self/status ===== */
static long get_memory_usage_kb() {
    FILE *fp = fopen("/proc/self/status", "r");
    if (!fp) return 0;

    char line[256];
    long vmrss = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line + 6, "%ld", &vmrss);
            break;
        }
    }

    fclose(fp);
    return vmrss;  // in KB
}

/* ===== Monitoring Thread ===== */

static void *monitor_thread(void *arg) {

    FILE *log = fopen(log_filename, "w");
    if (!log) {
        perror("Failed to open metrics log file");
        return NULL;
    }

    fprintf(log, "time_sec cpu_percent vmrss_kb pss_kb\n");
    fflush(log);

    unsigned long long prev_total = get_total_cpu_time();
    unsigned long long prev_proc  = get_process_cpu_time();

    time_t start_time = time(NULL);

    while (1) {
        sleep(5);
        // sleep(2);

        unsigned long long curr_total = get_total_cpu_time();
        unsigned long long curr_proc  = get_process_cpu_time();

        unsigned long long total_diff = curr_total - prev_total;
        unsigned long long proc_diff  = curr_proc - prev_proc;

        double cpu_percent = 0.0;
        if (total_diff > 0)
            cpu_percent = (double)proc_diff / total_diff * 100.0;

        long mem_kb = get_memory_usage_kb();
        long pss_kb = get_pss_kb();
        time_t now = time(NULL);
        fprintf(log, "%ld %.2f %ld %ld\n",
        now - start_time,
        cpu_percent,
        mem_kb,
        pss_kb);

        fflush(log);

        prev_total = curr_total;
        prev_proc  = curr_proc;
    }

    fclose(log);
    return NULL;
}

/* ===== Start Monitoring ===== */

void start_monitoring(const char *log_file) {

    strncpy(log_filename, log_file, sizeof(log_filename) - 1);
    log_filename[sizeof(log_filename) - 1] = '\0';

    pthread_t tid;
    if (pthread_create(&tid, NULL, monitor_thread, NULL) != 0) {
        perror("Failed to create monitor thread");
        return;
    }

    pthread_detach(tid);
}
