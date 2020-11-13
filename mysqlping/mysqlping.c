
/**
	* MySQL Ping
	* mysqlping.c
	*
	* MySQL pinger from C API.
	*
	* @author        Martin Latter
	* @copyright     Martin Latter, 02/09/2020
	* @version       0.05
	* @license       GNU GPL version 3.0 (GPL v3); https://www.gnu.org/licenses/gpl-3.0.html
	* @link          https://github.com/Tinram/MySQL.git
	*
	* Compile:
	* (Linux GCC x64)
	*                required dependency: libmysqlclient-dev
	*                gcc mysqlping.c $(mysql_config --cflags) $(mysql_config --libs) -o mysqlping -Ofast -Wall -Wextra -Wuninitialized -Wunused -Werror -std=gnu99 -s
	*
	* Usage:
	*                ./mysqlping --help
	*                ./mysqlping -u <username> [-h <host>] [-f] [-p port]
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <signal.h>
#include <mysql.h>
#include <unistd.h>


#define APP_NAME "MySQL Ping"
#define MB_VERSION "0.05"


void menu(char* pProgname);
unsigned int options(int iArgCount, char* aArgV[]);
void signal_handler(int sig);


char* pHost = NULL;
char* pUser = NULL;
char* pPassword = NULL;
char* pProgname = NULL;
unsigned int iPort = 3306;
unsigned iFlood = 0; // ping flood flag
unsigned int iSigCaught = 0;


/* based on an example by Greg Kemnitz */
void signal_handler(int iSig)
{
	if (iSig == SIGINT)
	{
		iSigCaught = 1;
	}
}


int main(int iArgCount, char* aArgV[])
{
	unsigned int iMenu = options(iArgCount, aArgV);

	if (signal(SIGINT, signal_handler) == SIG_ERR)
	{
		fprintf(stderr, "Signal fn registration failed!\n");
		return EXIT_FAILURE;
	}

	if ( ! iMenu)
	{
		return EXIT_FAILURE;
	}
	else
	{
		pPassword = getpass("password: "); // obsolete fn, use termios.h in future
	}

	pProgname = aArgV[0];

	MYSQL* pConn;

	pConn = mysql_init(NULL);

	if (pConn == NULL)
	{
		fprintf(stderr, "\nCannot initialise MySQL connector.\n\n");
		return EXIT_FAILURE;
	}

	if (mysql_real_connect(pConn, pHost, pUser, pPassword, NULL, iPort, NULL, 0) == NULL)
	{
		fprintf(stderr, "\nCannot connect to MySQL server.\n\n");
		return EXIT_FAILURE;
	}
	else
	{
		fprintf(stdout, "pinging %s...\n", pHost);
	}

	unsigned int iCounter = 0;

	while (!iSigCaught)
	{
		int iR = mysql_ping(pConn);

		if (iR != 0)
		{
			switch (iR)
			{
				case 1:
					fprintf(stderr, "Could not connect to MySQL server (error: %i).\n", iR);
				break;

				case 2006:
					fprintf(stderr, "MySQL server has gone away (error: %i).\n", iR);
				break;

				default:
					fprintf(stderr, "Ping to MySQL server exited with code: %i\n", iR);
			}

			break;
		}

		if (iFlood == 0)
		{
			fprintf(stdout, "%u\r", iCounter);
			fflush(stdout);
			sleep(1);
		}
		else
		{
			if (iCounter % 1000 == 0)
			{
				fprintf(stdout, "%u\r", iCounter);
				fflush(stdout);
			}
		}

		iCounter++;
	}

	fprintf(stdout, "\nstopped\n");

	mysql_close(pConn);

	return EXIT_SUCCESS;
}


/**
	* Process command-line switches using getopt()
	*
	* @param   int iArgCount, number of arguments
	* @param   array aArgV, switches
	* @return  unsigned integer
*/

unsigned int options(int iArgCount, char* aArgV[])
{
	int iOpts = 0;
	int iOptsIdx = 0;
	unsigned int iHelp = 0;
	unsigned int iVersion = 0;

	struct option aLongOpts[] =
	{
		{"help", no_argument, 0, 'i'},
		{"version", no_argument, 0, 'v'},
		{0, 0, 0, 0}
	};

	while ((iOpts = getopt_long(iArgCount, aArgV, "ivh:u:p:f", aLongOpts, &iOptsIdx)) != -1)
	{
		switch (iOpts)
		{
			case 'i':
				iHelp = 1;
				break;

			case 'v':
				iVersion = 1;
				break;

			case 'h':
				pHost = optarg;
				break;

			case 'u':
				pUser = optarg;
				break;

			case 'f':
				iFlood = 1;
				break;

			case 'p':
				iPort = (unsigned int) atoi(optarg);
				break;

			case '?':

				if (optopt == 'h' || optopt == 'u' || optopt == 'p')
				{
					fprintf(stderr, "\nMissing switch arguments.\n\n");
				}
				else if (optopt == 0)
				{
					break;
				}
				else
				{
					fprintf(stderr, "\nUnknown option `-%c'.\n\n", optopt);
				}

				return 0;
				break;

			default:
				return 0;
		}
	}

	if (iHelp == 1)
	{
		menu(aArgV[0]);
		return 0;
	}
	else if (iVersion == 1)
	{
		fprintf(stdout, "\n%s v.%s\n\n", APP_NAME, MB_VERSION);
		return 0;
	}
	else if (pUser == NULL)
	{
		fprintf(stderr, "\n%s: use '%s --help' for help\n\n", APP_NAME, aArgV[0]);
		return 0;
	}
	else
	{
		if (pHost == NULL)
		{
			pHost = "localhost";
		}

		return 1;
	}
}


/**
	* Display menu.
	*
	* @param   char* pProgname, filename from aArgV[0]
	* @return  void
*/

void menu(char* pProgname)
{
	fprintf(stdout, "\n%s v.%s\nby Tinram", APP_NAME, MB_VERSION);
	fprintf(stdout, "\n\nUsage:\n");
	fprintf(stdout, "\t%s -u <user> [-h <host>] [-f] [-p <port>]\n\n", pProgname);
}
