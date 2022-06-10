
/**
	* MySQL Transaction Monitor
	* mysqltrxmon.c
	*
	* @author        Martin Latter
	* @copyright     Martin Latter, 03/05/2022
	* @version       0.12
	* @license       GNU GPL version 3.0 (GPL v3); https://www.gnu.org/licenses/gpl-3.0.html
	* @link          https://github.com/Tinram/MySQL.git
	*
	* Compile:
	* (Linux GCC x64)
	*                required dependencies: libmysqlclient-dev, libncurses5-dev
	*                gcc mysqltrxmon.c $(mysql_config --cflags) $(mysql_config --libs) -o mysqltrxmon -lncurses -Ofast -Wall -Wextra -Wuninitialized -Wunused -Werror -std=gnu99 -s
	*
	* Usage:
	*                ./mysqltrxmon --help
	*                ./mysqltrxmon -u <username> [-h <host>] [-f <logfile>] [-t <time (ms)>] [-p <port>]
*/


#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <curses.h>
#include <getopt.h>
#include <mysql.h>


#define APP_NAME "mysqltrxmon"
#define MB_VERSION "0.12"


void menu(char* const pFName);
unsigned int options(int iArgCount, char* aArgV[]);
int mssleep(unsigned int ms);
void signal_handler(int iSig);


char* pHost = NULL;
char* pUser = NULL;
char* pPassword = NULL;
char* pLogfile = NULL;
char* pProgname = NULL;
const char* pRoot = "root";
unsigned int iPort = 3306;
unsigned int iTime = 250; // ms
unsigned int iSigCaught = 0;


/**
	* Sigint handling.
	* Based on example by Greg Kemnitz.
	*
	* @param   integer iSig
	* @return  void
*/

void signal_handler(int iSig)
{
	if (iSig == SIGINT || iSig == SIGTERM || iSig == SIGSEGV)
	{
		iSigCaught = 1;
	}
}


int main(int iArgCount, char* aArgV[])
{
	MYSQL* pConn;
	FILE* fp;
	unsigned int iMenu = options(iArgCount, aArgV);
	unsigned int iPS = 0;
	unsigned int iAccess = 0;
	pProgname = aArgV[0];

	if (signal(SIGINT, signal_handler) == SIG_ERR)
	{
		fprintf(stderr, "Signal function registration failed!\n");
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

	initscr();

	if (has_colors() == FALSE)
	{
		endwin();
		fprintf(stderr, "\nThis terminal does not support colours.\n\n");
		mysql_close(pConn);
		return EXIT_FAILURE;
	}

	/* Create logfile header row. */
	if (pLogfile != NULL)
	{
		fp = fopen(pLogfile, "a");

		if (fp == NULL)
		{
			fprintf(stderr, "\nCannot write to logfile.\n\n");
			mysql_close(pConn);
			return EXIT_FAILURE;
		}
		else
		{
			fprintf(fp, "%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s\n", "thd", "ps", "exm", "lock", "mod", "afft", "tmpd", "noidx", "wait", "start", "user", "trxop", "query");
			fclose(fp);
		}
	}

	/* Check performance schema availability. */
	mysql_query(pConn, "SHOW variables WHERE Variable_name = 'performance_schema'");
	MYSQL_RES* result_ps = mysql_store_result(pConn);
	MYSQL_ROW row_ps = mysql_fetch_row(result_ps);
	if (strstr(row_ps[1], "ON") != NULL)
	{
		iPS = 1;
	}
	mysql_free_result(result_ps);

	start_color();
	init_pair(1, COLOR_GREEN, COLOR_BLACK);
	init_pair(2, COLOR_MAGENTA, COLOR_BLACK);
	init_pair(3, COLOR_CYAN, COLOR_BLACK);
	init_pair(4, COLOR_RED, COLOR_BLACK);
	curs_set(0);

	while ( ! iSigCaught)
	{
		clear();

		printw("\n");
		mysql_query(pConn, "SHOW variables WHERE Variable_name = 'hostname'");
		MYSQL_RES* result_hn = mysql_store_result(pConn);
		MYSQL_ROW row_hn = mysql_fetch_row(result_hn);
		attron(A_BOLD);
		printw(" %s\n\n", row_hn[1]);
		attroff(A_BOLD);
		mysql_free_result(result_hn);

		/* TRX at InnoDB layer */
		mysql_query(pConn, "SELECT COUNT(*) FROM information_schema.INNODB_TRX"); /* includes RUNNING, LOCK WAIT, ROLLING BACK, COMMITTING */
		MYSQL_RES* result_acttr = mysql_store_result(pConn);

		if (mysql_errno(pConn) == 0)
		{
			iAccess = 1;
		}

		mysql_free_result(result_acttr);

		if (iAccess == 1)
		{
			MYSQL_ROW row_acttr = mysql_fetch_row(result_acttr);
			printw(" trx: %s\n", row_acttr[0]);

			/* History List Length */
			mysql_query(pConn, "SELECT COUNT FROM information_schema.INNODB_METRICS WHERE NAME = 'trx_rseg_history_len'");
			MYSQL_RES* result_hll = mysql_store_result(pConn);
			MYSQL_ROW row_hll = mysql_fetch_row(result_hll);
			printw(" hll: %s\n\n", row_hll[0]);
			mysql_free_result(result_hll);
		}

		if (iPS == 0)
		{
			attron(A_BOLD);
			attron(COLOR_PAIR(4));
			printw(" performance schema disabled\n");
			attroff(COLOR_PAIR(4));
			attroff(A_BOLD);
		}

		if (iAccess == 1)
		{
			mysql_query(pConn, "\
				SELECT thd.THREAD_ID, thd.PROCESSLIST_ID, stmt.ROWS_EXAMINED, trx.trx_rows_locked, trx.trx_rows_modified, stmt.ROWS_AFFECTED, stmt.CREATED_TMP_DISK_TABLES, stmt.NO_INDEX_USED, ROUND(stmt.TIMER_WAIT/1000000000000, 4), trx.trx_started, thd.PROCESSLIST_USER, trx.trx_operation_state, stmt.SQL_TEXT \
				FROM information_schema.INNODB_TRX trx \
					INNER JOIN performance_schema.threads thd ON thd.PROCESSLIST_ID = trx.trx_mysql_thread_id \
					INNER JOIN performance_schema.events_statements_current stmt USING (THREAD_ID) \
				WHERE trx.trx_state = 'RUNNING' \
			");

			MYSQL_RES* result_trx = mysql_store_result(pConn);
			MYSQL_ROW row_trx;

			if (pLogfile != NULL)
			{
				fp = fopen(pLogfile, "a");

				if (fp == NULL)
				{
					fprintf(stderr, "\nCannot write to logfile.\n\n");
					mysql_close(pConn);
					return EXIT_FAILURE;
				}
			}

			while ((row_trx = mysql_fetch_row(result_trx)))
			{
				if (row_trx != NULL)
				{
					char idx = (strcmp("1", row_trx[7]) == 1) ? 'N' : 'Y'; // NO_INDEX_USED -> reversal

					printw(" %s\t%s\t%s\t%s\t\t%s\t%s\t%s\t%s\t%s\t\t%s\t\t\t%s\n", "thd", "ps", "exm", "lock", "mod", "afft", "tmpd", "idx", "wait", "start", "user");

					attron(A_BOLD);

					attron(COLOR_PAIR(1));
					printw(" %s\t%s\t%s\t%s\t\t%s\t%s\t%s\t%c\t%s\t\t%s\t%s\n\n", row_trx[0], row_trx[1], row_trx[2], row_trx[3], row_trx[4], row_trx[5], row_trx[6], idx, row_trx[8], row_trx[9], row_trx[10]);
					attroff(COLOR_PAIR(1));

					if (row_trx[11] != NULL)
					{
						attron(COLOR_PAIR(2));
						printw(" %s\n\n", row_trx[11]);
						attroff(COLOR_PAIR(2));
					}

					if (row_trx[12] != NULL)
					{
						attron(COLOR_PAIR(3));
						printw(" %s\n\n\n", row_trx[12]);
						attroff(COLOR_PAIR(3));
					}

					attroff(A_BOLD);

					if (pLogfile != NULL)
					{
						fprintf(fp, "%s|%s|%s|%s|%s|%s|%s|%c|%s|%s|%s|%s|%s;\n", row_trx[0], row_trx[1], row_trx[2], row_trx[3], row_trx[4], row_trx[5], row_trx[6], idx, row_trx[8], row_trx[9], row_trx[10], row_trx[11], row_trx[12]);
					}
				}
			}

			mysql_free_result(result_trx);

			if (pLogfile != NULL)
			{
				fclose(fp);
			}
		}
		else
		{
			attron(A_BOLD);
			attron(COLOR_PAIR(4));
			printw(" no user privilege access\n");
			attroff(COLOR_PAIR(4));
			attroff(A_BOLD);
		}

		refresh();

		mssleep(iTime);
	}

	curs_set(1);

	endwin();

	mysql_close(pConn);

	return EXIT_SUCCESS;
}


/**
	* Process command-line switches using getopt()
	*
	* @param   integer iArgCount, number of arguments
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

	while ((iOpts = getopt_long(iArgCount, aArgV, "ivh:w:u:f:t:p:", aLongOpts, &iOptsIdx)) != -1)
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
				pLogfile = optarg;
				break;

			case 't':
				iTime = (unsigned int) atoi(optarg);
				if (iTime < 50) {iTime = 50;}
				if (iTime > 2000) {iTime = 2000;}
				break;

			case 'p':
				iPort = (unsigned int) atoi(optarg);
				break;

			case '?':

				if (optopt == 'h' || optopt == 'w' || optopt == 'u' || optopt == 'f' || optopt == 't' || optopt == 'p')
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
	* Use nanosleep() instead of unreliable usleep().
	* Credit: Sunny Shukia.
	*
	* @param   unsigned integer ms, milliseconds
	* @return  signed integer
*/

int mssleep(unsigned int ms)
{
	struct timespec rem;
	struct timespec req =
	{
		(int) (ms / 1000),
		(ms % 1000) * 1000000
	};

	return nanosleep(&req, &rem);
}


/**
	* Display menu.
	*
	* @param   char* pFName, filename from aArgV[0]
	* @return  void
*/

void menu(char* const pFName)
{
	fprintf(stdout, "\n%s v.%s\nby Tinram", APP_NAME, MB_VERSION);
	fprintf(stdout, "\n\nUsage:\n");
	fprintf(stdout, "\t%s -u <user> [-h <host>] [-f <logfile>] [-t <time (ms)>] [-p <port>]\n\n", pFName);
}
