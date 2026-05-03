#include "config_parser.h"
#include "capture.h"

#include <cstdio>
#include <csignal>
#include <syslog.h>
#include <unistd.h>
#include <string>

static Capture* g_capture = nullptr;

static void signal_handler(int /*signal*/) {
    if (g_capture) {
        g_capture->stop();
    }
}

static void usage(const char* program) {
    std::fprintf(stderr,
        "Usage: %s -o <output_file> [-c config.yaml]\n"
        "\n"
        "Options:\n"
        "  -o  output file path  (required)\n"
        "  -c  config file path  (default: config/config.yaml)\n"
        "  -h  show this help\n",
        program);
}

int main(int argc, char** argv) {

    const char* config_path = "config/config.yaml";
    const char* output_path = nullptr;

    int opt;
    while ((opt = getopt(argc, argv, "c:o:h")) != -1) {
        switch (opt) {
            case 'c':
                config_path = optarg;
                break;
            case 'o':
                output_path = optarg;
                break;
            case 'h':
                usage(argv[0]);
                return 0;
            default:
                usage(argv[0]);
                return 1;
        }
    }

    if (output_path == nullptr) {
        std::fprintf(stderr, "error: -o <output_file> is required\n");
        usage(argv[0]);
        return 1;
    }

    openlog("bmd_capture", LOG_PID | LOG_NDELAY, LOG_USER);

    if (!load_config(config_path)) {
        closelog();
        return 1;
    }

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    Capture capture;
    g_capture = &capture;

    syslog(LOG_INFO, "started");

    if (!capture.start(config(), output_path)) {
        closelog();
        return 1;
    }

    closelog();
    return 0;
}
