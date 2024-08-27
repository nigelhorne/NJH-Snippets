#include <stdio.h>
#include <signal.h>
#include <unistd.h>

static	void	stop(int signum);
static	void	start(int signum);
static	pid_t	pid;

int
main()
{
	signal(SIGTSTP, stop);
	pid = getpid();
	
	for(;;) {
		puts("main");
		pause();
	}

	return 0;
}

static void
stop(int signum)
{
	puts("stop");
	signal(SIGCONT, start);
	signal(SIGTSTP, SIG_DFL);
	kill(pid, SIGTSTP);
}

static void
start(int signum)
{
	puts("start");
	signal(SIGCONT, SIG_DFL);
	signal(SIGTSTP, stop);
}
