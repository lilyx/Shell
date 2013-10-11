/*
 * CS354: Operating Systems. 
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>


#define MAX_BUFFER_LINE 2048
int shift=0;
int tab=0;
char * prompt = "myshell>";
// Buffer where line is stored
int line_length;
char line_buffer[MAX_BUFFER_LINE];

// Simple history array
// This history does not change. 
// Yours have to be updated.
typedef struct _History{
	struct _History *next;
	struct _History *previous;
	struct _History *tail;
	struct _History *head;
	char* history;
}History;
History * head;
History * current;
void insert(char*);
void printHistory();

int notNew=0;

void read_line_print_usage()
{
  char * usage = "\n"
    	" ctrl-?        	      	      Print usage\n"
    	" Backspace(ctrl-H)    	      Deletes last character\n"
    	" Up arrow      	      	      See last command in the history\n"
	" Down arrow		      See next command in the history\n"
	" Left arrow		      Move cursor to the left\n"
	" Right arrow		      Move cursor to the right\n"
	" Delete key(ctrl-D)	      Remove the character at the position before cursor\n"
	" Home key(ctrl-A)	      The cursor move to the beginning of the line\n"
	" End key(ctrl-E)	      The cursor moves to the end of the line\n"
	" Autocompletement(<tab>)      Auto match the filename if there exist one\n"
	;
  write(1, usage, strlen(usage));
}

void sortArrayStrings(char** array, int nEntries){
	int swap;
	do{
		swap=0;
		int i;
		for(i=0;i<nEntries-1;i++){
			if(strcmp(array[i],array[i+1])>0){
				char* s=array[i];
				array[i]=array[i+1];
				array[i+1]=s;
				swap=1;
			}
		}
	}while(swap!=0);
}

/* 
 * Input a line with some basic editing.
 */
char * read_line() {

  // Set terminal in raw mode
  tty_raw_mode();

  line_length = 0;

  // Read one line until enter is typed
  while (1) {

    // Read one character in raw mode.
    char ch;
    read(0, &ch, 1);

    if (ch>=32&&ch!=127) {
      // It is a printable character. 

      // Do echo
      if(shift==0)
	 	write(1,&ch,1);

      // If max number of character reached return.
      if (line_length==MAX_BUFFER_LINE-2) break; 

      // add char to buffer.
      if(shift==0){
   	  	 line_buffer[line_length]=ch;
    	 line_length++;
    	}else{
       	  int s=shift;
       	  char c;
       	  for(;s!=1;s++){
       	  	c=line_buffer[line_length+s];
       	  	write(1,&ch,1);
       	  	line_buffer[line_length+s]=ch;
       	  	ch=c;
       	  }
       	  line_length++;
       	  s=shift;
       	  ch=8;
       	  for(;s!=0;s++)
       	  	write(1,&ch,1);
        }
      
    }
    else if (ch==1){
	//ctrl-A
		while(line_length+shift>0){
	  	 	ch=8;
	  	 	write(1,&ch,1);
	  	 	shift--;
	  	 }
    }
    else if (ch==4){
    //ctrl-D
    	if(line_length>0&&shift<0){
    		int s=shift;
     		for(;s<0;s++){
     			char c;
     			if(s!=-1)
     				c=line_buffer[line_length+s+1];
     			else c=' ';
     			write(1,&c,1);
     			line_buffer[line_length+s]=c;
     		}
     		ch=' ';
     		write(1,&ch,1);
     		line_buffer[line_length]=ch;
     		s=shift-1;
     		ch=8;
     		for(;s<0;s++){
     			write(1,&ch,1);
     		}
     		line_length--;
			shift++;
    	}else if(line_length==0){
    		strcpy(line_buffer,"exit");
    		line_length=4;
    		break;
    	}else{
    		ch=10;
    		write(1,&ch,1);
    		write(1,prompt,strlen(prompt));
    		write(1, line_buffer, line_length);
    		notNew=1;
    	}
    }
    else if (ch==5){
    //ctrl-E
    	while(shift!=0){
	  		ch=line_buffer[line_length+shift];
	  		write(1,&ch,1);
	  		shift++;
	  	}
    }
    else if (ch==9){
    //<tab>
    tab++;
	if(line_length!=0){
		line_buffer[line_length]=0;
		int status;
		struct stat st_buf;
		
		int i=line_length-1;	
		while(line_buffer[i]!=' '){
			i--;		
		}
		i++;
		
		char *filename=(char*)calloc(sizeof(char)*line_length,0);
		strcpy(filename,line_buffer+i);
 		filename[strlen(filename)]=0;
				
		char *filelist[1024];
		char* slash=strstr(filename,"/");
		char* d=(char*)calloc(strlen(filename),0);
		
		if(slash!=(filename+strlen(filename)-1)){
			if(slash!=NULL){
				i=strlen(filename)-1;
				while(filename[i]!='/'){
					i--;
				}
				strncpy(d,filename,i+1);
				d[i+1]=0;
				int length=strlen(filename);
				filename=strdup(filename+i+1);
				filename[length-i]=0;
			}else 
				d=strdup(".");
			
			DIR* dir=opendir(d);
			struct dirent * ent;
			int j=0;
			while((ent=readdir(dir))!=NULL){
				if((strlen(ent->d_name)>=strlen(filename))&&(strncmp(ent->d_name,filename,strlen(filename))==0)){
					filelist[j]=(char*)calloc(sizeof(ent->d_name)+1,0);
					strcpy(filelist[j],ent->d_name);
					j++;
				}
			}
			closedir(dir);
		
		if(j==1){
			i=strlen(filename);
			while(i<strlen(filelist[0])){
				char c=filelist[0][i];
				write(1,&c,1);
				line_buffer[line_length]=c;
				line_length++;	
				i++;	
			}
			status=stat(filelist[0],&st_buf);
			if(S_ISDIR(st_buf.st_mode)&&filelist[0][strlen(filelist[0])-1]!='/'){
				char c='/';
				write(1,&c,1);
				line_buffer[line_length]=c;
				line_length++;	
			}
			tab=0;
		}else if(j>0&&tab>1){
			sortArrayStrings(filelist,j);
			int x=0;
			ch=10;
			write(1,&ch,1);
			while(x<j){				
				write(1,filelist[x],strlen(filelist[x]));
				status=stat(filelist[x],&st_buf);
				if(S_ISDIR(st_buf.st_mode)){
					char c='/';
					write(1,&c,1);
				}
				if(x!=j-1){
					ch=' ';
					write(1,&ch,1);}
				x++;
			}
			ch=10;
			write(1,&ch,1);
			write(1,prompt,strlen(prompt));
			write(1,line_buffer,line_length);

		}
	}else if(tab>1){
			struct stat st_buf1;
			strcpy(d,filename);
			DIR* dir=opendir(d);
			struct dirent * ent;
			int j=0;
			while((ent=readdir(dir))!=NULL){
				if(ent->d_name[0]=='.')
					continue;
				filelist[j]=(char*)calloc(sizeof(ent->d_name)+1,0);
				strcpy(filelist[j],ent->d_name);
				j++;	
			}
			sortArrayStrings(filelist,j);
			int x=0;
			ch=10;
			write(1,&ch,1);
			while(x<j){				
				write(1,filelist[x],strlen(filelist[x]));
				status=stat(filelist[x],&st_buf1);
				if(S_ISDIR(st_buf1.st_mode)){
					char c='/';
					write(1,&c,1);
				}
				ch=' ';
				write(1,&ch,1);
				x++;
			}
			ch=10;
			write(1,&ch,1);
			write(1,prompt,strlen(prompt));
			write(1,line_buffer,line_length);
		}
	}
    }
    else if (ch==10) {
      // <Enter> was typed. Return line
      
      // Print newline
      write(1,&ch,1);

      break;
    }
    else if (ch == 31) {
      // ctrl-?
      read_line_print_usage();
      line_buffer[0]=0;
      break;
    }
    else if (ch == 8||ch==127) {
     // <backspace> was typed. Remove previous character read.
     if(line_length+shift>0){
     	// Go back one character
    	ch = 8;
    	write(1,&ch,1);
     	
     	if(shift==0){

    		// Write a space to erase the last character read
    		ch = ' ';
    		write(1,&ch,1);
		
    		// Go back one character
    		ch = 8;
    		write(1,&ch,1);
    		
    		// Remove one character from buffer
    		line_buffer[line_length]=0;
     	}else{
     		int s=shift;
     		for(;s<0;s++){
     			char c=line_buffer[line_length+s];
     			write(1,&c,1);
     			line_buffer[line_length+s-1]=c;
     		}
     		ch=' ';
     		write(1,&ch,1);
     		line_buffer[line_length-1]=ch;
     		s=shift-1;
     		ch=8;
     		for(;s<0;s++){
     			write(1,&ch,1);
     		}
     	}
     	line_length--;
     }
    }
    else if (ch==27) {
      // Escape sequence. Read two chars more
      //
      // HINT: Use the program "keyboard-example" to
      // see the ascii code for the different chars typed.
      //
      char ch1; 
      char ch2;
      read(0, &ch1, 1);
      read(0, &ch2, 1);
      if (ch1==91 && ch2==65) {
		// Up arrow. Print next line in history.
		if(current!=NULL){
		 	// Up arrow. Print next line in history.

			// Erase old line
			// Print backspaces
			int i = 0;
			for (; i < line_length; i++) {
			  ch = 8;
			  write(1,&ch,1);
			}
			
			// Print spaces on top
			for (i =0; i < line_length; i++) {
				ch = ' ';
	  			write(1,&ch,1);
			}

			// Print backspaces
			for (i =0; i < line_length; i++) {
	  			ch = 8;
	  			write(1,&ch,1);
			}	
	
			// Copy line from history
			strcpy(line_buffer, current->history);
			line_length = strlen(line_buffer)-1;
			line_buffer[line_length]=0;
			if(current->previous)
				current=current->previous;
			// echo line
    		write(1, line_buffer, line_length);
	  	}
	 }
	  if (ch1==91 && ch2==66){
	   //Down arrow
	   	if(current!=NULL){
	  		// Erase old line
			// Print backspaces
			int i = 0;
			for (; i < line_length; i++) {
			  ch = 8;
			  write(1,&ch,1);
			}
			
			// Print spaces on top
			for (i =0; i < line_length; i++) {
				ch = ' ';
	  			write(1,&ch,1);
			}

			// Print backspaces
			for (i =0; i < line_length; i++) {
	  			ch = 8;
	  			write(1,&ch,1);
			}
			
			if(current->next){
				current=current->next;
				// Copy line from history
				strcpy(line_buffer, current->history);
				line_length = strlen(line_buffer)-1;
				line_buffer[line_length]=0;
			}else{
				line_length=0;
				line_buffer[line_length]=0;
			}
	 	
			// echo line
    		write(1, line_buffer, line_length);
		}				
	  }
	  if (ch1==91 && ch2==67){
	  //right arrow
	  	if(shift!=0){
	  		ch=line_buffer[line_length+shift];
	  		write(1,&ch,1);
	  		shift++;
	  	}
	  }
	  if (ch1==91 && ch2==68){
	  //left arrow
	  	 if(line_length+shift>0){
	  	 	ch=8;
	  	 	write(1,&ch,1);
	  	 	shift--;
	  	 }
	  }
	}

  }

  // Add eol and null char at the end of string
  line_buffer[line_length]=10;
  line_length++;
  line_buffer[line_length]=0;

  if(line_length!=1&&notNew!=1){
	insert(strdup(line_buffer));
  }
  shift=0;
  tab=0;
  return line_buffer;
}

void insert(char* his){
	if(head==NULL){
		head=(History*)malloc(sizeof(History));
		head->history=strdup(his);
		head->head=head;
		head->tail=head;
		head->previous=NULL;
		head->next=NULL;
		current=head;
		return;
	}
	History* newHis=(History*)malloc(sizeof(History));
	newHis->history=strdup(his);
	newHis->head=head;
	newHis->tail=newHis;
	newHis->previous=head->tail;
	newHis->next=NULL;
	head->tail->next=newHis;
	head->tail=head->tail->next;
	current=newHis;
}

void printHistory(){
	History* p=head;
	while(p){
		printf("history: %s\n", p->history);
		p=p->next;
	}
}
