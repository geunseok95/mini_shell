#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#define FALSE 0
#define TRUE 1

#define EOL 1
#define ARG 2
#define INRD 3
#define OUTRD 4
#define PIPE 5
#define AMPERSAND 6

#define FOREGROUND 0
#define BACKGROUND 1
//argument 잘 고치자!
static char input[512];
static char tokens[1024];
char        *ptr, *tok;

int get_token(char **outptr)
{
	int type;

	*outptr = tok;
	while ((*ptr == ' ') || (*ptr == '\t')) ptr++;

	*tok++ = *ptr;

	switch (*ptr++) {
	case '\0': type = EOL; break;
	case '&': type = AMPERSAND; break;
	default: type = ARG;
		while ((*ptr != ' ') && (*ptr != '&') &&
			(*ptr != '\t') && (*ptr != '\0'))
			*tok++ = *ptr++;
	}
	*tok++ = '\0';
	return(type);
}

int execute(char **comm, int how)
{
	int pid;

	if ((pid = fork()) < 0) {
		fprintf(stderr, "minish : fork error\n");
		return(-1);
	}
	else if (pid == 0) { //자식은 exec 실행
		execvp(*comm, comm);
		fprintf(stderr, "minish : command not found\n");
		exit(127); //제대로 exec하면 실행 x
	}
	if (how == BACKGROUND) {    /* Background execution 부모도 실행!*/
		printf("[%d]\n", pid);
		return 0;
	}
	/* Foreground execution 끝날 때까지 기다린다.*/
	while (waitpid(pid, NULL, 0) < 0)
		if (errno != EINTR) return -1;
	return 0;
}

int parse_and_execute(char *input)
{
	char    *arg[1024];
	int type, how;
	int quit = FALSE;
	int narg = 0;
	int finished = FALSE;

	ptr = input;
	tok = tokens;
	while (!finished) {
		switch (type = get_token(&arg[narg])) {
		case ARG:
			narg++;
			break;
		case INRD:
			narg++;
			break;
		case OUTRD:
			narg++;
			break;
		case PIPE:
			narg++;
			break;
		case EOL:
		case AMPERSAND:
			if (!strcmp(arg[0], "quit")) quit = TRUE;
			else if (!strcmp(arg[0], "exit")) quit = TRUE;
			else if (!strcmp(arg[0], "cd")) {
				chdir(arg[1]);
			}
			else if (!strcmp(arg[1], ">")) {
				int pid, fd;

				pid = fork();
				if (pid == 0) {
					fd = open(arg[2], O_RDWR | O_CREAT | O_TRUNC | S_IROTH, 0644);
					if (fd < 0) {
						perror("error");
						exit(-1);
					}
					dup2(fd, STDOUT_FILENO);
					close(fd);

					execlp(*arg, arg[0], (char *)0);

					fprintf(stderr, "minish : command not found\n");
					exit(0);
				}
				wait();

			}
			else if (!strcmp(arg[1], "<")) {
				int pid, fd;

				pid = fork();
				if (pid == 0) {
					fd = open(arg[2], O_RDWR | O_CREAT | O_TRUNC | S_IROTH, 0644);
					if (fd < 0) {
						perror("error");
						exit(-1);
					}
					dup2(fd, STDIN_FILENO);
					close(fd);

					execlp(*arg, arg[0], (char *)0);

					fprintf(stderr, "minish : command not found\n");
					exit(0);
				}
				wait();

			}
			else if (!strcmp(arg[1], "|")) {
				int p[2];
				int pid1, pid2;


				pid1 = fork();
				if (pid1 == 0) {
					pipe(p);

					pid2 = fork();
					if (pid2 == 0) {
						dup2(p[0], STDIN_FILENO);
						close(p[0]);
						close(p[1]);

						execlp(arg[2], arg[2], (char *)0);

					}
					dup2(p[1], STDOUT_FILENO);
					close(p[0]);
					close(p[1]);
					execlp(*arg, arg[0], (char *)0);
					fprintf(stderr, "minish : command not found\n");

				}



				wait();

			}


			else if (!strcmp(arg[0], "type")) {
				if (narg > 1) {
					int i, fid;
					int readcount;
					char    buf[512];

					fid = open(arg[1], O_RDWR | O_CREAT | O_TRUNC | S_IROTH, 0644);
					if (fid >= 0) {
						readcount = read(fid, buf, 512);
						while (readcount > 0) {
							for (i = 0; i < readcount; i++)
								putchar(buf[i]);
							readcount = read(fid, buf, 512);
						}
						dup2(fid, 1);
						write(fid, buf, 512);
					}
					close(fid);

				}
			}
			else { //&의 유무 background,,,,
				how = (type == AMPERSAND) ? BACKGROUND : FOREGROUND;
				arg[narg] = NULL;
				if (narg != 0)
					execute(arg, how);
			}
			narg = 0;
			if (type == EOL)
				finished = TRUE;
			break;
		}
	}
	return quit;
}

main()
{
	char    *arg[1024];
	int quit;

	printf("msh # ");
	while (gets(input)) {
		quit = parse_and_execute(input);
		if (quit) break;
		printf("msh # ");
	}
}
