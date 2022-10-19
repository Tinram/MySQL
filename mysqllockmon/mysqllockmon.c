
/**
	* MySQL Lock Monitor
	* mysqllockmon.c
	*
	* @author        Martin Latter
	* @copyright     Martin Latter, 06/07/2022
	* @version       0.14 (from mysqltrxmon)
	* @license       GNU GPL version 3.0 (GPL v3); https://www.gnu.org/licenses/gpl-3.0.html
	* @link          https://github.com/Tinram/MySQL.git
	*
	* Compile:
	* (Linux GCC x64)
	*                Required dependencies: libmysqlclient-dev, libncurses5-dev
	*                gcc mysqllockmon.c $(mysql_config --cflags) $(mysql_config --libs) -o mysqllockmon -lncurses -Ofast -Wall -Wextra -Wuninitialized -Wunused -Werror -std=gnu99 -s
	*
	* Usage:
	*                ./mysqllockmon --help
	*                ./mysqllockmon -u <username> [-h <host>] [-f <logfile>] [-t <time (ms)>] [-p <port>]
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


#define APP_NAME "mysqllockmon"
#define MB_VERSION "0.14"


void signal_handler(int iSig);
void menu(char* const pFName);
int mssleep(unsigned int ms);
unsigned int options(int iArgCount, char* aArgV[]);


char* pHost = NULL;
char* pUser = NULL;
char* pPassword = NULL;
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
	unsigned int iMenu = options(iArgCount, aArgV);
	unsigned int iV8 = 0;
	unsigned int iPS = 0;
	unsigned int iAccess = 0;
	unsigned int iMDL = 0;
	int iRow = 0;
	int iKey = 0;
	char cDisplay = 'T'; /* trx display default */
	char aVersion[6];

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

	//mysql_options(pConn, MYSQL_INIT_COMMAND, "SET autocommit=0");
	//mysql_options(pConn, MYSQL_OPT_COMPRESS);

	if (mysql_real_connect(pConn, pHost, pUser, pPassword, NULL, iPort, NULL, 0) == NULL)
	{
		fprintf(stderr, "\nCannot connect to MySQL server.\n\n");
		return EXIT_FAILURE;
	}

	initscr();
	nodelay(stdscr, TRUE);
	keypad(stdscr, TRUE);

	if (has_colors() == FALSE)
	{
		endwin();
		fprintf(stderr, "\nThis terminal does not support colours.\n\n");
		mysql_close(pConn);
		return EXIT_FAILURE;
	}

	/* Identify MySQL version. */
	mysql_query(pConn, "SHOW VARIABLES WHERE Variable_name = 'version'");
	MYSQL_RES* result_ver = mysql_store_result(pConn);
	MYSQL_ROW row_ver = mysql_fetch_row(result_ver);
	strncpy(aVersion, row_ver[1], sizeof(aVersion));
	if ((unsigned int) atoi(row_ver[1]) >= 8)
	{
		iV8 = 1;
	}
	mysql_free_result(result_ver);

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
		iRow = 1;

		mysql_query(pConn, "SHOW VARIABLES WHERE Variable_name = 'hostname'");
		MYSQL_RES* result_hn = mysql_store_result(pConn);
		MYSQL_ROW row_hn = mysql_fetch_row(result_hn);
		mvprintw(iRow, 1, APP_NAME);
		iRow += 2;
		attron(A_BOLD);
		mvprintw(iRow, 1, row_hn[1]);
		attroff(A_BOLD);
		mysql_free_result(result_hn);
		iRow++;
		mvprintw(iRow, 1, "%s", aVersion);

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
			attrset(COLOR_PAIR(4));
			mvprintw(iRow += 2, 1, "trx: %s", row_acttr[0]);

			/* History List Length */
			mysql_query(pConn, "SELECT COUNT FROM information_schema.INNODB_METRICS WHERE NAME = 'trx_rseg_history_len'");
			MYSQL_RES* result_hll = mysql_store_result(pConn);
			MYSQL_ROW row_hll = mysql_fetch_row(result_hll);
			mvprintw(iRow += 1, 1, "hll: %s", row_hll[0]);
			attrset(A_NORMAL);
			mysql_free_result(result_hll);

			if (iMDL != 1)
			{
				mysql_query(pConn, "SELECT ENABLED FROM performance_schema.setup_instruments WHERE NAME = 'wait/lock/metadata/sql/mdl'");
				MYSQL_RES* result_mdl = mysql_store_result(pConn);
				MYSQL_ROW row_mdl = mysql_fetch_row(result_mdl);
				mysql_free_result(result_mdl);

				if (strstr(row_mdl[0], "YES") != NULL)
				{
					iMDL = 1;
				}
				else
				{
					/* Attempt UPDATE of p_s instrumentation for versions 5.x, to avoid manually updating. */
					mysql_query(pConn, "UPDATE performance_schema.setup_instruments SET ENABLED = 'YES' WHERE NAME = 'wait/lock/metadata/sql/mdl'");
				}
			}
		}

		if (iPS == 0)
		{
			attrset(A_BOLD | COLOR_PAIR(4));
			mvprintw(iRow += 2, 1, "performance schema disabled");
			attrset(A_NORMAL);
		}

		iKey = getch();

		if (iKey == KEY_UP)
		{
			cDisplay = 'T'; /* trx */
		}
		else if (iKey == KEY_DOWN)
		{
			cDisplay = 'I'; /* innodb locks */
		}
		else if (iKey == KEY_LEFT)
		{
			cDisplay = 'L'; /* table lock waits */
		}
		else if (iKey == KEY_RIGHT)
		{
			cDisplay = 'M'; /* metadata locks */
		}

		if (iAccess == 0)
		{
			attrset(A_BOLD | COLOR_PAIR(4));
			mvprintw(iRow += 2, 1, "no user privilege access");
			attrset(A_NORMAL);
		}
		else if (cDisplay == 'T')
		{
			mysql_query(pConn, "\
				SELECT \
					thd.THREAD_ID, thd.PROCESSLIST_ID, stmt.ROWS_EXAMINED, trx.trx_rows_locked, trx.trx_rows_modified, stmt.ROWS_AFFECTED, stmt.CREATED_TMP_DISK_TABLES, trx.trx_tables_locked, stmt.NO_INDEX_USED, ROUND(stmt.TIMER_WAIT/1000000000000, 4), trx.trx_started, TO_SECONDS(NOW()) - TO_SECONDS(trx.trx_started), thd.PROCESSLIST_USER, trx.trx_state, trx.trx_operation_state, stmt.SQL_TEXT \
				FROM \
					information_schema.INNODB_TRX trx \
				INNER JOIN \
					performance_schema.threads thd ON thd.PROCESSLIST_ID = trx.trx_mysql_thread_id \
				INNER JOIN \
					performance_schema.events_statements_current stmt USING (THREAD_ID) \
			");

			MYSQL_RES* result_trx = mysql_store_result(pConn);
			MYSQL_ROW row_trx;

			iRow += 3;
			attrset(A_BOLD | COLOR_PAIR(2));
			mvprintw(iRow, 1, "transactions");
			attrset(A_NORMAL);
			iRow = 12;

			while ((row_trx = mysql_fetch_row(result_trx)))
			{
				if (row_trx != NULL)
				{
					char idx = (strcmp("1", row_trx[8]) == 1) ? 'N' : 'Y'; // NO_INDEX_USED -> reversal

					mvprintw(iRow, 1, "thd");
					mvprintw(iRow, 8, "ps");
					mvprintw(iRow, 18, "exm");
					mvprintw(iRow, 28, "lock");
					mvprintw(iRow, 38, "mod");
					mvprintw(iRow, 48, "afft");
					mvprintw(iRow, 58, "tmpd");
					mvprintw(iRow, 68, "tlk");
					mvprintw(iRow, 78, "idx");
					mvprintw(iRow, 88, "wait");
					mvprintw(iRow, 100, "start");
					mvprintw(iRow, 124, "sec");
					mvprintw(iRow, 135, "user");

					attrset(A_BOLD | COLOR_PAIR(1));
					iRow++;

					mvprintw(iRow, 1, row_trx[0]);
					mvprintw(iRow, 8, row_trx[1]);
					mvprintw(iRow, 18, row_trx[2]);
					mvprintw(iRow, 28, row_trx[3]);
					mvprintw(iRow, 38, row_trx[4]);
					mvprintw(iRow, 48, row_trx[5]);
					mvprintw(iRow, 58, row_trx[6]);
					mvprintw(iRow, 68, row_trx[7]);
					mvprintw(iRow, 78, "%c", idx);
					mvprintw(iRow, 88, row_trx[9]);
					mvprintw(iRow, 100, row_trx[10]);
					mvprintw(iRow, 124, row_trx[11]);
					mvprintw(iRow, 135, row_trx[12]);

					attrset(A_NORMAL);
					attron(COLOR_PAIR(5));
					mvprintw(iRow += 2, 1, row_trx[13]);
					attroff(COLOR_PAIR(5));

					attron(A_BOLD);

					if (row_trx[14] != NULL)
					{
						attron(COLOR_PAIR(2));
						mvprintw(iRow += 1, 1, row_trx[14]);
						attroff(COLOR_PAIR(2));
					}

					if (row_trx[15] != NULL)
					{
						attron(COLOR_PAIR(3));
						mvprintw(iRow += 1, 1, row_trx[15]);
						attroff(COLOR_PAIR(3));
					}

					attroff(A_BOLD);
				}

				iRow += 6;
			}

			mysql_free_result(result_trx);
		}
		else if (cDisplay == 'I')
		{
			mysql_query(pConn, "\
				SELECT \
					wait_age_secs, locked_table, locked_index, locked_type, waiting_trx_rows_locked, waiting_trx_rows_modified, waiting_query, waiting_lock_mode, blocking_pid, blocking_query, blocking_lock_mode, blocking_trx_age, blocking_trx_rows_locked, blocking_trx_rows_modified, sql_kill_blocking_query \
				FROM \
					sys.innodb_lock_waits \
			");

			MYSQL_RES* result_trx = mysql_store_result(pConn);
			MYSQL_ROW row_trx;

			iRow += 3;
			attrset(A_BOLD | COLOR_PAIR(2));
			mvprintw(iRow, 1, "innodb lock waits");
			attrset(A_NORMAL);
			iRow = 12;

			while ((row_trx = mysql_fetch_row(result_trx)))
			{
				if (row_trx != NULL)
				{
					mvprintw(iRow, 1, "locked table");
					iRow++;
					attrset(A_BOLD | COLOR_PAIR(1));
					mvprintw(iRow, 1, row_trx[1]);
					attrset(A_NORMAL);

					iRow += 2;
					mvprintw(iRow, 1, "wait/s");
					mvprintw(iRow, 10, "lkd index");
					mvprintw(iRow, 24, "lkd type");
					mvprintw(iRow, 37, "w-lkd");
					mvprintw(iRow, 47, "w-mod");
					mvprintw(iRow, 57, "w-mode");
					mvprintw(iRow, 67, "b-pid");
					mvprintw(iRow, 77, "b-mode");
					mvprintw(iRow, 87, "b-lkd");
					mvprintw(iRow, 100, "b-mod");
					mvprintw(iRow, 111, "b-time");

					iRow++;
					attrset(A_BOLD | COLOR_PAIR(1));

					mvprintw(iRow, 1, row_trx[0]);
					mvprintw(iRow, 10, row_trx[2]);
					mvprintw(iRow, 24, row_trx[3]);
					mvprintw(iRow, 37, row_trx[4]);
					mvprintw(iRow, 47, row_trx[5]);
					mvprintw(iRow, 57, row_trx[7]);
					mvprintw(iRow, 67, row_trx[8]);
					mvprintw(iRow, 77, row_trx[10]);
					mvprintw(iRow, 87, row_trx[12]);
					mvprintw(iRow, 100, row_trx[13]);
					mvprintw(iRow, 111, row_trx[11]);

					if (row_trx[6] != NULL)
					{
						attron(COLOR_PAIR(5));
						mvprintw(iRow += 2, 1, row_trx[6]);
						attroff(COLOR_PAIR(5));
					}

					if (row_trx[9] != NULL)
					{
						attron(COLOR_PAIR(5));
						mvprintw(iRow += 1, 1, row_trx[9]);
						attroff(COLOR_PAIR(5));
					}

					attrset(A_NORMAL);
					attron(COLOR_PAIR(5));
					mvprintw(iRow += 1, 1, row_trx[14]);
					attroff(COLOR_PAIR(5));
				}

				iRow += 3;
			}

			mysql_free_result(result_trx);
		}
		else if (cDisplay == 'L')
		{
			if (iMDL == 0)
			{
				attrset(A_BOLD | COLOR_PAIR(4));
				mvprintw(iRow += 2, 1, "p_s metadata lock instrumentation disabled");
				attrset(A_NORMAL);
			}

			mysql_query(pConn, "\
				SELECT \
					object_schema, object_name, waiting_account, waiting_lock_type, waiting_lock_duration, waiting_query, waiting_query_secs, waiting_query_rows_affected, waiting_query_rows_examined, blocking_pid, blocking_account, blocking_lock_type, blocking_lock_duration, sql_kill_blocking_query \
				FROM \
					sys.schema_table_lock_waits \
			");

			MYSQL_RES* result_trx = mysql_store_result(pConn);
			MYSQL_ROW row_trx;

			iRow += 3;
			attrset(A_BOLD | COLOR_PAIR(2));
			mvprintw(iRow, 1, "table lock waits");
			attrset(A_NORMAL);
			iRow = 12;

			while ((row_trx = mysql_fetch_row(result_trx)))
			{
				if (row_trx != NULL)
				{
					mvprintw(iRow, 1, "db");
					mvprintw(iRow, 22, "table");
					iRow++;
					attrset(A_BOLD | COLOR_PAIR(1));
					mvprintw(iRow, 1, row_trx[0]);
					mvprintw(iRow, 22, row_trx[1]);
					attrset(A_NORMAL);

					iRow += 2;
					mvprintw(iRow, 1, "wait user");
					mvprintw(iRow, 22, "wait lock");
					mvprintw(iRow, 46, "duration");
					mvprintw(iRow, 62, "sec");
					mvprintw(iRow, 70, "aff");
					mvprintw(iRow, 79, "exm");
					mvprintw(iRow, 89, "bpid");
					mvprintw(iRow, 97, "block user");
					mvprintw(iRow, 119, "block lock");
					mvprintw(iRow, 138, "duration");

					iRow++;
					attrset(A_BOLD | COLOR_PAIR(1));

					mvprintw(iRow, 1, row_trx[2]);
					mvprintw(iRow, 22, row_trx[3]);
					mvprintw(iRow, 46, row_trx[4]);
					mvprintw(iRow, 62, row_trx[6]);
					(row_trx[7] != NULL) ? mvprintw(iRow, 70, row_trx[7]) : mvprintw(iRow, 70, "-");
					(row_trx[8] != NULL) ? mvprintw(iRow, 79, row_trx[8]) : mvprintw(iRow, 79, "-");
					mvprintw(iRow, 89, row_trx[9]);
					mvprintw(iRow, 97, row_trx[10]);
					mvprintw(iRow, 119, row_trx[11]);
					mvprintw(iRow, 138, row_trx[12]);

					if (row_trx[5] != NULL)
					{
						attron(COLOR_PAIR(5));
						mvprintw(iRow += 2, 1, row_trx[5]);
						attroff(COLOR_PAIR(5));
					}

					attrset(A_NORMAL);

					attron(COLOR_PAIR(5));
					mvprintw(iRow += 1, 1, row_trx[13]);
					attroff(COLOR_PAIR(5));
				}

				iRow += 3;
			}

			mysql_free_result(result_trx);
		}
		else if (cDisplay == 'M')
		{
			if (iMDL == 0)
			{
				attrset(A_BOLD | COLOR_PAIR(4));
				mvprintw(iRow += 2, 1, "p_s metadata lock instrumentation disabled");
				attrset(A_NORMAL);
			}

			if (iV8 != 1) // v.5.7
			{
				mysql_query(pConn, "\
					SELECT \
						OBJECT_TYPE, OBJECT_SCHEMA, OBJECT_NAME, LOCK_TYPE, LOCK_DURATION, LOCK_STATUS, OWNER_THREAD_ID  \
					FROM \
						performance_schema.metadata_locks \
					WHERE \
						OBJECT_SCHEMA <> 'performance_schema' \
				");
					/* join on sys.session is simply too expensive: 50 QPS >> 2500+ QPS */

				MYSQL_RES* result_trx = mysql_store_result(pConn);
				MYSQL_ROW row_trx;

				iRow += 3;
				attrset(A_BOLD | COLOR_PAIR(2));
				mvprintw(iRow, 1, "metadata locks");
				attrset(A_NORMAL);
				iRow = 12;

				while ((row_trx = mysql_fetch_row(result_trx)))
				{
					if (row_trx != NULL)
					{
						mvprintw(iRow, 1, "db");
						mvprintw(iRow, 22, "table");
						mvprintw(iRow, 45, "obj");
						mvprintw(iRow, 65, "lock type");
						mvprintw(iRow, 95, "duration");
						mvprintw(iRow, 115, "status");
						mvprintw(iRow, 130, "id");

						iRow++;
						attrset(A_BOLD | COLOR_PAIR(1));

						(row_trx[1] != NULL) ? mvprintw(iRow, 1, row_trx[1]) : mvprintw(iRow, 1, "-");
						(row_trx[2] != NULL) ? mvprintw(iRow, 22, row_trx[2]) : mvprintw(iRow, 22, "-");
						mvprintw(iRow, 45, row_trx[0]);
						mvprintw(iRow, 65, row_trx[3]);
						mvprintw(iRow, 95, row_trx[4]);
						mvprintw(iRow, 115, row_trx[5]);
						mvprintw(iRow, 130, row_trx[6]);

						attrset(A_NORMAL);
					}

					iRow += 2;
				}

				mysql_free_result(result_trx);
			}
			else // v.8.0+
			{
				mysql_query(pConn, "\
					SELECT \
						OBJECT_TYPE, ML.OBJECT_SCHEMA, ML.OBJECT_NAME, ML.LOCK_TYPE, DL.LOCK_MODE, DL.LOCK_TYPE, ML.LOCK_DURATION, DL.LOCK_STATUS, ML.OWNER_THREAD_ID \
					FROM \
						performance_schema.metadata_locks ML \
					LEFT JOIN \
						performance_schema.data_locks DL ON DL.THREAD_ID = ML.OWNER_THREAD_ID \
					WHERE \
						ML.OBJECT_SCHEMA NOT IN ('information_schema', 'mysql', 'performance_schema') \
				");

				MYSQL_RES* result_trx = mysql_store_result(pConn);
				MYSQL_ROW row_trx;

				iRow += 3;
				attrset(A_BOLD | COLOR_PAIR(2));
				mvprintw(iRow, 1, "metadata locks");
				attrset(A_NORMAL);
				iRow = 12;

				while ((row_trx = mysql_fetch_row(result_trx)))
				{
					if (row_trx != NULL)
					{
						mvprintw(iRow, 1, "db");
						mvprintw(iRow, 22, "table");
						mvprintw(iRow, 40, "obj");
						mvprintw(iRow, 53, "type");
						mvprintw(iRow, 76, "mode");
						mvprintw(iRow, 98, "type");
						mvprintw(iRow, 109, "duration");
						mvprintw(iRow, 124, "status");
						mvprintw(iRow, 136, "id");

						iRow++;
						attrset(A_BOLD | COLOR_PAIR(1));

						(row_trx[1] != NULL) ? mvprintw(iRow, 1, row_trx[1]) : mvprintw(iRow, 1, "-");
						(row_trx[2] != NULL) ? mvprintw(iRow, 22, row_trx[2]) : mvprintw(iRow, 22, "-");
						mvprintw(iRow, 40, row_trx[0]);
						mvprintw(iRow, 53, row_trx[3]);
						(row_trx[4] != NULL) ? mvprintw(iRow, 76, row_trx[4]) : mvprintw(iRow, 76, "-");
						(row_trx[5] != NULL) ? mvprintw(iRow, 98, row_trx[5]) : mvprintw(iRow, 98, "-");
						mvprintw(iRow, 109, row_trx[6]);
						(row_trx[7] != NULL) ? mvprintw(iRow, 124, row_trx[7]) : mvprintw(iRow, 124, "-");
						mvprintw(iRow, 136, row_trx[8]);

						attrset(A_NORMAL);
					}

					iRow += 2;
				}

				mysql_free_result(result_trx);
			}
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

			case 't':
				iTime = (unsigned int) atoi(optarg);
				if (iTime < 10) {iTime = 10;}
				if (iTime > 2000) {iTime = 2000;}
				break;

			case 'p':
				iPort = (unsigned int) atoi(optarg);
				break;

			case '?':

				if (optopt == 'h' || optopt == 'w' || optopt == 'u' || optopt == 't' || optopt == 'p')
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
	fprintf(stdout, "\t%s -u <user> [-h <host>] [-t <time (ms)>] [-p <port>]\n\n", pFName);
}
