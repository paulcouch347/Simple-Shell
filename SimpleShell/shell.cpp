/*
Author: Paul Couch
Date: 4/28/2020
Class: CS 270
Assignment: Simple Shell
*/
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <iostream>
#include <signal.h>
#include <string.h>
#include <string>
#include "shell.h"

using namespace std;

const int STDOUT_FD = 1;
const int STDIN_FD = 0;
extern char** environ;

int call_redirected(const char* program, const char* args[], const char* outfile, const char* infile) {
	pid_t pid = fork();
	if (pid < 0) {
		perror("fork could not create child process");
		return 0;
	}
	else if (pid == 0) {
		if (outfile != NULL) {
			int outfd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
			if (outfd < 0) {
				perror("could not open output file");
				exit(1);
			}
			if (dup2(outfd, STDOUT_FD) < 0) {
				perror("could not duplicate output file descriptor");
				exit(1);
			}
		}
		if (infile != NULL) {
			int infd = open(infile, O_RDONLY);
			if (infd < 0) {
				perror("could not open input file");
				exit(1);
			}
			if (dup2(infd, STDIN_FD) < 0) {
				perror("could not duplicate input file descriptor");
				exit(1);
			}
		}
		execvp(program, (char* const*)args);
		perror("did not execute process");
		exit(1);
	}
	else {
		int status = 0;
		signal(SIGINT, SIG_IGN);
		if (waitpid(-1, &status, 0) < 0) {
			perror("could not wait for child");
			return 0;
		}
		if (WEXITSTATUS(status) != 0) {
			fprintf(stderr, "Command returned %d\n", status);
			return 0;
		}
		if (WIFSIGNALED(status)) {
			fprintf(stderr, "Command Terminated: %s\n", strsignal(WTERMSIG(status)));
			return 0;
		}
		signal(SIGINT, SIG_DFL);
		return WIFEXITED(status) && WEXITSTATUS(status) == 0;
	}
}//end call_redirected

int cd(const char* path) {
	if (chdir(path) < 0) {
		perror("could not change directory");
		return 1;
	}
	else
		return 0;
}//end cd

int cd() {
	char* homedir = getenv("HOME");
	if (homedir == NULL) {
		perror("could not find home directory");
		return 1;
	}
	else {
		chdir(homedir);
		return 0;
	}
}//end cd

int setEnvironment(const char* variable, const char* value) {
	if (setenv(variable, value, 1) < 0) {
		perror("could not set variable");
		return 1;
	}
	else
		return 0;
}//end setEnvironment

int removeEnvironment(const char* variable) {
	if (unsetenv(variable) < 0) {
		perror("could not remove variable");
		return 1;
	}
	else
		return 0;
}//end removeEnvironment

int main(int argc, char* argv[]) {
	string cmdLine;
	if (argc == 1) {//check if there ae any command line arguments
		cerr << "shell> ";
		getline(cin, cmdLine);
		command* cmd = parse_command(cmdLine.c_str());
		while (cmd->args[0] == NULL) {//loop to ignore blank command line
			cerr << "shell> ";
			getline(cin, cmdLine);
			cmd = parse_command(cmdLine.c_str());
		}
		while (strcmp(cmd->args[0], "exit") != 0) {
			if (strcmp(cmd->args[0], "cd") == 0) {
				if (cmd->args[1] != NULL)
					cd(cmd->args[1]);
				else
					cd();
			}
			else if (strcmp(cmd->args[0], "setenv") == 0) {
				if (cmd->args[1] == NULL)
					cerr << "cannot execute setenv without arguments\n";
				else if (cmd->args[2] == NULL)
					removeEnvironment(cmd->args[1]);
				else
					setEnvironment(cmd->args[1], cmd->args[2]);
			}
			else {
				call_redirected(cmd->args[0], (const char**)cmd->args, cmd->out_redir, cmd->in_redir);
			}
			free_command(cmd);
			cerr << "shell> ";
			getline(cin, cmdLine);
			cmd = parse_command(cmdLine.c_str());
			while (cmd->args[0] == NULL) {//loop to ignore blank command line
				cerr << "shell> ";
				getline(cin, cmdLine);
				cmd = parse_command(cmdLine.c_str());
			}
		}
		free_command(cmd);
	}
	else {//if a file is given as a command line, argument, open file
		int fd = open(argv[1], O_RDONLY);
		if (fd < 0) {
			perror("could not open input file");
			exit(1);
		}
		if (dup2(fd, STDIN_FD) < 0) {
			perror("could not duplicate input file descriptor");
			exit(1);
		}
		getline(cin, cmdLine);
		command* cmd = parse_command(cmdLine.c_str());
		while (cmd->args[0] == NULL) {//loop to ignore blank command line
			getline(cin, cmdLine);
			cmd = parse_command(cmdLine.c_str());
		}
		while (strcmp(cmd->args[0], "exit") != 0) {
			if (strcmp(cmd->args[0], "cd") == 0) {
				if (cmd->args[1] != NULL)
					cd(cmd->args[1]);
				else
					cd();
			}
			else if (strcmp(cmd->args[0], "setenv") == 0) {
				if (cmd->args[1] == NULL)
					cerr << "cannot execute setenv without arguments\n";
				else if (cmd->args[2] == NULL)
					removeEnvironment(cmd->args[1]);
				else
					setEnvironment(cmd->args[1], cmd->args[2]);
			}
			else {
				call_redirected(cmd->args[0], (const char**)cmd->args, cmd->out_redir, cmd->in_redir);
			}
			free_command(cmd);
			getline(cin, cmdLine);
			cmd = parse_command(cmdLine.c_str());
			while (cmd->args[0] == NULL) {//loop to ignore blank command line
				getline(cin, cmdLine);
				cmd = parse_command(cmdLine.c_str());
			}
		}
		free_command(cmd);
	}
	return 0;
}//end main