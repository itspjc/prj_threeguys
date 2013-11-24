#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv){
	pid_t pid = fork();

	if(pid == 0){
		puts("Hi I am a child process");
	}else{
		printf("Child process ID : %d\n", pid);
		sleep(30);
	}

	if(pid == 0){
		puts("End Child process");
	}else{
		puts("End Child process");
	}

	return 0;
}
