
/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <regex.h>
#include "command.h"

void myunputc(int c);

SimpleCommand::SimpleCommand()
{
	// Creat available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **) malloc( _numberOfAvailableArguments * sizeof( char * ) );
}

void
SimpleCommand::insertArgument( char * argument )
{
	
	if ( _numberOfAvailableArguments == _numberOfArguments  + 1 ) {
		// Double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numberOfAvailableArguments * sizeof( char * ) );
	}
	
 	char* s1=strchr(argument,'$');
 	char* s2=strchr(argument,'{');
 	char* s3=strchr(argument,'}');
 	if(s1&&s2&&s3&&s1<s2&&s2<s3){
		char* newArgument=(char*)calloc(sizeof(char)*(strlen(argument)+1),0);
		char* a=argument;
		char* n=newArgument;
		while(*a){
			if(*a=='$'){
				a++;
				if(*a=='{'){
					a++;
					if(strchr(a,'}')!=NULL){
						char* val=(char*)calloc(strchr(a,'}')-a+1,0);
						memcpy(val,a,strchr(a,'}')-a);
						val[strlen(val)]='\0';
						char* newVal=getenv(val);
						strcpy(n,newVal);
						n=n+strlen(newVal);
						a=strchr(a,'}');
						free(val);
					}else{
						a=a-2;
						*n=*a;
						n++;
					}
				}else{
					a=a-1;
					*n=*a;
					n++;
				}
			}else{
				*n=*a;
				n++;
			}
			a++;
		}
		*n='\0';
		_arguments[ _numberOfArguments ] = newArgument;
	}else if(argument[0]=='~'){
		char* newArgument=(char*)calloc(6144,0);
		if(strlen(argument) == 1){
            strcat(newArgument, getenv("HOME"));
        }else if(argument[1]=='/'){
        	strcat(newArgument, getenv("HOME"));
        	argument++;
			strcat(newArgument, argument);
        }else{
        	 char* na=strdup(argument+1);
        	 char* user=strtok(na,"/");
        	 if(user==NULL)
        	 	user=strdup(na);

        	 struct passwd *p = getpwnam(user);

        	 strcat(newArgument,p->pw_dir);
        	 strcat(newArgument, "/");
        	 char* s=strstr(argument, "/");
             if(s!=NULL){
             	s++;
			 	strcat(newArgument, s);
			 }
		}
		
		_arguments[ _numberOfArguments ] = newArgument;
	}else
		_arguments[ _numberOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numberOfArguments + 1] = NULL;
	
	_numberOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numberOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_fileFlag=0;
	_shell=0;
	_subshell=0;
	_buffer=0;
}

void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numberOfAvailableSimpleCommands == _numberOfSimpleCommands ) {
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numberOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numberOfSimpleCommands ] = simpleCommand;
	_numberOfSimpleCommands++;
}

void
Command:: clear()
{
	//printf("_numberOfSimpleCommands:%d, _simpleCommands[ 0 ]->_numberOfArguments: %d\n",_numberOfSimpleCommands,_simpleCommands[ 0 ]->_numberOfArguments);
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numberOfArguments; j ++ ) {
				free ( _simpleCommands[ i ]->_arguments[ j ] );
		}

		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inputFile ) {
		free( _inputFile );
	}

	if ( _errFile ) {
		free( _errFile );
	}
	
	if	( _shell ){
		free( _shell );
	}

	if  ( _buffer ){
		free(_buffer);
	}
	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_fileFlag=0;
	_subshell=0;
}

void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
		printf("\n");
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inputFile?_inputFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}

void
Command::forkSubshell()
{		
	char* cmd=strdup(_shell);
	char* ex=strdup("exit\n");
	char* arg[100];
	int x=0;
	char* c=strtok(cmd," ");
	while(c!=NULL){
		arg[x]=strdup(c);
		c=strtok(NULL," ");
		x++;
	}
	arg[x]=NULL;
	int spipe2[2];
	pipe(spipe2);
	
	int r=fork();
	if(r==0){
		dup2(spipe2[1],1);
		execvp(arg[0],arg);
		perror("execvp");
		_exit(1);
	}
 	close(spipe2[1]);
 	int i=0;
	char buffer[6144];
	read(spipe2[0],buffer+i,6144);
	_buffer=(char*)calloc(sizeof(char)*(strlen(buffer)+1),0);
 	strncpy(_buffer,buffer,strlen(buffer));
	
	
}

void
Command::execute()
{	

	// Don't do anything if there are no simple commands
	if ( _numberOfSimpleCommands == 0 ) {
		prompt();
		return;
	}
	// Print contents of Command data structure
	// 	print();

	// Add execution here
	// For every simple command fork a new process
	// Setup i/o redirection
	// and call exec

	int tmpin=dup(0);
	int tmpout=dup(1);
	int tmperr=dup(2);
	
	int fdin;
	if(_inputFile){
		fdin=open(_inputFile,O_RDONLY);
	}else{
		fdin=dup(tmpin);
	}

	
	if(_subshell){
		
		int r;
		for(int i=0; i<_numberOfSimpleCommands;i++){
		r=fork();
		if(r==0){
		
			if(!strcmp(_simpleCommands[i]->_arguments[0],"printenv")){
			char **p=environ;
			while(*p!=NULL){
				printf("%s\n",*p);
				p++;
			}
			exit(0);
			}

			execvp(_simpleCommands[i]->_arguments[0],_simpleCommands[i]->_arguments);
			perror("excvp");
			_exit(1);
		}	
		waitpid(r,NULL,0);
		}
		
	}else{
	int ret;
	int fdout;
	int fderr;
	for(int i=0; i<_numberOfSimpleCommands;i++){
		if(!strcmp(_simpleCommands[i]->_arguments[0],"exit")){
			fprintf(stdout,"Good bye!!\n");
			exit(1);
		}
	
		dup2(fdin,0);
		close(fdin);
		
		if(!strcmp(_simpleCommands[i]->_arguments[0],"cd")){
			if(_simpleCommands[i]->_arguments[1]!=NULL){
				if(chdir(_simpleCommands[i]->_arguments[1])==-1)
					printf("failed 1\n");
			}else{
				if(chdir(getenv("HOME"))==-1)
					printf("failed 2\n");
				}
				continue;
		}

		
		if(i == _numberOfSimpleCommands-1){
			if(_outFile){
				if(_fileFlag!=1&&_fileFlag!=2)
					fdout=open(_outFile,O_WRONLY|O_CREAT|O_TRUNC|O_APPEND, 0600);
				else{
					fdout=open(_outFile,O_WRONLY|O_APPEND, 0600);
				}
			}else{
				fdout=dup(tmpout);
			}
			
		}else if(!_subshell){
			int fdpipe[2];
			pipe(fdpipe);
			fdout=fdpipe[1];
			fdin=fdpipe[0];
		}

		dup2(fdout,1);
		if(_fileFlag==3||_fileFlag==2){
			dup2(fdout,2);
		}
		close(fdout);

		if(!strcmp(_simpleCommands[i]->_arguments[0],"setenv")){
			setenv(_simpleCommands[i]->_arguments[1],_simpleCommands[i]->_arguments[2],1);
 			continue;
		}else if(!strcmp(_simpleCommands[i]->_arguments[0],"unsetenv")){
			unsetenv(_simpleCommands[i]->_arguments[1]);
			continue;
		}

        	
		ret=fork();
		if(ret==0){
			if(!strcmp(_simpleCommands[i]->_arguments[0],"printenv")){
			char **p=environ;
			while(*p!=NULL){
				printf("%s\n",*p);
				p++;
			}
			exit(0);
			}

			execvp(_simpleCommands[i]->_arguments[0],_simpleCommands[i]->_arguments);
			perror("excvp");
			_exit(1);
		}	
			
		if(_errFile){
			printf("Ambiguous output redirect\n");
		}
		
		
	}
	
	dup2(tmpin,0);
	dup2(tmpout,1);
	dup2(tmperr,2);
	close(tmpin);
	close(tmpout);
	close(tmperr);
	
	if(!_background){
 		waitpid(ret,NULL,0);
	}
	}
	// Clear to prepare for next command
	clear();
	
	// Print new prompt
	prompt();
}

// Shell implementation

void
Command::prompt()
{	
	if ( isatty(0) ) {
 		printf("myshell>");
		fflush(stdout);
	}
	
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

extern "C" void killzombie( int sig )
{
	while(waitpid(-1, NULL, WNOHANG)>0);
		
}


int yyparse(void);

main()
{
	struct sigaction signalAction;
	signalAction.sa_handler=SIG_IGN;
	sigemptyset(&signalAction.sa_mask);
	signalAction.sa_flags=SA_RESTART;
	int error=sigaction(SIGINT, &signalAction, NULL);
	if(error){
		perror("sigaction");
		exit(-1);
	}

    struct sigaction signalAction1; 
  	signalAction1.sa_handler = killzombie; 
    sigemptyset(&signalAction1.sa_mask); 
    signalAction1.sa_flags = SA_RESTART;
    int error1 = sigaction(SIGCHLD, &signalAction1, NULL ); 
    if ( error1 ){
    	perror( "sigaction" );
    	exit( -1 ); 
    }

	Command::_currentCommand.prompt();
	yyparse();
}

