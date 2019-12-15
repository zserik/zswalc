#include "app.hpp"

#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include <atomic>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
using namespace std;

static void handle_error(cgicc::FCgiIO &IO, const char *msg) {
  IO << "Content-Type: text/html\r\n\r\n"
        "<!doctype html>\n<html>\n"
        "<head><title>ERROR occured in chat app</title></head>\n"
        "<body>\n"
        "  <h1>ERROR in chat app</h1>\n"
        "  <p style=\"color: red;\">Error: " << msg << "</p>\n"
        "</body>\n</html>\n";
}

static atomic<bool> b_do_shutdown;

typedef void (*sighandler_t)(int);
static void my_signal(const int nr, const sighandler_t handler) noexcept {
  struct sigaction sig;
  sig.sa_handler = handler;
  sig.sa_flags   = 0;
  sigemptyset(&sig.sa_mask);
  sigaction(nr, &sig, 0);
}

static void shutdown_handler(int) noexcept {
  b_do_shutdown = true;
  my_signal(SIGINT, SIG_IGN);
  my_signal(SIGTERM, SIG_DFL);
  my_signal(SIGUSR1, SIG_IGN);
  close(0);
}

static void* worker(void *) {
  FCGX_Request request;
  FCGX_InitRequest(&request, 0, 0);
  my_signal(SIGTERM, shutdown_handler);
  my_signal(SIGUSR1, shutdown_handler);

  // check shutdown flag before and after the accept call
  while(!b_do_shutdown && !FCGX_Accept_r(&request) && !b_do_shutdown) {
    try {
      FCGX_SetExitStatus(0, request.out);
      cgicc::FCgiIO IO(request);
      try {
        handle_request(IO);
      } catch(const exception &e) {
        handle_error(IO, e.what());
      } catch(const char *e) {
        handle_error(IO, e);
      }
    } catch(...) {
      FCGX_SetExitStatus(1, request.out);
    }
  }

  FCGX_Finish_r(&request);
  FCGX_Free(&request, false);

  return nullptr;
}

int main(void) {
  FCGX_Init();

  // 1. prevent periodic stat(/etc/localtime)
  {
    ifstream tzf("/etc/timezone");
    string tmp;
    if(tzf) tzf >> tmp;
    if(!tmp.empty()) {
      tmp = ':' + tmp;
      setenv("TZ", tmp.c_str(), 0);
    }
  }
  tzset();

  // 2. make shutdown a bit more graceful
  my_signal(SIGINT, shutdown_handler);
  my_signal(SIGTERM, shutdown_handler);
  my_signal(SIGUSR1, shutdown_handler);

  // 3. spawn workers
  b_do_shutdown = false;
  vector<pthread_t> workers;
  {
    const size_t imax = thread::hardware_concurrency();
    workers.reserve(imax);
    for(size_t i = 0; i < imax; ++i) {
      pthread_t w_tid;
      if(pthread_create(&w_tid, nullptr, worker, nullptr))
        return 1;
      workers.emplace_back(w_tid);
    }
  }

  worker(nullptr);

  // 4. cleanup
  b_do_shutdown = true;
  for(const auto &i : workers) {
    pthread_kill(i, SIGUSR1);
    pthread_join(i, nullptr);
  }

  return 0;
}