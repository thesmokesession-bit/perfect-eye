/* Start-up of Perfect Dark (Dab's Mod) as a PS5 title: the log file, the crash handler,
 * SDL's video driver, the splash screen and the arguments a title has no command line for.
 * Called first thing in main() (port/src/main.c, PLATFORM_PS5). */
#include <SDL.h>

#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <ucontext.h>
#include <unistd.h>

int main(int argc, const char **argv);
int sceSystemServiceHideSplashScreen(void);

#define LOG_PATH "/app0/pd.log"
#define LOG_BUFFER_SIZE (64 * 1024)

static int log_fd = -1;
static char stdout_buffer[LOG_BUFFER_SIZE];
static char stderr_buffer[LOG_BUFFER_SIZE];

/* Many small writes to the title folder are slow on the console (each one stalls), so the
 * log is fully buffered and written out every few seconds instead of per line. The game's
 * own --log opens and closes its file for every line, so it is not used here. */
static void *log_flusher(void *arg)
{
	(void)arg;
	for (;;) {
		sleep(3);
		fflush(stderr);
		fflush(stdout);
	}
	return NULL;
}

static void open_log(void)
{
	FILE *truncate = fopen(LOG_PATH, "w");
	if (truncate) {
		fclose(truncate);
	}
	if (!freopen(LOG_PATH, "a", stdout)) {
		return;
	}
	setvbuf(stdout, stdout_buffer, _IOFBF, sizeof(stdout_buffer));
	log_fd = fileno(stdout);
	if (freopen(LOG_PATH, "a", stderr)) {
		setvbuf(stderr, stderr_buffer, _IOFBF, sizeof(stderr_buffer));
	}
	pthread_t thread;
	if (pthread_create(&thread, NULL, log_flusher, NULL) == 0) {
		pthread_detach(thread);
	}
}

/* Crash output goes straight to the log's descriptor with write(): stdio may hold locks. */
static void raw_printf(const char *format, ...)
{
	char line[256];
	va_list args;
	va_start(args, format);
	int len = vsnprintf(line, sizeof(line), format, args);
	va_end(args);
	if (log_fd >= 0 && len > 0) {
		(void)write(log_fd, line, (size_t)len < sizeof(line) ? (size_t)len : sizeof(line) - 1);
	}
}

static volatile sig_atomic_t crash_reported;

/* The faulting instruction and the code addresses found on the stack above it, with main's
 * address so they can be matched to the unsigned ELF (eboot.bin.elf) with llvm-symbolizer:
 * address - main_at_runtime + main_in_elf. Only the first crashing thread reports. */
static void on_signal(int sig, siginfo_t *info, void *context)
{
	if (crash_reported) {
		for (;;) {
			sleep(1);
		}
	}
	crash_reported = 1;
	fflush(stdout);
	fflush(stderr);
	const mcontext_t *mc = &((ucontext_t *)context)->uc_mcontext;
	raw_printf("FATAL: signal %d at address %p\n", sig, info ? info->si_addr : NULL);
	raw_printf("  rip %p rsp %p rbp %p (main at %p)\n", (void *)mc->mc_rip, (void *)mc->mc_rsp,
		(void *)mc->mc_rbp, (void *)&main);
	const uintptr_t code_start = (uintptr_t)&main & ~(uintptr_t)0xffffff;
	const uintptr_t code_end = code_start + 0x10000000;
	const uintptr_t *stack = (const uintptr_t *)mc->mc_rsp;
	int found = 0;
	for (int i = 0; i < 2048 && found < 40; i++) {
		if (stack[i] >= code_start && stack[i] < code_end) {
			raw_printf("  stack[%d] %p\n", i, (void *)stack[i]);
			found++;
		}
	}
	_Exit(128 + sig);
}

/* ps5_rom.c: the player's ROMs put where the game looks for them */
void ps5ImportRoms(void);

void ps5PlatformInit(int *argc, const char ***argv)
{
	open_log();

	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_sigaction = on_signal;
	action.sa_flags = SA_SIGINFO;
	sigemptyset(&action.sa_mask);
	const int sigs[] = { SIGABRT, SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGSYS };
	for (size_t i = 0; i < sizeof(sigs) / sizeof(sigs[0]); i++) {
		sigaction(sigs[i], &action, NULL);
	}
	/* A peer that goes away mid-send must not end the game: online play, the lobby, curl */
	signal(SIGPIPE, SIG_IGN);

	/* There is no video driver for the console's display: the window is an offscreen window,
	 * and the Vulkan renderer presents through VK_KHR_display (gfx_vulkan.cpp, __PROSPERO__). */
	SDL_SetHint(SDL_HINT_VIDEODRIVER, "offscreen");
	/* The system's splash screen stays over the title's output until the title hides it. */
	sceSystemServiceHideSplashScreen();

	ps5ImportRoms();

	/* Online play (ENet) runs on one dual-stack IPv6 UDP socket. Whether the console's
	 * network stack gives one is the first thing to know when online play fails. */
	{
		int s = socket(PF_INET6, SOCK_DGRAM, 0);
		int off = 0;
		int v6only = s >= 0 ? setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(off)) : -1;
		printf("PS5: dual-stack UDP socket: %s, IPV6_V6ONLY off: %s\n", s >= 0 ? "yes" : "NO",
			v6only == 0 ? "yes" : "NO");
		if (s >= 0) {
			close(s);
		}
	}

	/* The arguments a title cannot be given: everything in the title folder, Vulkan only
	 * (there is no OpenGL to fall back to), then whatever the launcher passed. */
	static const char *defaults[] = { "--portable", "--vulkan" };
	const int ndefaults = (int)(sizeof(defaults) / sizeof(defaults[0]));
	const int n = *argc > 0 ? *argc : 1;
	const char **args = calloc((size_t)(n + ndefaults + 1), sizeof(*args));
	if (!args) {
		return;
	}
	args[0] = (*argc > 0 && (*argv)[0]) ? (*argv)[0] : "/app0/eboot.bin";
	for (int i = 0; i < ndefaults; i++) {
		args[1 + i] = defaults[i];
	}
	for (int i = 1; i < *argc; i++) {
		args[ndefaults + i] = (*argv)[i];
	}
	*argc = n + ndefaults;
	*argv = args;

	printf("PS5: Perfect Dark (Dab's Mod) starting, title folder /app0\n");
}
