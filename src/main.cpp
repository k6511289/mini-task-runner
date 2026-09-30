#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <signal.h>
#include <cerrno>
#include <cstring>
#include "worker.h"

volatile sig_atomic_t shutdown_requested = 0;

void handle_shutdown_signal(int signal_number) {
    shutdown_requested = 1;
}

void terminate_workers(const std::vector<pid_t>& worker_pids) {
    for (pid_t pid : worker_pids) {
        std::cout << "[Master] Sending SIGTERM to Worker PID "
                  << pid << '\n';

        if (kill(pid, SIGTERM) == -1) {
            perror("kill");
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0]
                  << " <worker_count>\n";
        return 1;
    }

    int worker_count;

    try {
        worker_count = std::stoi(argv[1]);
    } catch (...) {
        std::cerr << "Error: worker_count must be an integer\n";
        return 1;
    }

    if (worker_count <= 0) {
        std::cerr << "Error: worker_count must be greater than 0\n";
        return 1;
    }

    std::cout << "[Master] Starting with "
              << worker_count << " workers\n";

    struct sigaction sa{};

    sa.sa_handler = handle_shutdown_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, nullptr) == -1) {
        perror("sigaction");
        return 1;
    }

    if (sigaction(SIGTERM, &sa, nullptr) == -1) {
        perror("sigaction");
        return 1;
    }

    std::vector<pid_t> worker_pids;

    for (int i = 0; i < worker_count; ++i) {
        pid_t pid = fork();

        if (pid < 0) {
            std::cerr << "fork failed\n";
            return 1;
        }

        if (pid == 0) {
            run_worker(i);
	    return 0;
        }

        std::cout << "[Master] Created Worker "
                  << i << ", PID: " << pid << '\n';
	worker_pids.push_back(pid);
    }

    bool all_success = true;
    int remaining_workers = worker_count;

	while (remaining_workers > 0) {
	    int status;

	    pid_t finished_pid = waitpid(-1, &status, 0);

	   if (finished_pid < 0) {

	        if (errno == EINTR) {
		        std::cout
		            << "[Master] waitpid interrupted by signal\n";

		        if (shutdown_requested) {
		            std::cout
		                << "[Master] Shutdown requested\n";

		            terminate_workers(worker_pids);

		            shutdown_requested = 0;
		        }

		        continue;
		    }

	        std::cerr
	            << "[Master] waitpid failed: "
	            << std::strerror(errno)
	            << '\n';

	       all_success = false;
	       break;
	   }

	    if (WIFEXITED(status)) {
	        int exit_code = WEXITSTATUS(status);

	        std::cout << "[Master] Worker PID "
	                  << finished_pid
	                  << " exited with code "
	                  << exit_code
	                  << '\n';

	        if (exit_code != 0) {
	            all_success = false;
	        }
	    } else if (WIFSIGNALED(status)) {
	        int signal_number = WTERMSIG(status);

	        std::cout << "[Master] Worker PID "
	                  << finished_pid
	                  << " terminated by signal "
	                  << signal_number
	                  << '\n';

	        all_success = false;
	    }
	    remaining_workers--;
	}

	std::cout << "[Master] All workers finished\n";

	return all_success ? 0 : 1;
}
