
/**
	* MySQL Transaction Monitor
	* mysqltrxmon.c
	*
	* @author        Martin Latter
	* @copyright     Martin Latter, 03/05/2022
	* @version       0.35
	* @license       GNU GPL version 3.0 (GPL v3); https://www.gnu.org/licenses/gpl-3.0.html
	* @link          https://github.com/Tinram/MySQL.git
	*
	* Compile:
	* (Linux GCC x64)
	*                Required dependencies: libmysqlclient-dev, libncurses5-dev
	*                gcc mysqltrxmon.c $(mysql_config --cflags) $(mysql_config --libs) -o mysqltrxmon -I../mysql_include/ -lncurses -Ofast -Wall -Wextra -Wuninitialized -Wunused -Werror -std=gnu99 -s
	*
	* Usage:
	*                ./mysqltrxmon --help
	*                ./mysqltrxmon -u <username> [-h <host>] [-f <logfile>] [-t <time (ms)>] [-p <port>]
*/


#include <mysql_utils.h>
#include <mysql_utils.c>


#define APP_NAME "MySQLTrxMon"
#define MB_VERSION "0.35"


void displayTRXStats(MYSQL* pConn, int* pRow);


unsigned int iTime = 250; // millisecs


int main(int iArgCount, char* const aArgV[])
{
	pProgname = aArgV[0];

	if (iArgCount <= 2)
	{
		menu(pProgname);
		return EXIT_FAILURE;
	}

	MYSQL* pConn;
	FILE* fp = NULL;
	char* const pMaria = "MariaDB";
	char aHostname[50];
	char aVersion[7];
	char aAuroraVersion[9];
	char aAuroraServerId[50];
	int iRow = 0;
	int iCh;
	int iPadPos = 0;
	unsigned int iMenu = options(iArgCount, aArgV);
	unsigned int iPS = 0;
	unsigned int iAccess = 0;
	unsigned int iV8 = 0;
	unsigned int iMaria = 0;
	unsigned int iAurora = 0;
	unsigned int iPadWidth = 157; // 164 for 13" MacBook
	unsigned int iPadHeight = 35; // 30-50; fullscreen terminal differs to windowed

	if (signal(SIGINT, signalHandler) == SIG_ERR)
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

	mysql_options4(pConn, MYSQL_OPT_CONNECT_ATTR_ADD, "program_name", APP_NAME);

	if (mysql_real_connect(pConn, pHost, pUser, pPassword, NULL, iPort, NULL, 0) == NULL)
	{
		fprintf(stderr, "\nCannot connect to MySQL server.\n(Error: %s)\n\n", mysql_error(pConn));
		return EXIT_FAILURE;
	}

	initscr();

	/* Check for ncurses colour support. */
	if (has_colors() == FALSE)
	{
		endwin();
		fprintf(stderr, "\nThis terminal does not support colours.\n\n");
		mysql_close(pConn);
		return EXIT_FAILURE;
	}

	/* Create ncurses pad for screen scrolling. */
	WINDOW* pPad = newpad(iPadHeight, iPadWidth);
	keypad(pPad, TRUE);
	nodelay(pPad, TRUE);
	scrollok(pPad, TRUE);

	/* Assign hostname. */
	assignHostname(pConn, aHostname, sizeof(aHostname) - 1);

	/* Identify MySQL version | MariaDB. */
	identifyMySQLVersion(pConn, aVersion, pMaria, &iMaria, &iV8, sizeof(aVersion) - 1);

	/* Identify Aurora version, if applicable. */
	identifyAuroraVersion(pConn, aAuroraVersion, aAuroraServerId, &iAurora, sizeof(aAuroraVersion) - 1, sizeof(aAuroraServerId) - 1);

	/* Check performance schema availability. */
	checkPerfSchema(pConn, &iPS);

	/* Create logfile header row. */
	if (pLogfile != NULL)
	{
		fp = fopen(pLogfile, "a");

		if (fp == NULL)
		{
			fprintf(stderr, "\nExited: cannot write to logfile.\n\n");
			mysql_close(pConn);
			return EXIT_FAILURE;
		}
		else
		{
			fprintf
			(
				fp,
				"%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s\n",
				"trx", "thd", "ps", "exm", "lock", "mod", "afft", "tmpd", "tlock", "noidx", "wait", "start", "secs", "user", "program", "trxstate", "trxopstate", "query"
			);

			fflush(fp); /* Output header row immediately. */
		}
	}

	/* Set ncurses colours. */
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
			mvprintw(iRow, 1, "%s", aHostname);
		}
		else
		{
			mvprintw(iRow, 1, "%s", aAuroraServerId);
		}
		attroff(A_BOLD);

		if (iMaria == 0)
		{
			if (iAurora == 0)
			{
				mvprintw(iRow += 1, 1, "%s", aVersion);
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

		/* TRX at InnoDB layer. */
		mysql_query(pConn, "SELECT COUNT(*) FROM information_schema.INNODB_TRX"); /* Includes RUNNING, LOCK WAIT, ROLLING BACK, COMMITTING. */
		MYSQL_RES* result_acttr = mysql_store_result(pConn);

		if (mysql_errno(pConn) == 0)
		{
			iAccess = 1;
			MYSQL_ROW row_acttr = mysql_fetch_row(result_acttr);
			attrset(A_BOLD | COLOR_PAIR(4));
			mvprintw(iRow += 2, 1, "trx: %s", row_acttr[0]);
			displayTRXStats(pConn, &iRow);
		}

		mysql_free_result(result_acttr);

		if (iPS == 0)
		{
			attrset(A_BOLD | COLOR_PAIR(4));
			mvprintw(iRow += 2, 1, "performance schema disabled");
			attrset(A_NORMAL);
		}
		else if (iAccess == 0)
		{
			attrset(A_BOLD | COLOR_PAIR(4));
			mvprintw(iRow += 2, 1, "no user privilege access");
			attrset(A_NORMAL);
		}
		else
		{
			iRow = 8;

			mvprintw(iRow, 1, "thd");
			mvprintw(iRow, 10, "ps");
			mvprintw(iRow, 20, "exm");
			mvprintw(iRow, 33, "lock");
			mvprintw(iRow, 46, "mod");
			mvprintw(iRow, 57, "afft");
			mvprintw(iRow, 68, "tmpd");
			mvprintw(iRow, 77, "tlk");
			mvprintw(iRow, 85, "idx");
			mvprintw(iRow, 93, "wait");
			mvprintw(iRow, 107, "start");
			mvprintw(iRow, 130, "sec");
			mvprintw(iRow, 140, "user");

			mysql_query(pConn, "\
				SELECT \
					trx.trx_id, thd.THREAD_ID, thd.PROCESSLIST_ID, stmt.ROWS_EXAMINED, trx.trx_rows_locked, trx.trx_rows_modified, stmt.ROWS_AFFECTED, stmt.CREATED_TMP_DISK_TABLES, trx.trx_tables_locked, stmt.NO_INDEX_USED, ROUND(stmt.TIMER_WAIT/1000000000000, 6), trx.trx_started, TO_SECONDS(NOW()) - TO_SECONDS(trx.trx_started) AS duration, thd.PROCESSLIST_USER, trx.trx_state, trx.trx_operation_state, stmt.SQL_TEXT, sca.ATTR_VALUE \
				FROM \
					information_schema.INNODB_TRX trx \
				INNER JOIN \
					performance_schema.threads thd ON thd.PROCESSLIST_ID = trx.trx_mysql_thread_id \
				INNER JOIN \
					performance_schema.events_statements_current stmt USING (THREAD_ID) \
				LEFT JOIN \
					performance_schema.session_connect_attrs sca ON sca.PROCESSLIST_ID = trx.trx_mysql_thread_id AND sca.ATTR_NAME = 'program_name' \
				ORDER BY \
					duration DESC \
			");
				/* LEFT JOIN for Aurora 2.10 */

			MYSQL_RES* result_trx = mysql_store_result(pConn);
			MYSQL_ROW row_trx;

			iRow = 0;

			while ((row_trx = mysql_fetch_row(result_trx)))
			{
				if (row_trx != NULL)
				{
					char idx = (strcmp("1", row_trx[9]) == 1) ? 'N' : 'Y'; // NO_INDEX_USED -> reversal
					iRow++;

					wattrset(pPad, A_BOLD | COLOR_PAIR(1));
					mvwprintw(pPad, iRow, 1, "%s", row_trx[1]);
					mvwprintw(pPad, iRow, 10, "%s", row_trx[2]);
					mvwprintw(pPad, iRow, 20, "%s", row_trx[3]);
					mvwprintw(pPad, iRow, 33, "%s", row_trx[4]);
					mvwprintw(pPad, iRow, 46, "%s", row_trx[5]);
					mvwprintw(pPad, iRow, 57, "%s", row_trx[6]);
					mvwprintw(pPad, iRow, 68, "%s", row_trx[7]);
					mvwprintw(pPad, iRow, 77, "%s", row_trx[8]);
					mvwprintw(pPad, iRow, 85, "%c", idx);
					mvwprintw(pPad, iRow, 93, "%s", row_trx[10]);
					mvwprintw(pPad, iRow, 107, "%s", row_trx[11]);
					mvwprintw(pPad, iRow, 130, "%s", row_trx[12]);
					mvwprintw(pPad, iRow, 140, "%s", row_trx[13]);
					wattrset(pPad, A_NORMAL);

					iRow++;

					if (row_trx[17] != NULL)
					{
						wattron(pPad, COLOR_PAIR(1));
						mvwprintw(pPad, iRow += 1, 1, "%s", row_trx[17]);
						wattroff(pPad, COLOR_PAIR(1));
					}

					wattron(pPad, COLOR_PAIR(5));
					mvwprintw(pPad, iRow += 1, 1, "%s", row_trx[14]);
					wattroff(pPad, COLOR_PAIR(5));

					if (row_trx[15] != NULL)
					{
						wattrset(pPad, A_BOLD | COLOR_PAIR(3));
						mvwprintw(pPad, iRow += 1, 1, "%s", row_trx[15]);
						wattrset(pPad, A_NORMAL);
					}

					if (row_trx[16] != NULL)
					{
						/* Truncate SQL at ~2 lines. */
						char aQuery[300];
						unsigned int iQLen = sizeof(aQuery) - 1;
						strncpy(aQuery, row_trx[16], iQLen);
						aQuery[iQLen] = '\0';

						/* Replace TABs and LFs. */
						replaceChar(aQuery, '\t', ' ');
						replaceChar(aQuery, '\n', ' ');

						wattron(pPad, COLOR_PAIR(2));
						mvwprintw(pPad, iRow += 1, 1, "%s", aQuery);
						wattroff(pPad, COLOR_PAIR(2));
					}

					if (pLogfile != NULL)
					{
						fprintf
						(
							fp,
							"%s|%s|%s|%s|%s|%s|%s|%s|%s|%c|%s|%s|%s|%s|%s|%s|%s|%s;\n",
							row_trx[0], row_trx[1], row_trx[2], row_trx[3], row_trx[4], row_trx[5], row_trx[6], row_trx[7], row_trx[8],
							idx,
							row_trx[10], row_trx[11], row_trx[12], row_trx[13], row_trx[17], row_trx[14], row_trx[15], row_trx[16]
						);

						//fflush(fp); /* Prefer default OS buffering over immediate flushing of fflush() */
					}
				}

				iRow += 2;
			}

			mysql_free_result(result_trx);

			/* ncurses pad scrolling. */
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

		refresh();

		prefresh(pPad, iPadPos, 1, 9, 1, iPadHeight, iPadWidth); /* (p, scrollY, scrollX, posY, posX, sizeY, sizeX) */

		msSleep(iTime);
	}

	if (pLogfile != NULL)
	{
		fclose(fp);
	}

	curs_set(1);

	delwin(pPad);

	endwin();

	mysql_close(pConn);

	return EXIT_SUCCESS;
}


/**
	* Display TRX Lock Wait and HLL stats.
	*
	* @param   MYSQL* pConn, connection pointer
	* @param   int* pRow, pointer to iRow
	* @return  void
*/

void displayTRXStats(MYSQL* pConn, int* pRow)
{
	/* TRX Lock Waits */
	mysql_query(pConn, "SELECT COUNT(*) FROM information_schema.INNODB_TRX WHERE trx_state = 'LOCK WAIT'");
	MYSQL_RES* result_trlk = mysql_store_result(pConn);
	MYSQL_ROW row_trlk = mysql_fetch_row(result_trlk);
	mvprintw(*pRow += 1, 1, "lwa: %s", row_trlk[0]);
	mysql_free_result(result_trlk);

	/* History List Length */
	mysql_query(pConn, "SELECT COUNT FROM information_schema.INNODB_METRICS WHERE NAME = 'trx_rseg_history_len'");
	MYSQL_RES* result_hll = mysql_store_result(pConn);
	MYSQL_ROW row_hll = mysql_fetch_row(result_hll);
	mvprintw(*pRow += 1, 1, "hll: %s", row_hll[0]);
	attrset(A_NORMAL);
	mysql_free_result(result_hll);
}


/**
	* Process command-line switches using getopt()
	*
	* @param   integer iArgCount, number of arguments
	* @param   char* aArgV, pointer to aArgV
	* @return  unsigned integer
*/

unsigned int options(int iArgCount, char* const aArgV[])
{
	int iOpts = 0;
	int iOptsIdx = 0;
	unsigned int iHelp = 0;

	struct option aLongOpts[] =
	{
		{"help", no_argument, 0, 'i'},
		{0, 0, 0, 0}
	};

	while ((iOpts = getopt_long(iArgCount, aArgV, "ih:w:u:f:t:p:", aLongOpts, &iOptsIdx)) != -1)
	{
		switch (iOpts)
		{
			case 'i':
				iHelp = 1;
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
				if (iTime < 100) {iTime = 100;} /* Breakdown in behaviour below 100ms. */
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
	else if (pUser == NULL)
	{
		fprintf(stderr, "\n%s: use '%s -h' for help\n\n", APP_NAME, aArgV[0]);
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
	* Display program menu.
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
