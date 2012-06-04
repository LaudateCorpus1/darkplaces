
#ifdef WIN32
#include <windows.h>
#include <mmsystem.h>
#include <io.h>
#include "conio.h"
#else
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#endif

#include <signal.h>

#include "quakedef.h"

// =======================================================================
// General routines
// =======================================================================
void Sys_Shutdown (void)
{
#ifdef FNDELAY
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
#endif
	fflush(stdout);
}

void Sys_Error (const char *error, ...)
{
	va_list argptr;
	char string[MAX_INPUTLINE];

// change stdin to non blocking
#ifdef FNDELAY
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
#endif

	va_start (argptr,error);
	dpvsnprintf (string, sizeof (string), error, argptr);
	va_end (argptr);

	Con_Printf ("Quake Error: %s\n", string);

	Host_Shutdown ();
	exit (1);
}

static int outfd = 1;
void Sys_PrintToTerminal(const char *text)
{
	if(outfd < 0)
		return;
#ifdef FNDELAY
	// BUG: for some reason, NDELAY also affects stdout (1) when used on stdin (0).
	// this is because both go to /dev/tty by default!
	{
		int origflags = fcntl (outfd, F_GETFL, 0);
		fcntl (outfd, F_SETFL, origflags & ~FNDELAY);
#endif
#ifdef WIN32
#define write _write
#endif
		while(*text)
		{
			fs_offset_t written = (fs_offset_t)write(outfd, text, strlen(text));
			if(written <= 0)
				break; // sorry, I cannot do anything about this error - without an output
			text += written;
		}
#ifdef FNDELAY
		fcntl (outfd, F_SETFL, origflags);
	}
#endif
	//fprintf(stdout, "%s", text);
}

char *Sys_ConsoleInput(void)
{
	//if (cls.state == ca_dedicated)
	{
		static char text[MAX_INPUTLINE];
		static unsigned int len = 0;
#ifdef WIN32
		int c;

		// read a line out
		while (_kbhit ())
		{
			c = _getch ();
			if (c == '\r')
			{
				text[len] = '\0';
				_putch ('\n');
				len = 0;
				return text;
			}
			if (c == '\b')
			{
				if (len)
				{
					_putch (c);
					_putch (' ');
					_putch (c);
					len--;
				}
				continue;
			}
			if (len < sizeof (text) - 1)
			{
				_putch (c);
				text[len] = c;
				len++;
			}
		}
#else
		fd_set fdset;
		struct timeval timeout;
		FD_ZERO(&fdset);
		FD_SET(0, &fdset); // stdin
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		if (select (1, &fdset, NULL, NULL, &timeout) != -1 && FD_ISSET(0, &fdset))
		{
			len = read (0, text, sizeof(text) - 1);
			if (len >= 1)
			{
				// rip off the \n and terminate
				// div0: WHY? console code can deal with \n just fine
				// this caused problems with pasting stuff into a terminal window
				// so, not ripping off the \n, but STILL keeping a NUL terminator
				text[len] = 0;
				return text;
			}
		}
#endif
	}
	return NULL;
}

char *Sys_GetClipboardData (void)
{
	return NULL;
}

void Sys_InitConsole (void)
{
}

#ifdef ANTICHEAT
# ifndef WIN32
#  include <sys/ptrace.h>
#  include <sys/wait.h>
#  include <errno.h>
# endif
#endif
static void anticheat_init(void)
{
#ifdef ANTICHEAT
#define FAIL exit(42)

	// anti ptrace; also, make a forked process copy to detach from debuggers
# ifndef WIN32
	pid_t pid = fork();
	if(pid < 0)
		FAIL;
	if(pid == 0)
	{
		// nothing to do here
	}
	else
	{
		// parent
		int status;
		if(ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0)
		{
			kill(pid, SIGKILL);
			FAIL;
		}
		for(;;)
		{
			if(waitpid(pid, &status, 0) == (pid_t) -1)
			{
				if(errno == ECHILD) // process no longer exists
					FAIL;
			}
			if(WIFEXITED(status))
			{
				exit(WEXITSTATUS(status));
			}
			if(WIFSTOPPED(status))
			{
				printf("ptrace: continue... (signal: %d)\n", (int) WSTOPSIG(status));
				if(ptrace(PTRACE_CONT, pid, (void *) WSTOPSIG(status), NULL) < 0)
				{
					perror("okay.png");
				}
			}
		}
		// never gonna get to here
	}
# endif

#endif
}

int main (int argc, char **argv)
{
	anticheat_init();

	signal(SIGFPE, SIG_IGN);

	com_argc = argc;
	com_argv = (const char **)argv;
	Sys_ProvideSelfFD();

	// COMMANDLINEOPTION: sdl: -noterminal disables console output on stdout
	if(COM_CheckParm("-noterminal"))
		outfd = -1;
	// COMMANDLINEOPTION: sdl: -stderr moves console output to stderr
	else if(COM_CheckParm("-stderr"))
		outfd = 2;
	else
		outfd = 1;

#ifdef FNDELAY
	fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);
#endif

	Host_Main();

	return 0;
}

qboolean sys_supportsdlgetticks = false;
unsigned int Sys_SDL_GetTicks (void)
{
	Sys_Error("Called Sys_SDL_GetTicks on non-SDL target");
	return 0;
}
void Sys_SDL_Delay (unsigned int milliseconds)
{
	Sys_Error("Called Sys_SDL_Delay on non-SDL target");
}
