#include <signal.h>

#include <wac/ac.h>
#include <array.h>

void usage()
{
fprintf(stderr,"usage:\n");
fprintf(stderr,"accmd database [ -prompt xx ] [ -timeout seconds ] [ -ace_in | -ace_out ]\n");
fprintf(stderr,"         database is one of\n") ;
fprintf(stderr,"              r:host:port example r:vesta:2000100, for oldstyle rpc server\n") ;
fprintf(stderr,"              a:host:port example a:vesta:12345, Mark's simple tcp tgifacemblyserver\n") ;
fprintf(stderr,"              s:hots:port:user:passwd example s:kaa:12345:anonymous, Sanger tcp taceserver\n") ;
fprintf(stderr,"	-prompt xx	set the prompt\n");
fprintf(stderr,"	-ace_in		read standard input as a .ace file to parse into \n");
fprintf(stderr,"			the database \n");
fprintf(stderr,"	-ace_out	suppress the prompt and command output.  the special \n");
fprintf(stderr,"			command 'write' displays the result of the last \n");
fprintf(stderr,"			command issued.  For example, you might: \n");
fprintf(stderr,"				accmd db -ace_out << EOF \n");
fprintf(stderr,"				find object \n");
fprintf(stderr,"				query tag \n");
fprintf(stderr,"				show -a \n");
fprintf(stderr,"				write \n");
fprintf(stderr,"				EOF \n");
fprintf(stderr,"			to extract a .ace file from the database \n");
fprintf(stderr,"	-timeout sec	exit if open takes longer than <sec> seconds. \n");
fprintf(stderr,"exit codes are:\n");
fprintf(stderr,"0	ok\n");
fprintf(stderr,"1	database open failed immediately \n");
fprintf(stderr,"2	database open failed after timeout \n");
fprintf(stderr,"3	server disconnect during command \n");
fprintf(stderr,"4	unrecognized parameters \n");
fprintf(stderr,"5	shutdown failed\n");
exit(4);
}

void timed_out()
{
write(2,"timeout\n",8);
exit(2);
}

int main(int argc,char **argv)
{
	AC_DB db;
	char *prompt, *dbname;
	char *s,*response;
	int ace_out = 0, ace_in = 0;
	char b[2000];
	int timeout = 0;

	argv++;

	if (!argv[0])
		usage();

	dbname = argv[0];

	argv++;
	
	prompt=halloc(strlen(dbname)+20, 0);
	sprintf(prompt,"acedb@%s>",dbname);
	while (argv[0])
		{
		if (strcasecmp(argv[0],"-prompt") == 0 || strcasecmp(argv[0],"-p") == 0)
			{
			argv++;
			if (!argv[0])
				usage();
			prompt=argv[0];
			}
		else if (strcasecmp(argv[0],"-ace_out") == 0 || strcasecmp(argv[0],"-aceout") == 0)
			{
			ace_out = 1;
			prompt="";
			}
		else if (strcasecmp(argv[0],"-ace_in") == 0 || strcasecmp(argv[0],"-acein") == 0)
			ace_in = 1;
		else if (strcasecmp(argv[0],"-timeout") == 0 || strcasecmp(argv[0],"-time_out") == 0)
			{
			argv++;
			if (!argv[0])
				usage();
			timeout = atoi(argv[0]);
			}
		else
			usage();
		argv++;
		}

	if (timeout)
		{
		signal(SIGALRM, timed_out);
		alarm(timeout);
		}
	db = ac_open_db ( dbname, &s );
	if (timeout)
		alarm(0);
	if (!db)
		{
		fprintf(stderr,"database open error: %s\n",s);
		exit(1);
		}

	if (ace_in)
		{
		/*
		* ace_in is a special case - all of stdin will be an ace file
		*/
		  BOOL ok ;
		Stack st;
		AC_KEYSET ks;
		char *err;
		ks = 0;

		st = stackCreate(10000);
		stackTextOnly(st);

		b[sizeof(b)-1]=0;
		while (fgets(b,sizeof(b)-1,stdin))
			catText(st, b);

		ok = ac_parse( db, stackText(st,0), &err, &ks, NULL_HANDLE);
		/*
		  if (!ok)
		  fprintf(stderr,"parse error %s \n", err ? err : "");
		  else
		*/
		fprintf(stderr,"%d objects parsed \n", ac_keyset_count(ks));
		if (err && *err)
		  fprintf(stderr,"%s",err);
		exit(0);
		}

	response = 0;

	for (;;)
		{
		char b[2000];
		int len;

		printf("%s",prompt);
		fflush(stdout);
		if (! fgets(b,sizeof(b),stdin))
			break;
		s = strchr(b,'\n');
		if (s) 
			*s = 0;
		if (strcmp(b,"quit") == 0)
			{
			/* not really necessary, but people want to type 'quit' */
			break;
			}
		else if (strcmp(b,"write") == 0)
			{
			write(1,response,len);
			}
		else if (strncmp(b,"shutdown",8) == 0)
			{
			response = ac_command(db, b, &len, 0);
			if (strstr(response,"Sorry") == 0)
				{
				/*
				* the server would not let us shut it down.
				*/
				write(1,response,len);
				exit(5);
				}
			else
				exit(0);
			}
		else
			{
			if (response)
				messfree(response);
			response = ac_command(db, b, &len, 0);
			if (!response)
				{
				printf("server disconnect\n");
				exit(3);
				}
			if (! ace_out)
				write(1,response,len);
			}
		}
	exit(0);
}
