/*      $Id: lircd.c,v 1.1.1.1 1999/04/29 21:16:58 columbus Exp $      */

/****************************************************************************
 ** lircd.c *****************************************************************
 ****************************************************************************
 *
 * lircd - LIRC Decoder Daemon
 * 
 * Copyright (C) 1996,97 Ralph Metzler <rjkm@thp.uni-koeln.de>
 * Copyright (C) 1998,99 Christoph Bartelmus <columbus@hit.handshake.de>
 *
 *  =======
 *  HISTORY
 *  =======
 *
 * 0.1:  03/27/96  decode SONY infra-red signals
 *                 create mousesystems mouse signals on pipe /dev/lircm
 *       04/07/96  send ir-codes to clients via socket (see irpty)
 *       05/16/96  now using ir_remotes for decoding
 *                 much easier now to describe new remotes
 *
 * 0.5:  09/02/98 finished (nearly) complete rewrite (Christoph)
 *
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define __USE_BSD

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#ifdef __GLIBC__
#include <sys/perm.h>
#endif
#include <errno.h>

#include "lircd.h"
#include "config_file.h"

char *progname="lircd-"VERSION;
char *logfile=LOGFILE;

struct protocol_directive directives[] =
{
	{"LIST",list},
	{"SEND_ONCE",send_once},
	{"SEND_START",send_start},
	{"SEND_STOP",send_stop},
	{"VERSION",version},
	{NULL,NULL}
	/*
	{"DEBUG",debug},
	{"DEBUG_LEVEL",debug_level},
	*/
};

enum protocol_string_num {
	P_BEGIN=0,
	P_DATA,
	P_END,
	P_ERROR,
	P_SUCCESS,
	P_SIGHUP
};

char *protocol_string[] = 
{
	"BEGIN\n",
	"DATA\n",
	"END\n",
	"ERROR\n",
	"SUCCESS\n",
	"SIGHUP\n"
};

#define HOSTNAME_LEN 128
char hostname[HOSTNAME_LEN+1];

FILE *lf=NULL;
int lirc,sockfd;
int clis[FD_SETSIZE-2]; /* substract one for each lirc and sockfd */
int clin=0;

inline int max(int a,int b)
{
	return(a>b ? a:b);
}

/* A safer write(), since sockets might not write all but only some of the
   bytes requested */

inline int write_socket(int fd, char *buf, int len)
{
	int done,todo=len;

	while(todo)
	{
		done=write(fd,buf,todo);
		if (done<=0) return(done);    
		buf+=done;
		todo-=done;
	}
	return(len-todo);
}

inline int write_socket_len(int fd, char *buf)
{
	int len;

	len=strlen(buf);
	if(write_socket(fd,buf,len)<len) return(0);
	return(1);
}

void sigterm(int sig)
{
	int i;
	
	signal(SIGALRM,SIG_IGN);
	
	if(free_remotes!=NULL)
	{
		free_config(free_remotes);
	}
	free_config(remotes);
	logprintf("caught signal\n");
	for (i=0; i<clin; i++)
	{
		shutdown(clis[i],2);
		close(clis[i]);
	};
	shutdown(sockfd,2);
	close(sockfd);
	close(lirc);
	if(lf) fclose(lf);
	signal(sig,SIG_DFL);
	raise(sig);
}

void sighup(int sig)
{
	struct stat s;
	int i;

	/* reopen logfile first */
	logprintf("closing logfile\n");
	if(-1==fstat(fileno(lf),&s))		
	{
		exit(EXIT_FAILURE); /* shouldn't ever happen */
	}
	fclose(lf);
	lf=fopen(logfile,"a");
	if(lf==NULL)
	{
		/* can't print any error messagees */
		exit(EXIT_FAILURE);
	}
	logprintf("reopened logfile\n");
	if(-1==fchmod(fileno(lf),s.st_mode))
	{
		logprintf("WARNING: could not set file permissions\n");
		logperror(NULL);
	}

	config();
	
	for (i=0; i<clin; i++)
	{
		if(!(write_socket_len(clis[i],protocol_string[P_BEGIN]) &&
		     write_socket_len(clis[i],protocol_string[P_SIGHUP]) &&
		     write_socket_len(clis[i],protocol_string[P_END])))
		{
			remove_client(clis[i]);
			i--;
		}
	}
}

void config(void)
{
	FILE *fd;
	struct ir_remote *config_remotes;
	
	if(free_remotes!=NULL)
	{
		logprintf("cannot read config file\n");
		logprintf("old config is still in use\n");
		return;
	}
	fd=fopen(LIRCDCFGFILE,"r");
	if(fd==NULL)
	{
		logprintf("could not open config file\n");
		logperror(NULL);
		return;
	}
	config_remotes=read_config(fd);
	fclose(fd);
	if(config_remotes==(void *) -1)
	{
		logprintf("reading of config file failed\n");
	}
	else
	{
		logprintf("config file read\n");
		if(config_remotes==NULL)
		{
			logprintf("warning: config file contains no valid remote control definition\n");
		}
		/* I cannot free the data structure
		   as they could still be in use */
		free_remotes=remotes;
		remotes=config_remotes;
		if(remotes==NULL)
		{
			logprintf("warning: no valid remote control defined\n");
		}
	}
}

void nolinger(int sock)
{
	static struct linger  linger = {0, 0};
	int lsize  = sizeof(struct linger);
	setsockopt(sock, SOL_SOCKET, SO_LINGER, (void *)&linger, lsize);
}

void remove_client(int fd)
{
	int i;

	for(i=0;i<clin;i++)
	{
		if(clis[i]==fd)
		{
			shutdown(clis[i],2);
			close(clis[i]);
			logprintf("removed client\n");
			
			clin--;
			for(;i<clin;i++)
			{
				clis[i]=clis[i+1];
			}
			return;
		}
	}
#       ifdef DEBUG
	logprintf("internal error in remove_client: no such fd\n");
#       endif
}

void add_client(void)
{
	int fd;
	int clilen;
	struct sockaddr_un client_addr;

	clilen=sizeof(client_addr);
	fd=accept(sockfd,(struct sockaddr *)&client_addr,&clilen);
	if(fd==-1) 
	{
		logprintf("accept() failed\n");
		logperror(NULL);
		exit(EXIT_FAILURE);
	};

	if(fd>=FD_SETSIZE)
	{
		logprintf("connection rejected\n");
		logperror(NULL);
		shutdown(fd,2);
		close(fd);
		return;
	}
	nolinger(fd);
	clis[clin++]=fd;

	logprintf("accepted new client\n");
}

void start_server(void)
{
	struct sockaddr_un serv_addr;
	struct stat s;
	int ret;

	sockfd=socket(AF_UNIX,SOCK_STREAM,0);
	if(sockfd==-1)
	{
		fprintf(stderr,"%s: could not create socket\n",progname);
		perror(progname);
		exit(EXIT_FAILURE);
	};

	/* 
	   get owner, permissions, etc.
	   so new socket can be the same since we
	   have to delete the old socket.  
	*/
	ret=stat(LIRCD,&s);
	if(ret==-1)
	{
		fprintf(stderr,"%s: could not get file information for %s\n",
			progname,LIRCD);
		perror(progname);
		exit(EXIT_FAILURE);
	}
	ret=unlink(LIRCD);
	if(ret==-1)
	{
		fprintf(stderr,"%s: could not delete %s\n",progname,LIRCD);
		perror(NULL);
		exit(EXIT_FAILURE);
	}

	serv_addr.sun_family=AF_UNIX;
	strcpy(serv_addr.sun_path,LIRCD);
	if(bind(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr))==-1)
	{
		fprintf(stderr,"%s; could not assign address to socket\n",
			progname);
		perror(progname);
		exit(EXIT_FAILURE);
	};

	if(chmod(LIRCD,s.st_mode)==-1 || chown(LIRCD,s.st_uid,s.st_gid)==-1)
	{
		fprintf(stderr,"%s: could not set file permissions\n",progname);
		perror(progname);
		exit(EXIT_FAILURE);
	};

	listen(sockfd,3);
	nolinger(sockfd);
  
	lf=fopen(logfile,"a");
	if(lf==NULL)
	{
		fprintf(stderr,"%s: could not open logfile\n",progname);
		perror(progname);
		exit(EXIT_FAILURE);
	}
	gethostname(hostname,HOSTNAME_LEN);
	logprintf("started server socket\n");
}

void logprintf(char *format_str, ...)
{
	time_t current;
	char *currents;
	va_list ap;  

	current=time(&current);
	currents=ctime(&current);
		    
	fprintf(lf,"%15.15s %s %s: ",currents+4,hostname,progname);
	va_start(ap,format_str);
	vfprintf(lf,format_str,ap);
#ifndef DAEMONIZE
	vfprintf(stderr,format_str,ap);
#endif DAEMONIZE
	va_end(ap);
	fflush(lf);
}

void logperror(const char *s)
{
	if(s!=NULL)
	{
		logprintf("%s: %s\n",s,strerror(errno));
	}
	else
	{
		logprintf("%s\n",strerror(errno));
	}
}

#ifdef DAEMONIZE

void daemonize(void)
{
	pid_t pid;
	
	if((pid=fork())<0)
{
		fprintf(stderr,"%s: fork() failed\n",progname);
		perror(progname);
		exit(EXIT_FAILURE);
	}
	else if(pid) /* parent */
	{
		exit(EXIT_SUCCESS);
	}
	else
	{
		fclose(stdin);
		fclose(stdout);
		fclose(stderr);
		setsid();
		chdir("/");
		umask(0);
	}
}

#endif DAEMONIZE

void sigalrm(int sig)
{
	struct itimerval repeat_timer;

	if(repeat_remote->last_code!=repeat_code)
	{
		/* we received a different code from the original
		   remote control
		   we could repeat the wrong code
		   so better stop repeating */
		return;
	}
	send_command(repeat_remote,repeat_code);

	repeat_timer.it_value.tv_sec=0;
	repeat_timer.it_value.tv_usec=repeat_remote->remaining_gap;
	repeat_timer.it_interval.tv_sec=0;
	repeat_timer.it_interval.tv_usec=0;

	setitimer(ITIMER_REAL,&repeat_timer,NULL);
}

int parse_rc(int fd,char *message,char *arguments,struct ir_remote **remote,
	     struct ir_ncode **code,int n)
{
	char *name=NULL,*command=NULL;

	*remote=NULL;
	*code=NULL;
	if(arguments==NULL) return(1);

	name=strtok(arguments,WHITE_SPACE);
	if(name==NULL) return(1);
	*remote=get_ir_remote(name);
	if(*remote==NULL)
	{
		return(send_error(fd,message,"unknown remote: \"%s\"\n",
				  name));
	}
	command=strtok(NULL,WHITE_SPACE);
	if(command==NULL) return(1);
	*code=get_ir_code(*remote,command);
	if(*code==NULL)
	{
		return(send_error(fd,message,"unknown command: \"%s\"\n",
				  command));
	}
	if(strtok(NULL,WHITE_SPACE)!=NULL)
	{
		return(send_error(fd,message,"bad send packet\n"));
	}
	if(n>0 && *remote==NULL)
	{
		return(send_error(fd,message,"remote missing\n"));
	}
	if(n>1 && *code==NULL)
	{
		return(send_error(fd,message,"code missing\n"));
	}
	return(1);
}

int send_success(int fd,char *message)
{
	if(!(write_socket_len(fd,protocol_string[P_BEGIN]) &&
	     write_socket_len(fd,message) &&
	     write_socket_len(fd,protocol_string[P_SUCCESS]) &&
	     write_socket_len(fd,protocol_string[P_END]))) return(0);
	return(1);
}

int send_error(int fd,char *message,char *format_str, ...)
{
	char lines[4],buffer[PACKET_SIZE+1];
	int i,n,len;
	va_list ap;  
	
	va_start(ap,format_str);
	vsprintf(buffer,format_str,ap);
	va_end(ap);
	
	logprintf("error processing command: %s",message);
	logprintf("%s",buffer);

	n=0;
	len=strlen(buffer);
	for(i=0;i<len;i++) if(buffer[i]=='\n') n++;
	sprintf(lines,"%d\n",n);
	
	if(!(write_socket_len(fd,protocol_string[P_BEGIN]) &&
	     write_socket_len(fd,message) &&
	     write_socket_len(fd,protocol_string[P_ERROR]) &&
	     write_socket_len(fd,protocol_string[P_DATA]) &&
	     write_socket_len(fd,lines) &&
	     write_socket_len(fd,buffer) &&
	     write_socket_len(fd,protocol_string[P_END]))) return(0);
	return(1);
}

int send_remote_list(int fd,char *message)
{
	char buffer[PACKET_SIZE+1];
	struct ir_remote *all;
	int n,len;
	
	n=0;
	all=remotes;
	while(all)
	{
		n++;
		all=all->next;
	}

	if(!(write_socket_len(fd,protocol_string[P_BEGIN]) &&
	     write_socket_len(fd,message) &&
	     write_socket_len(fd,protocol_string[P_SUCCESS]))) return(0);
	
	if(n==0)
	{
		return(write_socket_len(fd,protocol_string[P_END]));
	}
	sprintf(buffer,"%d\n",n);
	len=strlen(buffer);
	if(!(write_socket_len(fd,protocol_string[P_DATA]) &&
	     write_socket_len(fd,buffer))) return(0);

	all=remotes;
	while(all)
	{
		len=snprintf(buffer,PACKET_SIZE+1,"%s\n",all->name);
		if(len==PACKET_SIZE+1)
		{
			len=sprintf(buffer,"name_too_long\n");
		}
		if(write_socket(fd,buffer,len)<len) return(0);
		all=all->next;
	}
	return(write_socket_len(fd,protocol_string[P_END]));
}

int send_remote(int fd,char *message,struct ir_remote *remote)
{
	struct ir_ncode *codes;
	char buffer[PACKET_SIZE+1];
	int n,len;

	n=0;
	codes=remote->codes;
	if(codes!=NULL)
	{
		while(codes->name!=NULL)
		{
			n++;
			codes++;
		}
	}

	if(!(write_socket_len(fd,protocol_string[P_BEGIN]) &&
	     write_socket_len(fd,message) &&
	     write_socket_len(fd,protocol_string[P_SUCCESS]))) return(0);
	if(n==0)
	{
		return(write_socket_len(fd,protocol_string[P_END]));
	}
	sprintf(buffer,"%d\n",n);
	if(!(write_socket_len(fd,protocol_string[P_DATA]) &&
	     write_socket_len(fd,buffer))) return(0);

	codes=remote->codes;
	while(codes->name!=NULL)
	{
#ifdef __GLIBC__
		/* It seems you can't print 64-bit longs on glibc */
		
		len=snprintf(buffer,PACKET_SIZE+1,"%08lx%08lx %s\n",
			     (unsigned long) (codes->code>>32),
			     (unsigned long) (codes->code&0xFFFFFFFF),
			     codes->name);
#else
		len=snprintf(buffer,PACKET_SIZE,"%016llx %s\n",
			     codes->code,
			     codes->name);
#endif
		if(len==PACKET_SIZE+1)
		{
			len=sprintf(buffer,"code_too_long\n");
		}
		if(write_socket(fd,buffer,len)<len) return(0);
		codes++;
	}
	return(write_socket_len(fd,protocol_string[P_END]));
}

int send_name(int fd,char *message,struct ir_ncode *code)
{
	char buffer[PACKET_SIZE+1];
	int len;

	if(!(write_socket_len(fd,protocol_string[P_BEGIN]) &&
	     write_socket_len(fd,message) &&
	     write_socket_len(fd,protocol_string[P_SUCCESS]))) return(0);
#ifdef __GLIBC__
	/* It seems you can't print 64-bit longs on glibc */
	
	len=snprintf(buffer,PACKET_SIZE+1,"1\n%08lx%08lx %s\n",
		     (unsigned long) (code->code>>32),
		     (unsigned long) (code->code&0xFFFFFFFF),
		     code->name);
#else
	len=snprintf(buffer,PACKET_SIZE,"1\n%016llx %s\n",
		     code->code,
		     code->name);
#endif
	if(len==PACKET_SIZE+1)
	{
		len=sprintf(buffer,"1\ncode_too_long\n");
	}
	if(write_socket(fd,buffer,len)<len) return(0);
	return(write_socket_len(fd,protocol_string[P_END]));
}

int list(int fd,char *message,char *arguments)
{
	struct ir_remote *remote;
	struct ir_ncode *code;

	if(parse_rc(fd,message,arguments,&remote,&code,0)==0) return(0);

	if(remote==NULL)
	{
		return(send_remote_list(fd,message));
	}
	if(code==NULL)
	{
		return(send_remote(fd,message,remote));
	}
	return(send_name(fd,message,code));
}

int send_once(int fd,char *message,char *arguments)
{
	struct ir_remote *remote;
	struct ir_ncode *code;
	
	if(parse_rc(fd,message,arguments,&remote,&code,2)==0) return(0);
	
	if(remote==NULL || code==NULL) return(1);
	if(remote==repeat_remote)
	{
		return(send_error(fd,message,"remote is repeating\n"));
	}
	if(remote->repeat_bit>0)
		remote->repeat_state=
		!remote->repeat_state;
	
	send_command(remote,code);
	return(send_success(fd,message));
}

int send_start(int fd,char *message,char *arguments)
{
	struct ir_remote *remote;
	struct ir_ncode *code;
	struct itimerval repeat_timer;
	
	if(parse_rc(fd,message,arguments,&remote,&code,2)==0) return(0);
	
	if(remote==NULL || code==NULL) return(1);
	if(repeat_remote!=NULL)
	{
		return(send_error(fd,message,"already repeating\n"));
	}
	
	if(remote->repeat_bit>0)
		remote->repeat_state=
		!remote->repeat_state;
	send_command(remote,code);
	repeat_remote=remote;
	repeat_code=code;
	
	repeat_timer.it_value.tv_sec=0;
	repeat_timer.it_value.tv_usec=
	remote->remaining_gap;
	repeat_timer.it_interval.tv_sec=0;
	repeat_timer.it_interval.tv_usec=0;
	setitimer(ITIMER_REAL,&repeat_timer,NULL);
	return(send_success(fd,message));
}

int send_stop(int fd,char *message,char *arguments)
{
	struct ir_remote *remote;
	struct ir_ncode *code;
	struct itimerval repeat_timer;
	
	if(parse_rc(fd,message,arguments,&remote,&code,2)==0) return(0);
	
	if(remote==NULL || code==NULL) return(1);
	if(repeat_remote && repeat_code &&
	   strcasecmp(remote->name,repeat_remote->name)==0 && 
	   strcasecmp(code->name,repeat_code->name)==0)
	{
		repeat_timer.it_value.tv_sec=0;
		repeat_timer.it_value.tv_usec=0;
		repeat_timer.it_interval.tv_sec=0;
		repeat_timer.it_interval.tv_usec=0;
		
		setitimer(ITIMER_REAL,&repeat_timer,NULL);
		
		repeat_remote=NULL;
		repeat_code=NULL;
		return(send_success(fd,message));
	}
	else
	{
		return(send_error(fd,message,"not repeating\n"));
	}
}

int version(int fd,char *message,char *arguments)
{
	char buffer[PACKET_SIZE+1];

	if(arguments!=NULL)
	{
		return(send_error(fd,message,"bad send packet\n"));
	}
	sprintf(buffer,"1\n%s\n",VERSION);
	if(!(write_socket_len(fd,protocol_string[P_BEGIN]) &&
	     write_socket_len(fd,message) &&
	     write_socket_len(fd,protocol_string[P_SUCCESS]) &&
	     write_socket_len(fd,protocol_string[P_DATA]) &&
	     write_socket_len(fd,buffer) &&
	     write_socket_len(fd,protocol_string[P_END]))) return(0);
	return(1);
}

int get_command(int fd)
{
	int length;
	char buffer[PACKET_SIZE+1],backup[PACKET_SIZE+1];
	char *end;
	int packet_length,i;
	char *directive;

	length=read(fd,buffer,PACKET_SIZE);
	packet_length=0;
	while(length>packet_length)
	{
		buffer[length]=0;
		end=strchr(buffer,'\n');
		if(end==NULL)
		{
			logprintf("bad send packet: \"%s\"\n",buffer);
			while(length>0)
			{
				length=read(fd,buffer,PACKET_SIZE);
			}
			return(1);
		}
		end[0]=0;
#               ifdef DEBUG
		logprintf("sending: \"%s\"\n",buffer);
#               endif		
		packet_length=strlen(buffer)+1;

		strcpy(backup,buffer);strcat(backup,"\n");
		directive=strtok(buffer,WHITE_SPACE);
		if(directive==NULL)
		{
			if(!send_error(fd,backup,"bad send packet\n",
				       directive))
				return(0);
			goto skip;
		}
		for(i=0;directives[i].name!=NULL;i++)
		{
			if(strcasecmp(directive,directives[i].name)==0)
			{
				if(!directives[i].
				   function(fd,backup,strtok(NULL,"")))
					return(0);
				goto skip;
			}
		}
		
		if(!send_error(fd,backup,"unknown directive: \"%s\"\n",
			       directive))
			return(0);
	skip:
		if(length>packet_length)
		{
			int new_length;

			memmove(buffer,buffer+packet_length,
				length-packet_length+1);
			if(strchr(buffer,'\n')==NULL)
			{
				new_length=read(fd,buffer+length-packet_length,
						PACKET_SIZE-(length-packet_length));
				if(new_length>0)
				{
					length=length-packet_length+new_length;
				}
				else
				{
					length=new_length;
				}
			}
			else
			{
				length-=packet_length;
			}
			packet_length=0;
		}
	}

	if(length==0) /* EOF: connection closed by client */
	{
		return(0);
	}
	else if(length==-1)
	{
		logprintf("read failed\n");
	}
	return(1);
}

void free_old_remotes()
{
	struct ir_remote *scan_remotes,*found;
	struct ir_ncode *code;

	if(last_remote!=NULL)
	{
		scan_remotes=free_remotes;
		while(scan_remotes!=NULL)
		{
			if(last_remote==scan_remotes)
			{
				found=get_ir_remote(last_remote->name);
				if(found!=NULL)
				{
					code=get_ir_code(found,last_remote->last_code->name);
					if(code!=NULL)
					{
						found->reps=last_remote->reps;
						found->repeat_state=last_remote->repeat_state;
						found->remaining_gap=last_remote->remaining_gap;
						last_remote=found;
						last_remote->last_code=code;
					}
				}
				break;
			}
			scan_remotes=scan_remotes->next;
		}
		if(scan_remotes==NULL) last_remote=NULL;
	}
	/* check if last config is still needed */
	found=NULL;
	if(repeat_remote!=NULL)
	{
		scan_remotes=free_remotes;
		while(scan_remotes!=NULL)
		{
			if(repeat_remote==scan_remotes)
			{
				found=repeat_remote;
				break;
			}
			scan_remotes=scan_remotes->next;
		}
		if(found!=NULL)
		{
			found=get_ir_remote(repeat_remote->name);
			if(found!=NULL)
			{
				code=get_ir_code(found,repeat_code->name);
				if(code!=NULL)
				{
					struct itimerval repeat_timer;

					repeat_timer.it_value.tv_sec=0;
					repeat_timer.it_value.tv_usec=0;
					repeat_timer.it_interval.tv_sec=0;
					repeat_timer.it_interval.tv_usec=0;

					found->last_code=code;
					found->last_send=repeat_remote->last_send;
					found->repeat_state=repeat_remote->repeat_state;
					found->remaining_gap=repeat_remote->remaining_gap;

					setitimer(ITIMER_REAL,&repeat_timer,&repeat_timer);
					/* "atomic" */
					repeat_remote=found;
					repeat_code=code;
					/* end "atomic" */
					setitimer(ITIMER_REAL,&repeat_timer,NULL);
					found=NULL;
				}
			}
			else
			{
				found=repeat_remote;
			}
		}
	}
	if(found==NULL && decoding!=free_remotes)
	{
		free_config(free_remotes);
		free_remotes=NULL;
	}
#       ifdef DEBUG
	else
	{
		logprintf("free_remotes still in use\n");
	}
#       endif
}


unsigned long readdata(unsigned long maxusec)
{
	fd_set fds;
	int maxfd,i,ret;
	struct timeval tv;

	while(1)
	{
		FD_ZERO(&fds);
		FD_SET(lirc,&fds);
		FD_SET(sockfd,&fds);
		maxfd=max(lirc,sockfd);
#if defined(SIM_REC) && !defined(DAEMONIZE)
		FD_SET(STDIN_FILENO,&fds);
		maxfd=max(maxfd,STDIN_FILENO);
#endif

		for(i=0;i<clin;i++)
		{
			FD_SET(clis[i],&fds);
			maxfd=max(maxfd,clis[i]);
		}
		
		/*
		  select() is still interrupted despite of SA_RESTART
		  Is this a bug or desired behaviour ???
		*/
		
		do{
			do{
				if(maxusec>0)
				{
					tv.tv_sec=0;
					tv.tv_usec=maxusec;
					ret=select(maxfd+1,&fds,NULL,NULL,&tv);
					if(ret==0) return(0);
				}
				else
				{
					ret=select(maxfd+1,&fds,NULL,NULL,NULL);
				}
				if(free_remotes!=NULL)
				{
					free_old_remotes();
				}
			}
			while(ret==-1 && errno==EINTR);
			if(ret==-1)
			{
				logprintf("select failed\n");
				logperror(NULL);
				continue;
			}
		}
		while(ret==-1);
		
		for(i=0;i<clin;i++)
		{
			if(FD_ISSET(clis[i],&fds))
			{
				FD_CLR(clis[i],&fds);
				if(get_command(clis[i])==0)
				{
					remove_client(clis[i]);
					i--;
				}
			}
		}
		if(FD_ISSET(sockfd,&fds))
		{
#                       ifdef DEBUG
			logprintf("registering new client\n");
#                       endif
			add_client();
		}
		if(FD_ISSET(lirc,&fds))
		{
			unsigned long data;
			int ret;
			
			do
			{
				ret=read(lirc,&data,sizeof(unsigned long));
#                               ifdef DEBUG
				if(ret!=sizeof(unsigned long))
				{
					logprintf("error reading from lirc\n");
					logperror(NULL);
				}
#                               endif
			}
			while(ret!=sizeof(unsigned long));
			
			return(data);
		}
#if defined(SIM_REC) && !defined(DAEMONIZE)
		if(FD_ISSET(STDIN_FILENO,&fds))
		{
			unsigned long data;
			int ret;

			ret=fscanf(stdin,"space %ld\n",&data);
			if(ret==1)
			{
				return(data);
			}
			ret=fscanf(stdin,"pulse %ld\n",&data);
			if(ret==1)
			{
				return(PULSE_BIT|data);
			}
			ret=fscanf(stdin,"%*s\n");
			if(ret==EOF)
			{
				fflush(stdout);
				exit(0);
			}
		}
#endif
	}
}

void loop()
{
	char *message;
	int len,i;
	unsigned long data;

	while(1)
	{
		data=readdata(0);
		
		message=decode_command(data);

		if(message!=NULL)
		{
			len=strlen(message);
				
			for (i=0; i<clin; i++)
			{
#                               ifdef DEBUG
				logprintf("writing to client %d\n",i);
#                               endif
				if(write_socket(clis[i],message,len)<len)
				{
					remove_client(clis[i]);
					i--;
				}				
			}
		}
	}
}  

int main(int argc,char **argv)
{
	struct sigaction act;

	while(1)
	{
		int c;
		static struct option long_options[] =
		{
			{"help",no_argument,NULL,'h'},
			{"version",no_argument,NULL,'v'},
			{0, 0, 0, 0}
		};
		c = getopt_long(argc,argv,"hv",long_options,NULL);
		if(c==-1)
			break;
		switch (c)
		{
		case 'h':
			printf("Usage: %s [options]\n",progname);
			printf("\t -h --help\t\tdisplay this message\n");
			printf("\t -v --version\t\tdisplay version\n");
			return(EXIT_SUCCESS);
		case 'v':
			printf("%s\n",progname);
			return(EXIT_SUCCESS);
		default:
			return(EXIT_FAILURE);
		}
	}
#if !defined(SIM_REC) || defined(DAEMONIZE)
	if(optind!=argc)
	{
		fprintf(stderr,"%s: invalid argument count\n",progname);
		return(EXIT_FAILURE);
	}
#endif

#ifdef LIRC_DECODED
	if((lirc=open("/dev/remote",O_RDWR))<0)
#else
	if((lirc=open("/dev/lirc",O_RDWR))<0)
#endif
	{
		fprintf(stderr,"%s: could not open lirc\n",progname);
		perror(progname);
		exit(EXIT_FAILURE);
	}

	signal(SIGPIPE,SIG_IGN);

	act.sa_handler=sigterm;
	sigemptyset(&act.sa_mask);
	act.sa_flags=SA_RESTART;           /* don't fiddle with EINTR */
	sigaction(SIGTERM,&act,NULL);
	sigaction(SIGINT,&act,NULL);

	act.sa_handler=sigalrm;
	sigemptyset(&act.sa_mask);
	act.sa_flags=SA_RESTART;           /* don't fiddle with EINTR */
	sigaction(SIGALRM,&act,NULL);

	start_server();

	remotes=NULL;
	config();                          /* read config file */

	act.sa_handler=sighup;
	sigemptyset(&act.sa_mask);
	act.sa_flags=SA_RESTART;           /* don't fiddle with EINTR */
	sigaction(SIGHUP,&act,NULL);

#ifdef DAEMONIZE
	/* ready to accept connections */
	daemonize();
#endif

#if defined(SIM_REC) && !defined(DAEMONIZE)
	if(argc>1) sleep(argc);
#endif

#if defined(SIM_SEND) && !defined(DAEMONIZE)
	{
		struct ir_remote *r;
		struct ir_ncode *c;

		printf("space 1000000\n");
		r=remotes;
		while(r!=NULL)
		{
			c=r->codes;
			while(c->name!=NULL)
			{
				repeat_remote=NULL;
				repeat_code=NULL;
				send_command(r,c);
				repeat_remote=r;
				repeat_code=c;
				send_command(r,c);
				send_command(r,c);
				send_command(r,c);
				send_command(r,c);
				c++;
			}
			r=r->next;
		}
		fflush(stdout);
	}
	fprintf(stderr,"Ready.\n");
#endif
	loop();

	/* never reached */
	return(EXIT_SUCCESS); 
}
