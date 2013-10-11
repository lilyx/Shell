
/*
 * CS-252 Spring 2013
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%token    <string_val> WORD SHELLWORD

%token 	NOTOKEN GREAT NEWLINE GREATGREAT DPIPE AMPERSAND GREATGREATAMPERSAND PIPE GREATAMPERSAND LESS

%union	{
		char   *string_val;
	}

%{
//#define yylex yylex
#define MAXFILENAME 1024
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <regex.h>
#include <dirent.h>
#include <assert.h>
#include "command.h"

int maxEntries = 20;
int nEntries = 0;
char** array = (char**)malloc(maxEntries*sizeof(char*));
void yyerror(const char * s);
int yylex();
void myunputc(int c);
void sortArrayStrings(char** array, int nEntries);
void expandWildcard(char* prefix, char* suffix);
%}

%%

goal:	
	command_list
	;
	
command_list:
	command_line
	|command_list command_line
	;
	
command_line:
	pipe_list io_modifier_list background_optional NEWLINE{
		Command::_currentCommand.execute();
	}
	| NEWLINE 
	|error NEWLINE{yyerrok;}
	;
	
background_optional:
	 AMPERSAND{
		Command::_currentCommand._background = 1;
	}
	| /*can be empty*/
	;

io_modifier_list:
	io_modifier_list iomodifier_opt
	| /*can be empty*/
	;

iomodifier_opt:
	GREAT WORD {
		if(Command::_currentCommand._outFile==0)
			Command::_currentCommand._outFile = $2;
		else Command::_currentCommand._errFile=$2;
		Command::_currentCommand._fileFlag=0;
	}
	| GREATGREAT WORD {
		if(Command::_currentCommand._outFile==0)
			Command::_currentCommand._outFile = $2;
		else Command::_currentCommand._errFile=$2;
		Command::_currentCommand._fileFlag=1;
	}
	| GREATGREATAMPERSAND WORD{
		if(Command::_currentCommand._outFile==0)
			Command::_currentCommand._outFile = $2;
		else Command::_currentCommand._errFile=$2;
		Command::_currentCommand._fileFlag=2;
	}
	| GREATAMPERSAND WORD{
		if(Command::_currentCommand._outFile==0)
			Command::_currentCommand._outFile = $2;
		else Command::_currentCommand._errFile=$2;
		Command::_currentCommand._fileFlag=3;
	}
	| LESS WORD{
		if(Command::_currentCommand._inputFile==0)
			Command::_currentCommand._inputFile = $2;
		else Command::_currentCommand._errFile=$2;
	}
	;
	

pipe_list:
	pipe_list PIPE command_and_args
	| pipe_list DPIPE command_and_args
	| command_and_args
	;

command_and_args:
	command_word arg_list {
		Command::_currentCommand.
			insertSimpleCommand( Command::_currentSimpleCommand );
	}
	;
	
command_word:
	WORD {
		  
           Command::_currentSimpleCommand = new SimpleCommand();
	       Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;

arg_list:
	arg_list argument
	| /* can be empty */
	;

argument:
	WORD {
		 char* arg = $1;
         char* temparg;
         if(strchr(arg, '\\') == NULL){
            temparg = strdup(arg);
          }else{
            temparg = (char*)calloc(strlen(arg),0);
            int j=0;
            int k=0;
            while(arg[j]){
                if(arg[j] != '\\'){
                    temparg[k] = arg[j];
                    k++;
                }else if(arg[j] == '\\' && arg[j+1] == '\\'){
                    temparg[k] = '\\';
                    k++;
                    j++;
                }
                j++;
            }
        }
		expandWildcard(NULL, temparg);
		for(int i=0;i<nEntries;i++){
			Command::_currentSimpleCommand->insertArgument(array[i]);
		}
		free(array);
		array=(char**)malloc(maxEntries*sizeof(char*));
		nEntries = 0;

	}
	|SHELLWORD{
		Command::_currentCommand._shell = strdup($1);
        Command::_currentCommand._subshell = 1;
		Command::_currentCommand.forkSubshell();
		
		char* buffer=strdup(Command::_currentCommand._buffer);
		int len=strlen(buffer);
		int b=0;
		int x=len-1;
		while(x>=0){
			
			if(buffer[x]=='\n'){
				b++;
			}
			if(b==2){
				int n=Command::_currentSimpleCommand->_numberOfArguments;
				char* cmd[n];
				int j=n-1;
				while(j>=0){
					myunputc(' ');
					cmd[j]=(char*)calloc(strlen(Command::_currentSimpleCommand->_arguments[j])+1,0);
					strcpy(cmd[j],Command::_currentSimpleCommand->_arguments[j]);
					int y=strlen(cmd[j])-1;
					while(y>=0){
						myunputc(cmd[j][y]);
						y--;
					}
					j--;
				}
				Command::_currentCommand._subshell += 1;
				b--;
				myunputc('|');
				myunputc('|');
				x--;
				continue;
			}
			myunputc(buffer[x]);
			x--;
		}
		
	}
	;

%%
void sortArrayStrings(char** array, int nEntries){
	int swap;
	do{
		swap=0;
		for(int i=0;i<nEntries-1;i++){
			if(strcmp(array[i],array[i+1])>0){
				char* s=array[i];
				array[i]=array[i+1];
				array[i+1]=s;
				swap=1;
			}
		}
	}while(swap!=0);
}

void add(char* arg){
	if(nEntries == maxEntries){
		maxEntries *= 2;
		array=(char**)realloc(array, maxEntries*sizeof(char*));
		assert(array!=NULL);
	}
	
	array[nEntries]=strdup(arg);
	nEntries++;
	sortArrayStrings(array, nEntries);
}

void expandWildcard(char* prefix, char* suffix){
	if(suffix[0]==0){
		add(prefix);
		return;
		
	}
	// Obtain the next component in the suffix
    // Also advance suffix.
    char* s=strchr(suffix,'/');
    char component[MAXFILENAME];
    if(s!=NULL){
    	if(s==suffix)
    		sprintf(component, "/");
    	else
    		strncpy(component,suffix,s-suffix);
    	suffix=s+1;
    }else{
    	strcpy(component,suffix);
    	suffix=suffix+strlen(suffix);
    }
    char newPrefix[MAXFILENAME];
    if(strchr(component, '*')==NULL&&strchr(component, '?')==NULL){
    	if(prefix!=NULL){
    		if(prefix[0]=='/'&&strlen(prefix)==1)
    			sprintf(newPrefix, "/%s", component);
			else
				sprintf(newPrefix, "%s/%s", prefix, component);		
    		}
    	else sprintf(newPrefix, "%s", component);
    	expandWildcard(newPrefix,suffix);
    	return;
    }
    char* reg=(char*)malloc(2*strlen(component)+10);
  	char* a=component;
  	char* r=reg;
  	*r='^';r++;
  	while(*a){
  		if(*a=='*'){*r='.';r++;*r='*';r++;}
  		else if(*a=='?'){*r='.';r++;}
  		else if(*a == '.'){*r='\\';r++;*r='.';r++;}
  		else {*r=*a;r++;}
  		a++;
  	}
  	*r='$';r++;*r=0;
    
    regex_t re;
  	int expbuf=regcomp(&re,reg, REG_EXTENDED|REG_NOSUB);
  	if(expbuf!=0){
  		perror("compile");
  		return;
  	}

  	char * dir;
  	//if prefix is empty then list current directory
  	if(prefix==NULL){  		        		
		dir=(char*)calloc(sizeof(char),0);
		*dir='.';
	}
	else dir=prefix;
	
	DIR * d=opendir(dir);
	if(d==NULL){
		//fprintf(stderr,"dir=%s",dir);
		//perror("opendir");
		return;
	}
	struct dirent * ent;
	
	while((ent=readdir(d))!=NULL){
		//check if name matches
		regmatch_t match;
		if(regexec(&re,ent->d_name,1,&match,0)==0){
		// Entry matches. Add name of entry 
        	// that matches to the prefix and
        	// call expandWildcard(..) recursively
        	if(ent->d_name[0]=='.'){
				if(component[0]=='.'){
        			if(prefix==NULL)
						sprintf(newPrefix, "%s", ent->d_name);	
					else
						sprintf(newPrefix, "%s/%s", prefix, ent->d_name);
				}	
				else continue;			
			}else{	
 		       	if(prefix!=NULL){
					if(prefix[0]=='/'&&strlen(prefix)==1)
    					sprintf(newPrefix, "/%s",ent->d_name);
					else
						sprintf(newPrefix, "%s/%s", prefix, ent->d_name);	
				}	
				else sprintf(newPrefix, "%s",  ent->d_name);
			}
			expandWildcard(newPrefix,suffix);
		}
	}
	
	closedir(d);
	
}

void
yyerror(const char * s)
{
//	fprintf(stderr,"%s", s);
}

#if 0
main()
{
	yyparse();
}
#endif
