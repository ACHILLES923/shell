#include  <stdio.h>
#include  <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdbool.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64
#define MAX_NUM_BACKGROUND_PROCESS 64

/* Splits the string by space and returns the array of tokens
*
*/
char **tokenize(char *line)
{
  char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
  char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
  int i, tokenIndex = 0, tokenNo = 0;

  for(i =0; i < strlen(line); i++){

    char readChar = line[i];

    if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
      token[tokenIndex] = '\0';
      if (tokenIndex != 0){
	tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
	strcpy(tokens[tokenNo++], token);
	tokenIndex = 0; 
      }
    } else {
      token[tokenIndex++] = readChar;
    }
  }
 
  free(token);
  tokens[tokenNo] = NULL ;
  return tokens;
}

void cd_system_call(char** tokens)
{
	// Calculating the size of tokens
	int num_of_tokens = 0;
	while(tokens[num_of_tokens] != NULL) 
		num_of_tokens++;
       
    // If only cd is typed in prompt i.e only one token, then do nothing
    if(num_of_tokens == 1) return;   

	// data stores metadata when checking if the path is valid
	struct stat data;
	// If more that one path is provided for cd or the path is not valid, then error
	if(num_of_tokens > 2 || stat(tokens[1], &data) != 0) 
	{
		printf("Shell: Incorrect command\n");
	}
	// else Change directory
	else chdir(tokens[1]);
}

void shell_execute(char** tokens, int* background_process_list, int* size)
{
	// path_to_executable contains path of the executable for the command
	char path_to_executable[64+9] = "";
	bool is_background_process = false;   // Is the current process to be done in background
	// Counting number of tokens in tokens
	int num_of_tokens = 0;
	while(tokens[num_of_tokens] != NULL) 
		num_of_tokens++;

	// If the last token is '&', then it is a background process
	if(strcmp(tokens[num_of_tokens-1], "&") == 0) 
	{
		// Then just make last token NULL( because will use it as list for execv call) and toggle the variable for background process. 
		tokens[--num_of_tokens] = NULL;
		is_background_process = true;
	}

	int pid = fork();
	
	// If fork fails for some reason
	if(pid < 0)
	{
		printf("Error occured. Please try again!\n");
		return;
	}
	// For the child process
	else if(pid == 0)
	{
		
		// Forming the path to executable
		strcat(path_to_executable, "/usr/bin/");
		strcat(path_to_executable, tokens[0]);
		// Calling exec from child process. 
		// If the exec call is success, then the command is executed.
		execv(path_to_executable, tokens);
		// If command or the arguments are incorrect, then exec call fails. So print an error message and end the child process.
		printf("Shell: The command or arguments provided are incorrect.\n");
		exit(0);
	}
	// For the parent process/current process
	else
	{
		// If the process is a background process
		if(is_background_process)
		{	
			// Add it PID to the list and increment size by 1
			background_process_list[*size] = pid;
			*size = *size + 1;
			return;
		}
		// else it is foreground, so just wait for child to finish
		else 
		{
			int status;
			waitpid(pid, &status, 0);
		}
	}
}

// This function reaps child processes.
void reap_background_process(int* background_process_list, int* size)
{
	// check all the PID's in the background_process_list
	for(int i=0; i<(*size); i++)
	{
		// If empty, then move on
		if(background_process_list[i] == -1) continue;
		int status;
		// Checking status of the child process. If complete, then it returns -1, else if it is going on, returns 0
		int return_status = waitpid(background_process_list[i], &status, WNOHANG);

		// If complete
		if(return_status == -1) 
		{
			printf("Shell: Background process finished. PID: %d \n", background_process_list[i]);
			// Now move all the PID's to left by one to remove that entry from list
			for(int j=i; j<*size; j++) background_process_list[j] = background_process_list[j+1];
			*size = *size - 1;
		}
	}
}


int main(int argc, char* argv[]) {
	char  line[MAX_INPUT_SIZE];            
	char  **tokens;              
	int i;

	// background_process_list stores PID's of all the background processes currently running and size is the number of it
	int* background_process_list = (int *)malloc(MAX_NUM_BACKGROUND_PROCESS*sizeof(int));
	int size = 0;
	// Make all of them -1
	for(int i=0; i<MAX_NUM_BACKGROUND_PROCESS; i++) background_process_list[i] = -1;

	while(1) {			
		/* BEGIN: TAKING INPUT */
		bzero(line, sizeof(line));
		printf("$ ");
		scanf("%[^\n]", line);
		getchar();

		// printf("Command entered: %s (remove this debug output later)\n", line);
		/* END: TAKING INPUT */

		line[strlen(line)] = '\n'; //terminate with new line
		tokens = tokenize(line);
		
		// If nothing is typed in prompt, then continue
		if(tokens[0] == NULL) continue;
		// If command is not  "cd", then call shell_execute
		if(strcmp(tokens[0], "cd")) shell_execute(tokens, background_process_list, &size);
		// else call cd_system_call
		else cd_system_call(tokens);

		// Now check if any background process has finished. If so, then reap it
		reap_background_process(background_process_list, &size);
		

		// printf("Total background processes currently: %d \n", size);
		// for(int i=0; i<size; i++) printf("process %d : %d \n", i+1, background_process_list[i]);

		// for(i=0;tokens[i]!=NULL;i++){
		// 	printf("found token %s (remove this debug output later)\n", tokens[i])
       
		// Freeing the allocated memory	
		for(i=0;tokens[i]!=NULL;i++){
			free(tokens[i]);
		}
		free(tokens);
	}
	return 0;
}
