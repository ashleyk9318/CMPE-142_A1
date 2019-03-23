#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int cd(char **args);
int path(char **args);
int lsh_exit(char **args);
int redirect(char **argv);


char *builtin_str[] = {
  "cd",
  "path",
  "exit"
};

int (*builtin_func[]) (char **) = {
  &cd,
  &path,
  &lsh_exit
};

int num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

int cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("lsh");
    }
  }
  return 1;
}

int path(char **args)
{
  char path[1024];
  getcwd(path, sizeof(path));
  printf("PATH: %s", path);
  return *path;
}

int lsh_exit(char **args)
{
  return 0;
}

int launch(char **args)
{
  pid_t pid, wpid;
  int status;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("lsh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
      // Error forking
      perror("lsh");
  } else {
      // Parent process
      do {
        wpid = waitpid(pid, &status, WUNTRACED);
      } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}


int redirect(char **argv)
{
  int pid = fork();

  if (pid == -1) {
    perror("fork");
  } else if (pid == 0) {
    int in = 0, out = 0;
    char inF[1024], outF[1024];

    for (int i = 0; argv[i] != '\0'; i++) {
      if (strcmp(argv[i], "<") == 0) { // if attempt '<'
        argv[i] = NULL;
        strcpy(inF, argv[i+1]);
        in = 2;
      }
      if (strcmp(argv[i], ">") == 0) { // if attempt '>'
        argv[i] = NULL;
        strcpy(outF, argv[i+1]);
        out = 2;
      }
    }
    if (in) { // '<'
      int file0;
      if ((file0 = open(inF, 0)) < 0) { // User r, Group r, Other r
        perror("Error! Cannot open the file.");
        exit(0);
      }
      dup2(file0, 0);
      close(file0);
    }
    if (out) { // '>'
      int file1;
      if ((file1 = creat(outF, 0644)) < 0) { // User rw, Group rw, Other r
        perror("Error! Cannot create a file.");
        exit(0);
      }
      dup2(file1, 1);
      close(file1);
    }
    execvp(*argv, argv);
    perror("execvp");
    exit(1);
  }
  else if(pid < 0) {
    printf("fork failed.\n");
    exit(1);
  }
  return 1;
}


int execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return launch(args);
}

#define RL_BUFSIZE 1024
char *read_line(void)
{
  int bufsize = RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    c = getchar();

    if (c == EOF || c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;
  }
}

#define TOK_BUFSIZE 64
#define TOK_DELIM " \t\r\n\a"
char **split_line(char *line)
{
  int bufsize = TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  token = strtok(line, TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

void loop(void)
{
  char *line;
  char **args;
  int status;

  do {
    printf("CMPE 142_A1> ");
    line = read_line();
    args = split_line(line);
    status = execute(args);

    free(line);
    free(args);
  } while (status);
}


int main(int argc, char **argv)
{
  loop();

  return EXIT_SUCCESS;
}