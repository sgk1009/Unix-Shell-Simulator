//all of our includes
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>

//declarations
pid_t waitpid(pid_t pid, int *status, int options);

//part 2 and 3 functions
void cd(char* c);
void pipe_call(char *c[], pid_t *pid);
void basic_call(char *c[]);
void built_in_history(char* t);

//all of smaller functions
void read_in_line();
void add_to_history();
void count_pipes();
void run_command();
void create_arguments();
void no_pipes();
void first_or_middle();
void last_pipe();

//global variables
char* myargs[100];
char* history[100];
char bar = '|';

char *input, *nl, *command, *duplicate;
char *str1, *str2, *cmdln, *args, *saveptr1, *saveptr2;

int historycount = 0;
size_t inlen = 0;
int pipecount = 0;
int lastpipe = 0;

ssize_t rd;
int i;
int prev_pipe,fd1[2];
pid_t cpid; //ppid, cpid2;
int savedOut, savedIn;

int main(int argc, char* argv[])
{
    while(1)
    {
        input = NULL;          //initialize our input first/ and reset it every loop
        savedIn = dup(0);     //save input
        savedOut = dup(1);   //save output

        //assume memory is full of garbage, so clear
        memset(myargs, 0, sizeof(myargs));

        //print prompt
        printf("%s", "sish> ");

        read_in_line();     //read our line

        add_to_history();   //add every function to history after getting it 

        //check for exit 
        if(strstr(input, "exit") != NULL)
            break;

        count_pipes();      //get our pipe count

        lastpipe = pipecount;
        prev_pipe = STDIN_FILENO;

        run_command();  

        //reset stdin and stdout
        dup2(savedOut,1);
        close(savedOut);
        dup2(savedIn,0);
        close(savedIn);

        pipecount = 0;    //reset pipecount to 0

    }//end of while loop

    exit(EXIT_SUCCESS);
}//end of main

//for all commands
void read_in_line()
{
		//read line
        rd = getline(&input, &inlen, stdin);
        if(rd == -1)
        {
              perror("Failed to read line");
              exit(EXIT_SUCCESS);
        }

        //remove trailing next line
        nl = strchr(input, '\n');
        if(nl != NULL)
            *nl = '\0';
}

//add input to our history array
void add_to_history()
{
		//history should hold only the last 100 lines
        //so, if it's full, make some room!
        if(historycount > 100)
        {
            for(int b = 0; b < historycount - 1 ; b++)
            {
                        history[b] = history[b + 1];       //shift all items down one index, effectively getting rid of the first input
            }

            historycount--;     //reset our history account to be below 100
        }

        //add enough memory for input into history at "historycount" index
        history[historycount] = (char*) malloc(strlen(input));

        //add new line to history
        strncpy(history[historycount], input, strlen(input));
        historycount++;     //up historycount 
}

//count the number of pipes to determine the number of loop iterations
void count_pipes()
{
	//copy input
	duplicate = (char*) malloc(strlen(input));
	duplicate = strdup(input);

	//count number of pipes, 0 if none
	for(int i = 0; i < strlen(duplicate); i++)
	{
	       if(duplicate[i] == bar)
	             pipecount++;
	}

	//no leaks
	free(duplicate);
}

//if/else statements to see if we use our pipe functions or our built in functions
void run_command()
{
	//tokenize the line
	int j;
	for(j = 0, str1 = input; ; j++, str1 = NULL)
	{
		cmdln = strtok_r(str1, "|", &saveptr1);

		if(cmdln == NULL)
			break;

        create_arguments();

        if(pipecount == 0)    //no pipes in input line
        	no_pipes();
	    else
	    {
	    	//first and middle pipes here
	    	if(lastpipe != 0)
	    		first_or_middle();
			else
				last_pipe();
	    }//end of pipes
	}//end of for loop
}

//store individual arguments into our array "myargs"
void create_arguments()
{
	int k;
	for(k = 0, str2 = cmdln; ; k++, str2 = NULL)
	{
		args = strtok_r(str2, " ", &saveptr2);
		if(args == NULL)
			break;

		myargs[k] = args;
	}

	myargs[k] = NULL;
	command = myargs[0];    //our keyword to see which function we need to execute
}

//if/else statement to direct which function to call (cd,history or execvp)
void no_pipes()
{
	 if (strncmp("cd", command, 2) == 0)
		 cd(myargs[1]);
	 else if (strncmp("history", command, 7) == 0)
		 built_in_history(myargs[1]);
	 else
		 basic_call(myargs);
}

//first or middle pipe function
void first_or_middle()
{
	if(pipe(fd1) == -1)
	{
		perror("error in making the pipe");     //error checking
		return;
	}

	pid_t ppid = fork();
	if(ppid < 0)
	{
		perror("forking problem in pipes");     //error checking
		return;
	}

	if(ppid == 0)
	{
		if(prev_pipe != STDIN_FILENO)
		{
			if(dup2(prev_pipe,STDIN_FILENO) == -1)  //error checking
				perror("dup2 error in pipe");

			close(prev_pipe);
		}

		if(dup2(fd1[1],STDOUT_FILENO) == -1)
			perror("dup2 error in piping 2");       //error checking

		close(fd1[1]);                              //close write end

		pipe_call(myargs, &ppid);                   //execvp for pipes
	}

	close(prev_pipe);
	close(fd1[1]);                                  //close write end
	prev_pipe = fd1[0];                             //set read end
	lastpipe --;                                    //last pipe counter decreases 
}

//last pipe function
void last_pipe()
{
	if(prev_pipe != STDIN_FILENO)
	{
		if(dup2(prev_pipe,STDIN_FILENO) == -1)
		   	perror("dup2 in the last command failed");      //error checking

		close(prev_pipe);
	}

	pid_t cpid2 = fork();
	if(cpid2 < 0)
	{
		perror("fork cpid1 failed");        //error checking
		return;
	}

	pipe_call(myargs, &cpid2);          //execvp for pipes

	close(fd1[0]);      //close read end
	close(fd1[1]);      //close write end
}

//execvp call for pipes
void pipe_call(char *my_args[], pid_t *pid)
{
	//child
    if(*pid == 0)
    {
        execvp(my_args[0], my_args);
        perror("basic execvp child failed");        //error checking
    }
    //parent
    else
    	waitpid(*pid, NULL, 0);
    	return;
}

//execvp calls for built-ins and pipe-less calls
void basic_call(char *my_args[])
{
	cpid = fork();
	if(cpid < 0)
	{
		perror("fork cpid1 failed");    //error checking
		return;
	}

	//child
    if(cpid == 0)
    {
        execvp(my_args[0], my_args);
        perror("basic execvp child failed");    //error checking
    }
    //parent
    else
    {
    	waitpid(cpid, NULL, 0);
    	return;
    }
}

//built in cd
void cd(char* dest)
{
    char s[100];

    if(chdir(dest)!=0)
    {
        perror("failed to change directory /n");
    }

    printf("%s\n", getcwd(s, 100));     //print current working directory
}

//built in history
void built_in_history(char* arg)
{
	//regular history call
    if(arg == NULL)
    {
        //print the history
        for(int x = 0; x < historycount; x++)
                printf("%d %s\n", x, history[x]);
    }

    //clear history call
    else if(strncmp("-c", arg, 2) == 0)
    {
        //clear history and reset history count
        memset(history, 0, (sizeof(*history)*historycount));
        historycount = 0;
    }

    //call command in history
    else
    {
        //parse the number
        int d = atoi(arg);
        //grab the line
        input =	history[d];
        pipecount = 0;

       //catch attempt to call history with history
       if(strstr(input, "history") != NULL)
       {
        	fprintf(stderr, "%s %d %s\n", "Sorry, history", d, "is another history call. Please try another command.");  
        	return;
       }

       count_pipes();
       run_command();

    }
}
