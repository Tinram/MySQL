
/**
	* MySQL Transaction Monitor
	* mysqltrxmon.c
	*
	* @author        Martin Latter
	* @copyright     Martin Latter, 03/05/2022
	* @version       0.22
	* @license       GNU GPL version 3.0 (GPL v3); https://www.gnu.org/licenses/gpl-3.0.html
	* @link          https://github.com/Tinram/MySQL.git
	*
	* Compile:
	* (Linux GCC x64)
	*                Required dependencies: libmysqlclient-dev, libncurses5-dev
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


#define APP_NAME "MySQLTrxMon"
#define MB_VERSION "0.22"


void signal_handler(int iSig);
void replaceChar(char* aSQL, char cOrg, char cRep);
void menu(char* const pFName);
int msSleep(unsigned int ms);
unsigned int options(int iArgCount, char* aArgV[]);


char* pHost = NULL;
char* pUser = NULL;
char* pPassword = NULL;
char* pLogfile = NULL;
char* pProgname = NULL;
const char* pRoot = "root";
unsigned int iPort = 3306;
unsigned int iTime = 250; // millisecs
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
	const char* pMaria = "MariaDB";
	char aHostname[50];
	char aVersion[7];
	char aAuroraVersion[9];
	char aAuroraServerId[50];
	unsigned int iMenu = options(iArgCount, aArgV);
	unsigned int iPS = 0;
	unsigned int iAccess = 0;
	unsigned int iMaria = 0;
	unsigned int iAurora = 0;
	unsigned int iPadWidth = 157; // 160 for 13" MacBook
	unsigned int iPadHeight = 35; // 30-50; fullscreen terminal differs to windowed
	int iRow = 0;
	int iCh;
	int iPadPos = 0;

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
		pPassword = getpass("password: "); /* Obsolete fn, use termios.h in future. */
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

	/* Create pad for scrolling. */
	WINDOW* pPad = newpad(iPadHeight, iPadWidth);
	keypad(pPad, TRUE);
	nodelay(pPad, TRUE);
	scrollok(pPad, TRUE);

	/* Assign hostname. */
	mysql_query(pConn, "SHOW VARIABLES WHERE Variable_name = 'hostname'");
	MYSQL_RES* result_hn = mysql_store_result(pConn);
	MYSQL_ROW row_hn = mysql_fetch_row(result_hn);
	unsigned int iHLen = sizeof(aHostname) - 1;
	strncpy(aHostname, row_hn[1], iHLen);
	aHostname[iHLen] = '\0';
	mysql_free_result(result_hn);

	/* Identify MySQL version | MariaDB. */
	mysql_query(pConn, "SHOW VARIABLES WHERE Variable_name = 'version'");
	MYSQL_RES* result_ver = mysql_store_result(pConn);
	MYSQL_ROW row_ver = mysql_fetch_row(result_ver);
	if (strstr(row_ver[1], pMaria) != NULL)
	{
		iMaria = 1;
	}
	unsigned int iVLen = sizeof(aVersion) - 1;
	strncpy(aVersion, row_ver[1], iVLen);
	aVersion[iVLen] = '\0';
	mysql_free_result(result_ver);

	/* Identify Aurora version, if applicable. */
	mysql_query(pConn, "SHOW VARIABLES WHERE Variable_name = 'aurora_version'");
	MYSQL_RES* result_aur_ver = mysql_store_result(pConn);
	MYSQL_ROW row_aur_ver = mysql_fetch_row(result_aur_ver);
	mysql_free_result(result_aur_ver);

	if (row_aur_ver != NULL)
	{
		iAurora = 1;
		unsigned int iAVLen = sizeof(aAuroraVersion) - 1;
		strncpy(aAuroraVersion, row_aur_ver[1], iAVLen);
		aAuroraVersion[iAVLen] = '\0';

		mysql_query(pConn, "SHOW VARIABLES WHERE Variable_name = 'aurora_server_id'");
		MYSQL_RES* result_aur_sid = mysql_store_result(pConn);
		MYSQL_ROW row_aur_sid = mysql_fetch_row(result_aur_sid);
		mysql_free_result(result_aur_sid);

		unsigned int iASIdLen = sizeof(aAuroraServerId) - 1;
		strncpy(aAuroraServerId, row_aur_sid[1], iASIdLen);
		aAuroraServerId[iASIdLen] = '\0';
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
			fprintf(fp, "%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s\n", "trx", "thd", "ps", "exm", "lock", "mod", "afft", "tmpd", "tlock", "noidx", "wait", "start", "secs", "user", "trxstate", "trxopstate", "query");
			fclose(fp);
		}
	}

	/* Check performance schema availability. */
	mysql_query(pConn, "SHOW VARIABLES WHERE Variable_name = 'performance_schema'");
	MYSQL_RES* result_ps = mysql_store_result(pConn);
	MYSQL_ROW row_ps = mysql_fetch_row(result_ps);
	if (strstr(row_ps[1], "ON") != NULL)
	{
		iPS = 1;
	}
	mysql_free_result(result_ps);

	start_color();
	init_color(COLOR_BLACK, 0, 0, 0); // for Gnome
	init_pair(1, COLOR_GREEN, COLOR_BLACK);
	init_pair(2, COLOR_MAGENTA, COLOR_BLACK);
	init_pair(3, COLOR_CYAN, COLOR_BLACK);
	init_pair(4, COLOR_RED, COLOR_BLACK);
	init_pair(5, COLOR_BLUE, COLOR_BLACK);
	curs_set(0);

	while ( ! iSigCaught)
	{
		clear();
		wclear(pPad);

		iRow = 1;

		attron(A_BOLD);
		if (iAurora == 0)
		{
			mvprintw(iRow, 1, aHostname);
		}
		else
		{
			mvprintw(iRow, 1, aAuroraServerId);
		}
		attroff(A_BOLD);

		if (iMaria == 0)
		{
			if (iAurora == 0)
			{
				mvprintw(iRow += 1, 1, aVersion);
			}
			else
			{
				mvprintw(iRow += 1, 1, "%s (au: %s)", aVersion, aAuroraVersion);
			}
		}
		else
		{
			mvprintw(iRow += 1, 1, "%s %s", pMaria, aVersion);
		}

		/* TRX at InnoDB layer */
		mysql_query(pConn, "SELECT COUNT(*) FROM information_schema.INNODB_TRX"); /* Includes RUNNING, LOCK WAIT, ROLLING BACK, COMMITTING. */
		MYSQL_RES* result_acttr = mysql_store_result(pConn);

		if (mysql_errno(pConn) == 0)
		{
			iAccess = 1;
		}

		mysql_free_result(result_acttr);

		if (iAccess == 1)
		{
			MYSQL_ROW row_acttr = mysql_fetch_row(result_acttr);
			attrset(A_BOLD | COLOR_PAIR(4));
			mvprintw(iRow += 2, 1, "trx: %s", row_acttr[0]);

			/* History List Length */
			mysql_query(pConn, "SELECT COUNT FROM information_schema.INNODB_METRICS WHERE NAME = 'trx_rseg_history_len'");
			MYSQL_RES* result_hll = mysql_store_result(pConn);
			MYSQL_ROW row_hll = mysql_fetch_row(result_hll);
			mvprintw(iRow += 1, 1, "hll: %s", row_hll[0]);
			attrset(A_NORMAL);
			mysql_free_result(result_hll);
		}

		if (iPS == 0)
		{
			attrset(A_BOLD | COLOR_PAIR(4));
			mvprintw(iRow += 2, 1, "performance schema disabled");
			attrset(A_NORMAL);
		}

		if (iAccess == 1)
		{
			iRow = 8;

			mvprintw(iRow, 1, "thd");
			mvprintw(iRow, 8, "ps");
			mvprintw(iRow, 16, "exm");
			mvprintw(iRow, 28, "lock");
			mvprintw(iRow, 42, "mod");
			mvprintw(iRow, 53, "afft");
			mvprintw(iRow, 64, "tmpd");
			mvprintw(iRow, 73, "tlk");
			mvprintw(iRow, 81, "idx");
			mvprintw(iRow, 89, "wait");
			mvprintw(iRow, 101, "start");
			mvprintw(iRow, 126, "sec");
			mvprintw(iRow, 136, "user");

			mysql_query(pConn, "\
				SELECT \
					trx.trx_id, thd.THREAD_ID, thd.PROCESSLIST_ID, stmt.ROWS_EXAMINED, trx.trx_rows_locked, trx.trx_rows_modified, stmt.ROWS_AFFECTED, stmt.CREATED_TMP_DISK_TABLES, trx.trx_tables_locked, stmt.NO_INDEX_USED, ROUND(stmt.TIMER_WAIT/1000000000000, 6), trx.trx_started, TO_SECONDS(NOW()) - TO_SECONDS(trx.trx_started) AS duration, thd.PROCESSLIST_USER, trx.trx_state, trx.trx_operation_state, stmt.SQL_TEXT \
				FROM \
					information_schema.INNODB_TRX trx \
				INNER JOIN \
					performance_schema.threads thd ON thd.PROCESSLIST_ID = trx.trx_mysql_thread_id \
				INNER JOIN \
					performance_schema.events_statements_current stmt USING (THREAD_ID) \
				ORDER BY \
					duration DESC \
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

			iRow = 0;

			while ((row_trx = mysql_fetch_row(result_trx)))
			{
				if (row_trx != NULL)
				{
					char idx = (strcmp("1", row_trx[9]) == 1) ? 'N' : 'Y'; // NO_INDEX_USED -> reversal
					iRow++;

					wattrset(pPad, A_BOLD | COLOR_PAIR(1));
					mvwprintw(pPad, iRow, 1, row_trx[1]);
					mvwprintw(pPad, iRow, 8, row_trx[2]);
					mvwprintw(pPad, iRow, 16, row_trx[3]);
					mvwprintw(pPad, iRow, 28, row_trx[4]);
					mvwprintw(pPad, iRow, 42, row_trx[5]);
					mvwprintw(pPad, iRow, 53, row_trx[6]);
					mvwprintw(pPad, iRow, 64, row_trx[7]);
					mvwprintw(pPad, iRow, 73, row_trx[8]);
					mvwprintw(pPad, iRow, 81, "%c", idx);
					mvwprintw(pPad, iRow, 89, row_trx[10]);
					mvwprintw(pPad, iRow, 101, row_trx[11]);
					mvwprintw(pPad, iRow, 126, row_trx[12]);
					mvwprintw(pPad, iRow, 136, row_trx[13]);
					wattrset(pPad, A_NORMAL);

					wattron(pPad, COLOR_PAIR(5));
					mvwprintw(pPad, iRow += 2, 1, row_trx[14]);
					wattroff(pPad, COLOR_PAIR(5));

					if (row_trx[15] != NULL)
					{
						wattrset(pPad, A_BOLD | COLOR_PAIR(3));
						mvwprintw(pPad, iRow += 1, 1, row_trx[15]);
						wattrset(pPad, A_NORMAL);
					}

					if (row_trx[16] != NULL)
					{
						/* Truncate SQL at ~2 lines */
						char aQuery[300];
						unsigned int iQLen = sizeof(aQuery) - 1;
						strncpy(aQuery, row_trx[16], iQLen);
						aQuery[iQLen] = '\0';

						/* Remove LFs. */
						replaceChar(aQuery, '\n', ' ');

						wattron(pPad, COLOR_PAIR(2));
						mvwprintw(pPad, iRow += 1, 1, "%s", aQuery);
						wattroff(pPad, COLOR_PAIR(2));
					}

					if (pLogfile != NULL)
					{
						fprintf(fp, "%s|%s|%s|%s|%s|%s|%s|%s|%s|%c|%s|%s|%s|%s|%s|%s|%s;\n", row_trx[0], row_trx[1], row_trx[2], row_trx[3], row_trx[4], row_trx[5], row_trx[6], row_trx[7], row_trx[8], idx, row_trx[10], row_trx[11], row_trx[12], row_trx[13], row_trx[14], row_trx[15], row_trx[16]);
					}
				}

				iRow += 2;
			}

			mysql_free_result(result_trx);

			if (pLogfile != NULL)
			{
				fclose(fp);
			}

			iCh = wgetch(pPad);

			switch (iCh)
			{
				case KEY_UP:
					iPadPos--;
				break;

				case KEY_DOWN:
					iPadPos++;
				break;
			}
		}
		else
		{
			attrset(A_BOLD | COLOR_PAIR(4));
			mvprintw(iRow += 2, 1, "no user privilege access");
			attrset(A_NORMAL);
		}

		refresh();

		prefresh(pPad, iPadPos, 1, 9, 1, iPadHeight, iPadWidth); /* (p, scrollY, scrollX, posY, posX, sizeY, sizeX) */

		msSleep(iTime);
	}

	curs_set(1);

	delwin(pPad);

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
				if (iTime < 10) {iTime = 10;}
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

int msSleep(unsigned int ms)
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
	* Character replacement to clean multi-line SQL strings.
	* Credit: Fabio Cabral.
	*
	* @param   array aSQL, SQL string
	* @param   char cOrg, original char
	* @param   char cRep, replace char
	* @return  void
*/

void replaceChar(char* aSQL, char cOrg, char cRep) {

	char* src;
	char* dst;

	for (src = dst = aSQL; *src != '\0'; src++)
	{
		*dst = *src;

		if (*dst == cOrg)
		{
			*dst = cRep;
		}

		dst++;
	}

	*dst = '\0';
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
