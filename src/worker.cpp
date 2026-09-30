#include "worker.h"

#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <cstdio>

volatile sig_atomic_t stop_requested = 0;

void handle_sigterm(int signal_number) {
    stop_requested = 1;
}

void run_worker(int worker_id) {
    struct sigaction ignore_sa{};
    ignore_sa.sa_handler = SIG_IGN;
    sigemptyset(&ignore_sa.sa_mask);
    ignore_sa.sa_flags = 0;

    if (sigaction(SIGINT, &ignore_sa, nullptr) == -1) {
        perror("sigaction SIGINT");
        _exit(1);
    }

    struct sigaction term_sa{};

    term_sa.sa_handler = handle_sigterm;
    sigemptyset(&term_sa.sa_mask);
    term_sa.sa_flags = 0;

    if (sigaction(SIGTERM, &term_sa, nullptr) == -1) {
        perror("sigaction SIGTERM");
        _exit(1);
    }

    std::cout << "[Worker " << worker_id
              << "] PID: " << getpid()
              << ", PPID: " << getppid()
              << std::endl;

    std::cout << "[Worker " << worker_id
              << "] waiting for work..."
              << std::endl;

    while (!stop_requested) {
        sleep(1);
    }

    std::cout << "[Worker " << worker_id
              << "] graceful shutdown"
              << std::endl;
}
