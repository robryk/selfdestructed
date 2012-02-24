#include <cstdio>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>

#include <string>
#include <sstream>

using std::string;
using std::stringstream;

volatile int exit_flag;

void sig_handler(int) {
	exit_flag = 1;
}

string get_nodename()
{
	utsname u;
	if (uname(&u) < 0)
		return "";
	return string(u.nodename);
}

int fatal_sigs[] = { SIGHUP, SIGINT, SIGQUIT, SIGPIPE, SIGTERM, SIGUSR1, SIGUSR2, SIGPWR };

int main(int argc, char** argv)
{
	char arg;
	string dir = "~/.selfdestruct/", fprefix = get_nodename(), fsuffix = "unknown";
	while ((arg = getopt(argc, argv, "hd:t:")) != -1) {
		if (arg == 'd') {
			dir = string(optarg);
		} else if (arg == 't') {
			fsuffix = string(optarg);
		} else {
			printf("Usage: %s [-h] [-d directory] [-t session-type]\n");
			return 0;
		}
	}
	chdir(dir.c_str());
	for(int i=0;i<sizeof(fatal_sigs)/sizeof(fatal_sigs[0]);i++)
		if (signal(fatal_sigs[i], &sig_handler) == SIG_ERR)
			assert(0);
	FILE* file = NULL;
	string file_name;
	for(int cnt=0;file==NULL;cnt++) {
		std::stringstream str;
		str << fprefix << "." << fsuffix << "." << cnt;
		file_name = str.str();
		errno = 0;
		file = fopen(file_name.c_str(), "w+x");
		assert(errno == EEXIST || file);
	}
	setbuf(file, NULL);
	setenv("CMDFILE", file_name.c_str(), 1);
	while (exit_flag == 0) {
		sleep(1);
		struct timeval tvs[2];
		int fd = fileno(file);
		if (gettimeofday(&tvs[0], NULL) < 0)
			assert(0);
		tvs[1] = tvs[0];
		if (futimes(fd, tvs) < 0)
			assert(0);
		rewind(file);
		char buf[1024];
		if (fgets(buf, sizeof(buf), file) != NULL) {
			if (buf[strlen(buf)-1] != '\n')
				continue;
			system(buf);
			if (ftruncate(fd, 0) < 0)
				assert(0);
		}
	}
	unlink(file_name.c_str());
	return 0;
}
	
