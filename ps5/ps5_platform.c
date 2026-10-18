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
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/filio.h>
#include <time.h>
#include <ucontext.h>
#include <unistd.h>

int main(int argc, const char **argv);
int sceKernelAvailableFlexibleMemorySize(size_t *size);
int sceKernelAvailableDirectMemorySize(off_t start, off_t end, size_t align, off_t *start_out, size_t *size_out);
size_t sceKernelGetDirectMemorySize(void);

int sceSystemServiceHideSplashScreen(void);

/* A system notification (the toast at the top right) and the same line in the log: what the
 * player sees of a slow first start or of a game that cannot start, before there is a picture */
typedef struct {
	char useless[45];
	char message[3075];
} notify_request_t;
int sceKernelSendNotificationRequest(int device, notify_request_t *req, size_t size, int blocking);

void ps5Notify(const char *fmt, ...)
{
	static notify_request_t req;
	va_list args;
	memset(&req, 0, sizeof(req));
	va_start(args, fmt);
	vsnprintf(req.message, sizeof(req.message), fmt, args);
	va_end(args);
	printf("PS5: notification: %s\n", req.message);
	fflush(stdout);
	sceKernelSendNotificationRequest(0, &req, sizeof(req), 0);
}

/* The system's splash screen (the title's own pic) stays up through the load and comes down
 * with the first picture the game presents (gfx_vulkan.cpp), rather than leaving a black
 * screen that looks like nothing is happening. */
void ps5FirstFrame(void)
{
	static int done;
	if (!done) {
		done = 1;
		sceSystemServiceHideSplashScreen();
		printf("PS5: first frame, splash screen down\n");
	}
}

#define LOG_PATH "/app0/pd.log"
#define LOG_BUFFER_SIZE (64 * 1024)

static int log_fd = -1;
static char stdout_buffer[LOG_BUFFER_SIZE];
static char stderr_buffer[LOG_BUFFER_SIZE];

/* Many small writes to the title folder are slow on the console (each one stalls), so the
 * log is fully buffered and written out every few seconds instead of per line. The game's
 * own --log opens and closes its file for every line, so it is not used here. */
static void nap(unsigned ms)
{
	struct timespec ts = { ms / 1000, (long)(ms % 1000) * 1000000L };
	while (nanosleep(&ts, &ts) != 0) {
	}
}

static void *log_flusher(void *arg)
{
	(void)arg;
	for (;;) {
		nap(3000);
		fflush(stderr);
		fflush(stdout);
	}
	return NULL;
}

static int log_pipe_in = -1, log_file = -1;

static void *log_writer(void *arg)
{
	static char chunk[16384];
	(void)arg;
	for (;;) {
		const ssize_t n = read(log_pipe_in, chunk, sizeof(chunk));
		if (n > 0) {
			(void)write(log_file, chunk, (size_t)n);
		} else if (n == 0) {
			return NULL;
		}
	}
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
	int piped = 0;
	/* stdout and stderr then go into a pipe and a thread of their own writes the pipe to the
	 * file: a flush held stdio's lock through the slow write, and the game's thread logging a
	 * line meanwhile waited for it (stutters, 2026-10-18). Without a pipe, as it was. */
	{
		int fds[2];
		const int file = open(LOG_PATH, O_WRONLY | O_APPEND);
		pthread_t writer;
		if (file >= 0 && pipe(fds) == 0) {
			log_pipe_in = fds[0];
			log_file = file;
			if (pthread_create(&writer, NULL, log_writer, NULL) == 0) {
				pthread_detach(writer);
				fflush(stdout);
				fflush(stderr);
				dup2(fds[1], fileno(stdout));
				dup2(fds[1], fileno(stderr));
				close(fds[1]);
				log_fd = file;
				piped = 1;
			}
		}
	}
	/* With the pipe, a line is cheap to write (the writer thread takes the slow part), so each
	 * line goes as it is printed and a sudden end loses none; the flusher stays for the rest */
	if (piped) {
		setvbuf(stdout, stdout_buffer, _IOLBF, sizeof(stdout_buffer));
		setvbuf(stderr, stderr_buffer, _IOLBF, sizeof(stderr_buffer));
	}
	pthread_t thread;
	if (pthread_create(&thread, NULL, log_flusher, NULL) == 0) {
		pthread_detach(thread);
	}
}

/* The watchdog's own file, opened for every line: it must not depend on stdio or on the log's
 * descriptor, which are what a stall might be stuck in */
#define WATCH_PATH "/app0/pd-watch.log"
static void watch_printf(const char *format, ...)
{
	char line[256];
	va_list args;
	va_start(args, format);
	int len = vsnprintf(line, sizeof(line), format, args);
	va_end(args);
	int fd = open(WATCH_PATH, O_WRONLY | O_APPEND | O_CREAT, 0666);
	if (fd >= 0 && len > 0) {
		(void)write(fd, line, (size_t)len < sizeof(line) ? (size_t)len : sizeof(line) - 1);
	}
	if (fd >= 0) {
		close(fd);
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

int pthread_attr_get_np(pthread_t thread, pthread_attr_t *attr);

/* The end of this thread's stack, so a scan of it stops there: the sampler read past a render
 * thread's and faulted (2026-10-18). Without the thread's attributes, the end of rsp's page. */
static uintptr_t stack_end(uintptr_t rsp)
{
	pthread_attr_t attr;
	void *addr = NULL;
	size_t size = 0;
	uintptr_t end = (rsp + 0x1000) & ~(uintptr_t)0xfff;
	if (pthread_attr_init(&attr) == 0) {
		if (pthread_attr_get_np(pthread_self(), &attr) == 0 && pthread_attr_getstack(&attr, &addr, &size) == 0
				&& (uintptr_t)addr < rsp && rsp < (uintptr_t)addr + size) {
			end = (uintptr_t)addr + size;
		}
		pthread_attr_destroy(&attr);
	}
	return end;
}

/* The faulting instruction and the code addresses found on the stack above it, with main's
 * address so they can be matched to the unsigned ELF (eboot.bin.elf) with llvm-symbolizer:
 * address - main_at_runtime + main_in_elf. Only the first crashing thread reports. */
static void on_signal(int sig, siginfo_t *info, void *context)
{
	if (crash_reported) {
		for (;;) {
			nap(1000);
		}
	}
	crash_reported = 1;
	/* The report first, through its own file: stdio's locks may be held by the thread that
	 * crashed or by another, and fflush() waiting on one hung here before (2026-10-18). */
	const mcontext_t *mc = &((ucontext_t *)context)->uc_mcontext;
	const uintptr_t code_start = (uintptr_t)&main & ~(uintptr_t)0x3fffff;
	const uintptr_t code_end = code_start + 0x2000000;
	for (int pass = 0; pass < 2; pass++) {
		void (*out)(const char *, ...) = pass == 0 ? watch_printf : raw_printf;
		out("FATAL: signal %d (code %d) at address %p\n", sig, info ? info->si_code : 0,
			info ? info->si_addr : NULL);
		out("  rip %p rsp %p rbp %p (main at %p)\n", (void *)mc->mc_rip, (void *)mc->mc_rsp,
			(void *)mc->mc_rbp, (void *)&main);
		const uintptr_t *stack = (const uintptr_t *)mc->mc_rsp;
		const uintptr_t *top = (const uintptr_t *)stack_end(mc->mc_rsp);
		int found = 0;
		for (int i = 0; i < 4096 && found < 48 && stack + i < top; i++) {
			if (stack[i] >= code_start && stack[i] < code_end) {
				out("  stack[%d] %p\n", i, (void *)stack[i]);
				found++;
			}
		}
	}
	{
		static notify_request_t req;
		snprintf(req.message, sizeof(req.message), "Perfect Dark crashed (signal %d). pd.log in its folder says where.", sig);
		sceKernelSendNotificationRequest(0, &req, sizeof(req), 0);
	}
	/* Then the log's last few seconds, if no other thread is in the middle of writing it */
	if (ftrylockfile(stdout) == 0) {
		fflush(stdout);
		funlockfile(stdout);
	}
	_Exit(128 + sig);
}

/* The watchdog for the first console runs: a line every two seconds saying how many frames the
 * game has started and presented, and when they stop coming, where the game's thread and the
 * renderer's thread are (the code addresses on their stacks, read as for a crash). */
volatile unsigned ps5_frames_started, ps5_presents;
pthread_t ps5_worker_thread;
volatile int ps5_worker_known;
static pthread_t main_thread;

static void on_sample(int sig, siginfo_t *info, void *context)
{
	(void)sig;
	(void)info;
	const mcontext_t *mc = &((ucontext_t *)context)->uc_mcontext;
	const int is_main = pthread_equal(pthread_self(), main_thread);
	watch_printf("stall: %s thread at rip %p (main at %p)\n", is_main ? "game" : "render",
		(void *)mc->mc_rip, (void *)&main);
	const uintptr_t code_start = (uintptr_t)&main & ~(uintptr_t)0x3fffff;
	const uintptr_t code_end = code_start + 0x2000000;
	const uintptr_t *stack = (const uintptr_t *)mc->mc_rsp;
	const uintptr_t *top = (const uintptr_t *)stack_end(mc->mc_rsp);
	int found = 0;
	for (int i = 0; i < 4096 && found < 48 && stack + i < top; i++) {
		if (stack[i] >= code_start && stack[i] < code_end) {
			watch_printf("  %s stack[%d] %p\n", is_main ? "game" : "render", i, (void *)stack[i]);
			found++;
		}
	}
}

static void *watchdog(void *arg)
{
	(void)arg;
	unsigned last = 0, still = 0, samples = 0;
	watch_printf("watchdog up\n");
	for (int t = 2;; t += 2) {
		nap(2000);
		/* a crash handler stuck after its report (the notification or the log's flush): end it */
		if (crash_reported) {
			nap(2000);
			_Exit(139);
		}
		const unsigned frames = ps5_frames_started;
		{
			/* memory, for a game the system ends without a signal (out of it, perhaps) */
			size_t flex = 0, dsize = 0;
			off_t dstart = 0;
			sceKernelAvailableFlexibleMemorySize(&flex);
			sceKernelAvailableDirectMemorySize(0, (off_t)sceKernelGetDirectMemorySize(), 0, &dstart, &dsize);
			watch_printf("%ds: %u frames started, %u presented; free flexible %zu MB, direct %zu MB\n", t, frames,
				ps5_presents, flex >> 20, dsize >> 20);
		}
		still = (frames == last) ? still + 1 : 0;
		last = frames;
		if (frames > 0 && still >= 10 && samples < 3) {
			samples++;
			pthread_kill(main_thread, SIGUSR1);
			if (ps5_worker_known) {
				nap(1000);
				pthread_kill(ps5_worker_thread, SIGUSR1);
			}
		}
	}
	return NULL;
}

int sceKernelLoadStartModule(const char *path, size_t args, const void *argp, uint32_t flags, const void *opt, int *res);
int sceKernelDlsym(int module, const char *name, void **addr);

/* The console's system libraries the title imports from: an import the console left unresolved is
 * a call to address 0 (libSceKeyboard's, 2026-10-18), so the watchdog's file lists any such at start */
extern void sceAgcDriverSetHsOffchipParam(void);
extern void sceAgcDriverSetTFRing(void);
extern void sceAgcDriverSubmitDcb(void);
extern void sceAgcInit(void);
extern void sceAgcSuspendPoint(void);
extern void sceAudioOutClose(void);
extern void sceAudioOutInit(void);
extern void sceAudioOutOpen(void);
extern void sceAudioOutOutput(void);
extern void sceImeDialogAbort(void);
extern void sceImeDialogGetResult(void);
extern void sceImeDialogGetStatus(void);
extern void sceImeDialogInit(void);
extern void sceImeDialogTerm(void);
extern void sceKernelAllocateDirectMemory(void);
extern void sceKernelAllocateMainDirectMemory(void);
extern void sceKernelCreateEqueue(void);
extern void sceKernelDeleteEqueue(void);
extern void sceKernelMapDirectMemory(void);
extern void sceKernelMprotect(void);
extern void sceKernelMunmap(void);
extern void sceKernelReadTsc(void);
extern void sceKernelReleaseDirectMemory(void);
extern void sceKernelReserveVirtualRange(void);
extern void sceKernelWaitEqueue(void);
extern void sceKeyboardClose(void);
extern void sceKeyboardInit(void);
extern void sceKeyboardOpen(void);
extern void sceKeyboardReadState(void);
extern void scePadClose(void);
extern void scePadGetHandle(void);
extern void scePadInit(void);
extern void scePadOpen(void);
extern void scePadReadState(void);
extern void scePadSetLightBar(void);
extern void scePadSetMotionSensorState(void);
extern void scePadSetTriggerEffect(void);
extern void scePadSetVibration(void);
extern void scePadSetVibrationMode(void);
extern void sceSystemServiceLaunchWebBrowser(void);
extern void sceUserServiceGetForegroundUser(void);
extern void sceUserServiceGetLoginUserIdList(void);
extern void sceUserServiceGetUserName(void);
extern void sceUserServiceInitialize(void);
extern void sceVideoOutAddFlipEvent(void);
extern void sceVideoOutClose(void);
extern void sceVideoOutConfigureOutput(void);
extern void sceVideoOutDeleteFlipEvent(void);
extern void sceVideoOutGetFlipStatus(void);
extern void sceVideoOutIsOutputSupported(void);
extern void sceVideoOutOpen(void);
extern void sceVideoOutRegisterBuffers2(void);
extern void sceVideoOutSetBufferAttribute2(void);
extern void sceVideoOutSetFlipRate(void);
extern void sceVideoOutSubmitFlip(void);
extern void sceVideoOutWaitVblank(void);
static const struct { const char *name; void (*fn)(void); } ps5_imports[] = {
	{ "sceAgcDriverSetHsOffchipParam", (void (*)(void))sceAgcDriverSetHsOffchipParam },
	{ "sceAgcDriverSetTFRing", (void (*)(void))sceAgcDriverSetTFRing },
	{ "sceAgcDriverSubmitDcb", (void (*)(void))sceAgcDriverSubmitDcb },
	{ "sceAgcInit", (void (*)(void))sceAgcInit },
	{ "sceAgcSuspendPoint", (void (*)(void))sceAgcSuspendPoint },
	{ "sceAudioOutClose", (void (*)(void))sceAudioOutClose },
	{ "sceAudioOutInit", (void (*)(void))sceAudioOutInit },
	{ "sceAudioOutOpen", (void (*)(void))sceAudioOutOpen },
	{ "sceAudioOutOutput", (void (*)(void))sceAudioOutOutput },
	{ "sceImeDialogAbort", (void (*)(void))sceImeDialogAbort },
	{ "sceImeDialogGetResult", (void (*)(void))sceImeDialogGetResult },
	{ "sceImeDialogGetStatus", (void (*)(void))sceImeDialogGetStatus },
	{ "sceImeDialogInit", (void (*)(void))sceImeDialogInit },
	{ "sceImeDialogTerm", (void (*)(void))sceImeDialogTerm },
	{ "sceKernelAllocateDirectMemory", (void (*)(void))sceKernelAllocateDirectMemory },
	{ "sceKernelAllocateMainDirectMemory", (void (*)(void))sceKernelAllocateMainDirectMemory },
	{ "sceKernelAvailableDirectMemorySize", (void (*)(void))sceKernelAvailableDirectMemorySize },
	{ "sceKernelCreateEqueue", (void (*)(void))sceKernelCreateEqueue },
	{ "sceKernelDeleteEqueue", (void (*)(void))sceKernelDeleteEqueue },
	{ "sceKernelDlsym", (void (*)(void))sceKernelDlsym },
	{ "sceKernelGetDirectMemorySize", (void (*)(void))sceKernelGetDirectMemorySize },
	{ "sceKernelLoadStartModule", (void (*)(void))sceKernelLoadStartModule },
	{ "sceKernelMapDirectMemory", (void (*)(void))sceKernelMapDirectMemory },
	{ "sceKernelMprotect", (void (*)(void))sceKernelMprotect },
	{ "sceKernelMunmap", (void (*)(void))sceKernelMunmap },
	{ "sceKernelReadTsc", (void (*)(void))sceKernelReadTsc },
	{ "sceKernelReleaseDirectMemory", (void (*)(void))sceKernelReleaseDirectMemory },
	{ "sceKernelReserveVirtualRange", (void (*)(void))sceKernelReserveVirtualRange },
	{ "sceKernelWaitEqueue", (void (*)(void))sceKernelWaitEqueue },
	{ "sceKeyboardClose", (void (*)(void))sceKeyboardClose },
	{ "sceKeyboardInit", (void (*)(void))sceKeyboardInit },
	{ "sceKeyboardOpen", (void (*)(void))sceKeyboardOpen },
	{ "sceKeyboardReadState", (void (*)(void))sceKeyboardReadState },
	{ "scePadClose", (void (*)(void))scePadClose },
	{ "scePadGetHandle", (void (*)(void))scePadGetHandle },
	{ "scePadInit", (void (*)(void))scePadInit },
	{ "scePadOpen", (void (*)(void))scePadOpen },
	{ "scePadReadState", (void (*)(void))scePadReadState },
	{ "scePadSetLightBar", (void (*)(void))scePadSetLightBar },
	{ "scePadSetMotionSensorState", (void (*)(void))scePadSetMotionSensorState },
	{ "scePadSetTriggerEffect", (void (*)(void))scePadSetTriggerEffect },
	{ "scePadSetVibration", (void (*)(void))scePadSetVibration },
	{ "scePadSetVibrationMode", (void (*)(void))scePadSetVibrationMode },
	{ "sceSystemServiceLaunchWebBrowser", (void (*)(void))sceSystemServiceLaunchWebBrowser },
	{ "sceUserServiceGetForegroundUser", (void (*)(void))sceUserServiceGetForegroundUser },
	{ "sceUserServiceGetLoginUserIdList", (void (*)(void))sceUserServiceGetLoginUserIdList },
	{ "sceUserServiceGetUserName", (void (*)(void))sceUserServiceGetUserName },
	{ "sceUserServiceInitialize", (void (*)(void))sceUserServiceInitialize },
	{ "sceVideoOutAddFlipEvent", (void (*)(void))sceVideoOutAddFlipEvent },
	{ "sceVideoOutClose", (void (*)(void))sceVideoOutClose },
	{ "sceVideoOutConfigureOutput", (void (*)(void))sceVideoOutConfigureOutput },
	{ "sceVideoOutDeleteFlipEvent", (void (*)(void))sceVideoOutDeleteFlipEvent },
	{ "sceVideoOutGetFlipStatus", (void (*)(void))sceVideoOutGetFlipStatus },
	{ "sceVideoOutIsOutputSupported", (void (*)(void))sceVideoOutIsOutputSupported },
	{ "sceVideoOutOpen", (void (*)(void))sceVideoOutOpen },
	{ "sceVideoOutRegisterBuffers2", (void (*)(void))sceVideoOutRegisterBuffers2 },
	{ "sceVideoOutSetBufferAttribute2", (void (*)(void))sceVideoOutSetBufferAttribute2 },
	{ "sceVideoOutSetFlipRate", (void (*)(void))sceVideoOutSetFlipRate },
	{ "sceVideoOutSubmitFlip", (void (*)(void))sceVideoOutSubmitFlip },
	{ "sceVideoOutWaitVblank", (void (*)(void))sceVideoOutWaitVblank },
	{ "sceSystemServiceHideSplashScreen", (void (*)(void))sceSystemServiceHideSplashScreen },
	{ "sceKernelSendNotificationRequest", (void (*)(void))sceKernelSendNotificationRequest },
};

static void check_imports(void)
{
	unsigned missing = 0;
	for (size_t i = 0; i < sizeof(ps5_imports) / sizeof(ps5_imports[0]); i++) {
		void (*volatile fn)(void) = ps5_imports[i].fn;
		if (!fn) {
			watch_printf("import unresolved: %s\n", ps5_imports[i].name);
			missing++;
		}
	}
	watch_printf("imports: %u of %u unresolved\n", missing, (unsigned)(sizeof(ps5_imports) / sizeof(ps5_imports[0])));
}

/* A system library the title does not get loaded for it (libSceKeyboard, libSceMouse,
 * libSceImeDialog: their imports came back unresolved, and /system/common/lib is not where a
 * title sees them: that load said not found). A title's view of /system is under a randomized
 * name, libkernel's sceKernelGetFsSandboxRandomWord(), as the payload SDK's loader finds it
 * (with syscall 602). The handle, or a negative error. */

int ps5LoadSystemModule(const char *name)
{
	char path[512];
	int handle = -1;
	const char *word = NULL;
	{
		const char *(*randomWord)(void) = NULL;
		/* libkernel by either of the handles it goes by */
		if (sceKernelDlsym(0x2001, "sceKernelGetFsSandboxRandomWord", (void **)&randomWord) != 0) {
			sceKernelDlsym(1, "sceKernelGetFsSandboxRandomWord", (void **)&randomWord);
		}
		word = randomWord ? randomWord() : NULL;
	}
	const int sandbox = word && word[0];
	for (int i = 0; i < 4 && handle < 0; i++) {
		static const char *const dirs[] = { "common/lib", "priv/lib" };
		if (i < 2) {
			if (!sandbox) {
				continue;
			}
			snprintf(path, sizeof(path), "/%s/%s/%s", word, dirs[i], name);
		} else {
			snprintf(path, sizeof(path), "/system/%s/%s", dirs[i - 2], name);
		}
		handle = sceKernelLoadStartModule(path, 0, NULL, 0, NULL, NULL);
	}
	printf("PS5: %s: %s (0x%08x, sandbox %s)\n", name, handle >= 0 ? "loaded" : "not loaded", (unsigned)handle,
		sandbox ? word : "unknown");
	return handle;
}

/* A function of a library ps5LoadSystemModule() loaded, or NULL */
void *ps5SystemSym(int handle, const char *name)
{
	void *addr = NULL;
	if (handle < 0 || sceKernelDlsym(handle, name, &addr) != 0) {
		return NULL;
	}
	return addr;
}

/* getaddrinfo for the title (link-title.sh binds the name here): numbers as they are, names
 * through the console's resolver in libSceNet. The SDK's own only fails ("Could not resolve
 * hostname" for the lobby, 2026-10-18). IPv4 answers; an IPv6 request gets them mapped. */
/* Plain imports: weak ones were never bound (each came back NULL, 2026-10-18). Each is read
 * through a volatile pointer, so an import the console leaves unresolved is a failed lookup
 * rather than a call to address 0. */
int sceNetPoolCreate(const char *name, int size, int flags);
int sceNetResolverCreate(const char *name, int memid, int flags);
int sceNetResolverStartNtoa(int rid, const char *hostname, uint32_t *addr, int timeout, int retry, int flags);
int sceNetResolverDestroy(int rid);

static int resolve_name(const char *name, struct in_addr *out)
{
	static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	static int pool = -1;
	if (!strcasecmp(name, "localhost")) {
		out->s_addr = htonl(INADDR_LOOPBACK);
		return 0;
	}
	int (*volatile poolCreate)(const char *, int, int) = sceNetPoolCreate;
	int (*volatile resolverCreate)(const char *, int, int) = sceNetResolverCreate;
	int (*volatile startNtoa)(int, const char *, uint32_t *, int, int, int) = sceNetResolverStartNtoa;
	int (*volatile resolverDestroy)(int) = sceNetResolverDestroy;
	if (!poolCreate || !resolverCreate || !startNtoa || !resolverDestroy) {
		static int told;
		if (!told) {
			told = 1;
			printf("PS5: no resolver to look up %s (pool %p create %p ntoa %p)\n", name, (void *)poolCreate,
				(void *)resolverCreate, (void *)startNtoa);
		}
		return -1;
	}
	pthread_mutex_lock(&lock);
	if (pool < 0) {
		pool = poolCreate("pd_resolver", 64 * 1024, 0);
	}
	const int memid = pool;
	pthread_mutex_unlock(&lock);
	const int rid = resolverCreate("pd_resolver", memid, 0);
	if (rid < 0) {
		printf("PS5: resolver for %s: 0x%08x (pool 0x%08x)\n", name, (unsigned)rid, (unsigned)memid);
		return -1;
	}
	uint32_t addr = 0;
	const int r = startNtoa(rid, name, &addr, 0, 0, 0);
	resolverDestroy(rid);
	if (r < 0 || addr == 0) {
		printf("PS5: %s did not resolve: 0x%08x\n", name, (unsigned)r);
		return -1;
	}
	out->s_addr = addr;
	return 0;
}

int pd_ps5_getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res)
{
	const int family = hints ? hints->ai_family : AF_UNSPEC;
	const int flags = hints ? hints->ai_flags : 0;
	int port = 0;
	struct in_addr v4;
	struct in6_addr v6;
	int is6 = 0;

	if (!res) {
		return EAI_FAIL;
	}
	*res = NULL;
	watch_printf("getaddrinfo %s port %s family %d socktype %d\n", node ? node : "(none)", service ? service : "-",
		family, hints ? hints->ai_socktype : -1);
	if (family != AF_UNSPEC && family != AF_INET && family != AF_INET6) {
		return EAI_FAMILY;
	}
	if (service && *service) {
		char *end;
		const long n = strtol(service, &end, 10);
		if (*end == '\0' && n >= 0 && n < 65536) {
			port = (int)n;
		} else if (!strcasecmp(service, "http")) {
			port = 80;
		} else if (!strcasecmp(service, "https")) {
			port = 443;
		} else {
			return EAI_SERVICE;
		}
	}
	if (!node) {
		if (family == AF_INET6) {
			is6 = 1;
			v6 = (flags & AI_PASSIVE) ? in6addr_any : in6addr_loopback;
		} else {
			v4.s_addr = htonl((flags & AI_PASSIVE) ? INADDR_ANY : INADDR_LOOPBACK);
		}
	} else if (inet_pton(AF_INET6, node, &v6) == 1) {
		if (family == AF_INET) {
			return EAI_NONAME;
		}
		is6 = 1;
	} else if (inet_pton(AF_INET, node, &v4) != 1) {
		if ((flags & AI_NUMERICHOST) || resolve_name(node, &v4) != 0) {
			return EAI_NONAME;
		}
	}
	if (!is6 && family == AF_INET6) {
		/* v4-mapped, ::ffff:a.b.c.d */
		memset(&v6, 0, sizeof(v6));
		v6.s6_addr[10] = 0xff;
		v6.s6_addr[11] = 0xff;
		memcpy(&v6.s6_addr[12], &v4, 4);
		is6 = 1;
	}

	struct block {
		struct addrinfo ai;
		struct sockaddr_storage ss;
	} *b = calloc(1, sizeof(*b));
	if (!b) {
		return EAI_MEMORY;
	}
	b->ai.ai_flags = flags;
	b->ai.ai_socktype = hints ? hints->ai_socktype : 0;
	b->ai.ai_protocol = hints ? hints->ai_protocol : 0;
	b->ai.ai_addr = (struct sockaddr *)&b->ss;
	if (is6) {
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&b->ss;
		sin6->sin6_len = sizeof(*sin6);
		sin6->sin6_family = AF_INET6;
		sin6->sin6_port = htons((uint16_t)port);
		sin6->sin6_addr = v6;
		b->ai.ai_family = AF_INET6;
		b->ai.ai_addrlen = sizeof(*sin6);
	} else {
		struct sockaddr_in *sin = (struct sockaddr_in *)&b->ss;
		sin->sin_len = sizeof(*sin);
		sin->sin_family = AF_INET;
		sin->sin_port = htons((uint16_t)port);
		sin->sin_addr = v4;
		b->ai.ai_family = AF_INET;
		b->ai.ai_addrlen = sizeof(*sin);
	}
	*res = &b->ai;
	{
		char text[64] = "";
		inet_ntop(b->ai.ai_family, is6 ? (const void *)&v6 : (const void *)&v4, text, sizeof(text));
		watch_printf("getaddrinfo %s -> %s\n", node ? node : "(none)", text);
	}
	return 0;
}

void pd_ps5_freeaddrinfo(struct addrinfo *ai)
{
	while (ai) {
		struct addrinfo *next = ai->ai_next;
		free(ai); /* one block each, the addrinfo first */
		ai = next;
	}
}

/* fcntl, as the title's code calls it (link-title.sh wraps the name). The console answers
 * EINVAL to F_SETFD's close-on-exec on a socket, and curl gave up on every connection over
 * it ("fcntl set CLOEXEC: Invalid argument", 2026-10-18): there is no exec on the console, so
 * that is done. A non-blocking mode it refuses goes through FIONBIO instead. */
int __real_fcntl(int fd, int cmd, ...);
int __wrap_fcntl(int fd, int cmd, ...)
{
	va_list args;
	va_start(args, cmd);
	const long arg = va_arg(args, long);
	va_end(args);
	const int r = __real_fcntl(fd, cmd, arg);
	if (r == -1 && errno == EINVAL) {
		static int told_setfd, told_setfl;
		if (cmd == F_SETFD) {
			if (!told_setfd) {
				told_setfd = 1;
				watch_printf("fcntl F_SETFD 0x%lx on %d refused; taken as done\n", arg, fd);
			}
			return 0;
		}
		if (cmd == F_SETFL) {
			int on = (arg & O_NONBLOCK) != 0;
			const int io = ioctl(fd, FIONBIO, &on);
			if (!told_setfl) {
				told_setfl = 1;
				watch_printf("fcntl F_SETFL 0x%lx on %d refused; FIONBIO %d\n", arg, fd, io);
			}
			if (io == 0) {
				return 0;
			}
			errno = EINVAL;
		}
	}
	return r;
}

/* The last thing written before the game ends itself (sysFatal, exit()): into the watchdog's
 * file at once, since a line through stdio's pipe is lost when the process goes before the
 * writer gets to it. With the code addresses on the stack, as a crash's report has. */
static void stack_scan(const char *who)
{
	const uintptr_t code_start = (uintptr_t)&main & ~(uintptr_t)0x3fffff;
	const uintptr_t code_end = code_start + 0x2000000;
	const uintptr_t *stack = (const uintptr_t *)__builtin_frame_address(0);
	const uintptr_t *top = (const uintptr_t *)stack_end((uintptr_t)stack);
	int found = 0;
	watch_printf("%s (main at %p)\n", who, (void *)&main);
	for (int i = 0; i < 4096 && found < 32 && stack + i < top; i++) {
		if (stack[i] >= code_start && stack[i] < code_end) {
			watch_printf("  stack[%d] %p\n", i, (void *)stack[i]);
			found++;
		}
	}
}

/* A line in the watch file now (open, write, close: kept if the title dies the next instant), and
 * the frames after it traced too (gfx_pc.cpp, gfx_vulkan.cpp): where a death no signal reports
 * (a GPU fault ends the process outright) happened, by the last line written */
volatile int ps5_trace_frames;
void ps5Trace(int frames, const char *fmt, ...)
{
	char line[256];
	va_list args;
	va_start(args, fmt);
	vsnprintf(line, sizeof(line), fmt, args);
	va_end(args);
	watch_printf("[frame %u, present %u] %s\n", ps5_frames_started, ps5_presents, line);
	if (frames > ps5_trace_frames) {
		ps5_trace_frames = frames;
	}
}

void ps5LastWords(const char *fmt, ...)
{
	char line[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(line, sizeof(line), fmt, args);
	va_end(args);
	watch_printf("last words: %s\n", line);
	stack_scan("last words from");
}

static void on_exit_report(void)
{
	stack_scan("exit() handlers done");
}

/* exit() and _exit() as everything in the title calls them (link-title.sh wraps both): said
 * first, before any atexit handler that might hang */
void __real_exit(int status) __attribute__((noreturn));
void __real__exit(int status) __attribute__((noreturn));
void __wrap_exit(int status)
{
	char who[48];
	snprintf(who, sizeof(who), "exit(%d) called", status);
	stack_scan(who);
	__real_exit(status);
}

void __wrap__exit(int status)
{
	char who[48];
	snprintf(who, sizeof(who), "_exit(%d) called", status);
	stack_scan(who);
	__real__exit(status);
}

/* ps5_rom.c: the player's ROMs put where the game looks for them */
void ps5ImportRoms(void);

void ps5PlatformInit(int *argc, const char ***argv)
{
	{
		FILE *f = fopen(WATCH_PATH, "w");
		if (f) {
			fclose(f);
		}
	}
	open_log();

	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_sigaction = on_signal;
	action.sa_flags = SA_SIGINFO | SA_ONSTACK;
	{
		/* the game thread's report even when it ran out of stack */
		static char alt[64 * 1024];
		stack_t ss = { .ss_sp = alt, .ss_size = sizeof(alt), .ss_flags = 0 };
		sigaltstack(&ss, NULL);
	}
	sigemptyset(&action.sa_mask);
	/* Every signal whose default ends the process: online play ended it with none of the
	 * first six (2026-10-18), and an unhandled one leaves no report */
	const int sigs[] = { SIGABRT, SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGSYS, SIGTRAP, SIGTERM, SIGHUP, SIGINT,
		SIGQUIT, SIGALRM, SIGVTALRM, SIGPROF, SIGXCPU, SIGXFSZ, SIGUSR2, SIGEMT };
	for (size_t i = 0; i < sizeof(sigs) / sizeof(sigs[0]); i++) {
		sigaction(sigs[i], &action, NULL);
	}
	main_thread = pthread_self();

	{
		struct sigaction sample;
		memset(&sample, 0, sizeof(sample));
		sample.sa_sigaction = on_sample;
		sample.sa_flags = SA_SIGINFO | SA_RESTART;
		sigemptyset(&sample.sa_mask);
		sigaction(SIGUSR1, &sample, NULL);
		pthread_t thread;
		if (pthread_create(&thread, NULL, watchdog, NULL) == 0) {
			pthread_detach(thread);
		}
	}
	check_imports();
	atexit(on_exit_report);

	/* A peer that goes away mid-send must not end the game: online play, the lobby, curl */
	signal(SIGPIPE, SIG_IGN);

	/* There is no video driver for the console's display: the window is an offscreen window,
	 * and the Vulkan renderer presents through VK_KHR_display (gfx_vulkan.cpp, __PROSPERO__). */
	SDL_SetHintWithPriority(SDL_HINT_VIDEODRIVER, "offscreen", SDL_HINT_OVERRIDE);
	/* SDL's own SIGINT/SIGTERM handlers would replace the title's and turn either signal into a
	 * quit event, which the game answers with a plain exit(0): a console that ended the game
	 * that way left nothing in the log. The title's handler reports them instead. */
	SDL_SetHintWithPriority(SDL_HINT_NO_SIGNAL_HANDLERS, "1", SDL_HINT_OVERRIDE);
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
	static const char *defaults[8] = { "--portable", "--vulkan" };
	int ndefaults = 2;

	/* The GoldenEye tile is the same game installed as a second title (PPSA64008),
	 * which opens GoldenEye as soon as the Perfect Menu is up. */
	{
		char json[8192] = { 0 };
		FILE *f = fopen("/app0/sce_sys/param.json", "rb");
		if (f) {
			fread(json, 1, sizeof(json) - 1, f);
			fclose(f);
		}
		if (strstr(json, "\"PPSA64008\"")) {
			defaults[ndefaults++] = "--ps5-goldeneye";
			printf("PS5: GoldenEye tile\n");
		}
	}
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
