
/**
	* MySQL Lock Monitor
	* mysqllockmon.c
	*
	* @author        Martin Latter
	* @copyright     Martin Latter, 06/07/2022
	* @version       0.26 (from mysqltrxmon)
	* @license       GNU GPL version 3.0 (GPL v3); https://www.gnu.org/licenses/gpl-3.0.html
	* @link          https://github.com/Tinram/MySQL.git
	*
	* Compile:
	* (Linux GCC x64)
	*                Required dependencies: libmysqlclient-dev, libncurses5-dev
	*                gcc mysqllockmon.c $(mysql_config --cflags) $(mysql_config --libs) -o mysqllockmon -I../mysql_include/ -lncurses -Ofast -Wall -Wextra -Wuninitialized -Wunused -Werror -std=gnu99 -s
	*
	* Usage:
	*                ./mysqllockmon --help
	*                ./mysqllockmon -u <username> [-h <host>] [-t <time (ms)>] [-p <port>]
*/


#include <mysql_utils.h>
#include <mysql_utils.c>


#define APP_NAME "MySQLLockMon"
#define MB_VERSION "0.26"


void displayTransactions(MYSQL* pConn, int* pRow);
void displayInnoDB(MYSQL* pConn, int* pRow);
void displayTableLockWaits(MYSQL* pConn, int* pRow, unsigned int* pMDL);
void displayMetadata(MYSQL* pConn, int* pRow, unsigned int* pMDL, unsigned int* pV8);


unsigned int iTime = 250; // millisecs


int main(int iArgCount, char* aArgV[])
{
	pProgname = aArgV[0];

	if (iArgCount <= 2)
	{
		menu(pProgname);
		return EXIT_FAILURE;
	}

	typedef enum
	{
		TRANSACTIONS,
		INNODB_LOCK_WAITS,
		TABLE_LOCK_WAITS,
		METADATA_LOCKS
	} LockType;

	MYSQL* pConn;
	char* const pMaria = "MariaDB";
	char aHostname[50];
	char aVersion[7];
	char aAuroraVersion[9];
	char aAuroraServerId[50];
	int iRow = 0;
	int iKey = 0;
	unsigned int iMenu = options(iArgCount, aArgV);
	unsigned int iV8 = 0;
	unsigned int iPS = 0;
	unsigned int iMaria = 0;
	unsigned int iAurora = 0;
	unsigned int iAccess = 0;
	unsigned int iMDL = 0;
	LockType displayChoice_t = TRANSACTIONS;

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
	//mysql_options(pConn, MYSQL_OPT_COMPRESS, 0);

	if (mysql_real_connect(pConn, pHost, pUser, pPassword, NULL, iPort, NULL, 0) == NULL)
	{
		fprintf(stderr, "\nCannot connect to MySQL server.\n(Error: %s)\n\n", mysql_error(pConn));
		return EXIT_FAILURE;
	}

	initscr();
	nodelay(stdscr, TRUE);
	keypad(stdscr, TRUE);

	/* Check for ncurses colour support. */
	if (has_colors() == FALSE)
	{
		endwin();
		fprintf(stderr, "\nThis terminal does not support colours.\n\n");
		mysql_close(pConn);
		return EXIT_FAILURE;
	}

	/* Assign hostname. */
	assignHostname(pConn, aHostname, sizeof(aHostname) - 1);

	/* Identify MySQL version. */
	identifyMySQLVersion(pConn, aVersion, pMaria, &iMaria, &iV8, sizeof(aVersion) - 1);

	/* Identify Aurora version, if applicable. */
	identifyAuroraVersion(pConn, aAuroraVersion, aAuroraServerId, &iAurora, sizeof(aAuroraVersion) - 1, sizeof(aAuroraServerId) - 1);

	/* Check performance schema availability. */
	checkPerfSchema(pConn, &iPS);

	if (iMaria == 1)
	{
		endwin();
		fprintf(stderr, "\nMariaDB is not supported.\n\n");
		mysql_close(pConn);
		return EXIT_FAILURE;
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
		iRow = 1;

		mvprintw(iRow, 1, APP_NAME);
		iRow += 2;

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

		if (iAurora == 0)
		{
			mvprintw(iRow += 1, 1, "%s", aVersion);
		}
		else
		{
			mvprintw(iRow += 1, 1, "%s (au: %s)", aVersion, aAuroraVersion);
		}

		/* TRX at InnoDB layer. */
		mysql_query(pConn, "SELECT COUNT(*) FROM information_schema.INNODB_TRX"); /* Includes RUNNING, LOCK WAIT, ROLLING BACK, COMMITTING */
		MYSQL_RES* result_acttr = mysql_store_result(pConn);

		if (mysql_errno(pConn) == 0)
		{
			iAccess = 1;
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

			if (iMDL != 1 && iPS == 1)
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

		mysql_free_result(result_acttr);


		/* The following works around ncurses loop peculiarities. */

		iKey = getch();

		if (iKey == KEY_UP)
		{
			displayChoice_t = TRANSACTIONS;
		}
		else if (iKey == KEY_DOWN)
		{
			displayChoice_t = INNODB_LOCK_WAITS;
		}
		else if (iKey == KEY_LEFT)
		{
			displayChoice_t = TABLE_LOCK_WAITS;
		}
		else if (iKey == KEY_RIGHT)
		{
			displayChoice_t = METADATA_LOCKS;
		}


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
			if (displayChoice_t == TRANSACTIONS)
			{
				displayTransactions(pConn, &iRow);
			}
			else if (displayChoice_t == INNODB_LOCK_WAITS)
			{
				displayInnoDB(pConn, &iRow);
			}
			else if (displayChoice_t == TABLE_LOCK_WAITS)
			{
				displayTableLockWaits(pConn, &iRow, &iMDL);
			}
			else if (displayChoice_t == METADATA_LOCKS)
			{
				displayMetadata(pConn, &iRow, &iMDL, &iV8);
			}
		}

		refresh();

		msSleep(iTime);
	}

	curs_set(1);

	endwin();

	mysql_close(pConn);

	return EXIT_SUCCESS;
}


/**
	* Transactions display.
	*
	* @param   MYSQL* pConn, connection pointer
	* @param   int* pRow, pointer to iRow
	* @return  void
*/

void displayTransactions(MYSQL* pConn, int* pRow)
{
	int iRow = *pRow;

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

	MYSQL_RES* result_q = mysql_store_result(pConn);
	MYSQL_ROW row_res;

	iRow += 3;
	attrset(A_BOLD | COLOR_PAIR(2));
	mvprintw(iRow, 1, "transactions");
	attrset(A_NORMAL);
	iRow = 12;

	while ((row_res = mysql_fetch_row(result_q)))
	{
		if (row_res != NULL)
		{
			char idx = (strcmp("1", row_res[8]) == 1) ? 'N' : 'Y'; // NO_INDEX_USED -> reversal

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

			mvprintw(iRow, 1, "%s", row_res[0]);
			mvprintw(iRow, 8, "%s", row_res[1]);
			mvprintw(iRow, 18, "%s", row_res[2]);
			mvprintw(iRow, 28, "%s", row_res[3]);
			mvprintw(iRow, 38, "%s", row_res[4]);
			mvprintw(iRow, 48, "%s", row_res[5]);
			mvprintw(iRow, 58, "%s", row_res[6]);
			mvprintw(iRow, 68, "%s", row_res[7]);
			mvprintw(iRow, 78, "%c", idx);
			mvprintw(iRow, 88, "%s", row_res[9]);
			mvprintw(iRow, 100, "%s", row_res[10]);
			mvprintw(iRow, 124, "%s", row_res[11]);
			mvprintw(iRow, 135, "%s", row_res[12]);

			attrset(A_NORMAL);
			attron(COLOR_PAIR(5));
			mvprintw(iRow += 2, 1, "%s", row_res[13]);
			attroff(COLOR_PAIR(5));

			if (row_res[14] != NULL)
			{
				attrset(A_BOLD | COLOR_PAIR(3));
				mvprintw(iRow += 1, 1, "%s", row_res[14]);
				attrset(A_NORMAL);
			}

			if (row_res[15] != NULL)
			{
				/* Truncate SQL at ~2 lines */
				char aQuery[300];
				unsigned int iQLen = sizeof(aQuery) - 1;
				strncpy(aQuery, row_res[15], iQLen);
				aQuery[iQLen] = '\0';

				/* Remove LFs. */
				replaceChar(aQuery, '\n', ' ');

				attron(COLOR_PAIR(2));
				mvprintw(iRow += 1, 1, "%s", aQuery);
				attroff(COLOR_PAIR(2));
			}
		}

		iRow += 4;
	}

	mysql_free_result(result_q);
}


/**
	* InnoDB lock waits display.
	*
	* @param   MYSQL* pConn, connection pointer
	* @param   int* pRow, pointer to iRow
	* @return  void
*/

void displayInnoDB(MYSQL* pConn, int* pRow)
{
	int iRow = *pRow;

	mysql_query(pConn, "\
		SELECT \
			wait_age_secs, locked_table, locked_index, locked_type, waiting_trx_rows_locked, waiting_trx_rows_modified, waiting_query, waiting_lock_mode, blocking_pid, blocking_query, blocking_lock_mode, blocking_trx_age, blocking_trx_rows_locked, blocking_trx_rows_modified, sql_kill_blocking_query \
		FROM \
			sys.innodb_lock_waits \
	");

	MYSQL_RES* result_q = mysql_store_result(pConn);
	MYSQL_ROW row_res;

	iRow += 3;
	attrset(A_BOLD | COLOR_PAIR(2));
	mvprintw(iRow, 1, "innodb lock waits");
	attrset(A_NORMAL);
	iRow = 12;

	while ((row_res = mysql_fetch_row(result_q)))
	{
		if (row_res != NULL)
		{
			mvprintw(iRow, 1, "locked table");
			iRow++;
			attrset(A_BOLD | COLOR_PAIR(1));
			mvprintw(iRow, 1, "%s", row_res[1]);
			attrset(A_NORMAL);

			iRow += 2;
			mvprintw(iRow, 1, "wait/s");
			mvprintw(iRow, 10, "lkd index");
			mvprintw(iRow, 24, "lkd type");
			mvprintw(iRow, 37, "w-lkd");
			mvprintw(iRow, 47, "w-mod");
			mvprintw(iRow, 57, "w-mode");
			mvprintw(iRow, 75, "b-pid");
			mvprintw(iRow, 85, "b-mode");
			mvprintw(iRow, 104, "b-lkd");
			mvprintw(iRow, 113, "b-mod");
			mvprintw(iRow, 126, "b-time");

			iRow++;
			attrset(A_BOLD | COLOR_PAIR(1));

			mvprintw(iRow, 1, "%s", row_res[0]);
			mvprintw(iRow, 10, "%s", row_res[2]);
			mvprintw(iRow, 24, "%s", row_res[3]);
			mvprintw(iRow, 37, "%s", row_res[4]);
			mvprintw(iRow, 47, "%s", row_res[5]);
			mvprintw(iRow, 57, "%s", row_res[7]);
			mvprintw(iRow, 75, "%s", row_res[8]);
			mvprintw(iRow, 85, "%s", row_res[10]);
			mvprintw(iRow, 104, "%s", row_res[12]);
			mvprintw(iRow, 113, "%s", row_res[13]);
			mvprintw(iRow, 126, "%s", row_res[11]);

			if (row_res[6] != NULL)
			{
				char aQuery[300];
				unsigned int iQLen = sizeof(aQuery) - 1;
				strncpy(aQuery, row_res[6], iQLen);
				aQuery[iQLen] = '\0';

				/* Remove LFs. */
				replaceChar(aQuery, '\n', ' ');

				attron(COLOR_PAIR(5));
				mvprintw(iRow += 2, 1, "%s", aQuery);
				attroff(COLOR_PAIR(5));
			}

			if (row_res[9] != NULL)
			{
				char aQuery[300];
				unsigned int iQLen = sizeof(aQuery) - 1;
				strncpy(aQuery, row_res[9], iQLen);
				aQuery[iQLen] = '\0';

				/* Remove LFs. */
				replaceChar(aQuery, '\n', ' ');

				attron(COLOR_PAIR(5));
				mvprintw(iRow += 1, 1, "%s", aQuery);
				attroff(COLOR_PAIR(5));
			}

			attrset(A_NORMAL);
			attron(COLOR_PAIR(5));
			mvprintw(iRow += 1, 1, "%s", row_res[14]);
			attroff(COLOR_PAIR(5));
		}

		iRow += 3;
	}

	mysql_free_result(result_q);
}


/**
	* Table lock waits display.
	*
	* @param   MYSQL* pConn, connection pointer
	* @param   int* pRow, pointer to iRow
	* @param   int* pRow, pointer to iMDL
	* @return  void
*/

void displayTableLockWaits(MYSQL* pConn, int* pRow, unsigned int* pMDL)
{
	int iRow = *pRow;

	if (*pMDL == 0)
	{
		attrset(A_BOLD | COLOR_PAIR(4));
		mvprintw(iRow += 3, 1, "p_s metadata lock instrumentation disabled");
		attrset(A_NORMAL);
		return;
	}

	mysql_query(pConn, "\
		SELECT \
			object_schema, object_name, waiting_account, waiting_lock_type, waiting_lock_duration, waiting_query, waiting_query_secs, waiting_query_rows_affected, waiting_query_rows_examined, blocking_pid, blocking_account, blocking_lock_type, blocking_lock_duration, sql_kill_blocking_query \
		FROM \
			sys.schema_table_lock_waits \
	");

	MYSQL_RES* result_q = mysql_store_result(pConn);
	MYSQL_ROW row_res;

	iRow += 3;
	attrset(A_BOLD | COLOR_PAIR(2));
	mvprintw(iRow, 1, "table lock waits");
	attrset(A_NORMAL);
	iRow = 12;

	while ((row_res = mysql_fetch_row(result_q)))
	{
		if (row_res != NULL)
		{
			mvprintw(iRow, 1, "db");
			mvprintw(iRow, 22, "table");
			iRow++;
			attrset(A_BOLD | COLOR_PAIR(1));
			mvprintw(iRow, 1, "%s", row_res[0]);
			mvprintw(iRow, 22, "%s", row_res[1]);
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

			mvprintw(iRow, 1, "%s", row_res[2]);
			mvprintw(iRow, 22, "%s", row_res[3]);
			mvprintw(iRow, 46, "%s", row_res[4]);
			mvprintw(iRow, 62, "%s", row_res[6]);
			(row_res[7] != NULL) ? mvprintw(iRow, 70, "%s", row_res[7]) : mvprintw(iRow, 70, "-");
			(row_res[8] != NULL) ? mvprintw(iRow, 79, "%s", row_res[8]) : mvprintw(iRow, 79, "-");
			mvprintw(iRow, 89, "%s", row_res[9]);
			mvprintw(iRow, 97, "%s", row_res[10]);
			mvprintw(iRow, 119, "%s", row_res[11]);
			mvprintw(iRow, 138, "%s", row_res[12]);

			if (row_res[5] != NULL)
			{
				char aQuery[300];
				unsigned int iQLen = sizeof(aQuery) - 1;
				strncpy(aQuery, row_res[5], iQLen);
				aQuery[iQLen] = '\0';

				/* Replace TABs and LFs. */
				replaceChar(aQuery, '\t', ' ');
				replaceChar(aQuery, '\n', ' ');

				attron(COLOR_PAIR(5));
				mvprintw(iRow += 2, 1, "%s", aQuery);
				attroff(COLOR_PAIR(5));
			}

			attrset(A_NORMAL);

			attron(COLOR_PAIR(5));
			mvprintw(iRow += 1, 1, "%s", row_res[13]);
			attroff(COLOR_PAIR(5));
		}

		iRow += 3;
	}

	mysql_free_result(result_q);
}


/**
	* Metadata lock waits display.
	*
	* @param   MYSQL* pConn, connection pointer
	* @param   int* pRow, pointer to iRow
	* @param   int* pV8, pointer to iV8
	* @return  void
*/

void displayMetadata(MYSQL* pConn, int* pRow, unsigned int* pMDL, unsigned int* pV8)
{
	int iRow = *pRow;

	if (*pMDL == 0)
	{
		attrset(A_BOLD | COLOR_PAIR(4));
		mvprintw(iRow += 3, 1, "p_s metadata lock instrumentation disabled");
		attrset(A_NORMAL);
		return;
	}

	if (*pV8 != 1) /* v.5.7 */
	{
		mysql_query(pConn, "\
			SELECT \
				OBJECT_TYPE, OBJECT_SCHEMA, OBJECT_NAME, LOCK_TYPE, LOCK_DURATION, LOCK_STATUS, OWNER_THREAD_ID  \
			FROM \
				performance_schema.metadata_locks \
			WHERE \
				OBJECT_SCHEMA <> 'performance_schema' \
		");
			/* Join on sys.session is simply too expensive: 50 QPS >> 2500+ QPS */

		MYSQL_RES* result_q = mysql_store_result(pConn);
		MYSQL_ROW row_res;

		iRow += 3;
		attrset(A_BOLD | COLOR_PAIR(2));
		mvprintw(iRow, 1, "metadata locks");
		attrset(A_NORMAL);
		iRow = 12;

		while ((row_res = mysql_fetch_row(result_q)))
		{
			if (row_res != NULL)
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

				(row_res[1] != NULL) ? mvprintw(iRow, 1, "%s", row_res[1]) : mvprintw(iRow, 1, "-");
				(row_res[2] != NULL) ? mvprintw(iRow, 22, "%s", row_res[2]) : mvprintw(iRow, 22, "-");
				mvprintw(iRow, 45, "%s", row_res[0]);
				mvprintw(iRow, 65, "%s", row_res[3]);
				mvprintw(iRow, 95, "%s", row_res[4]);
				mvprintw(iRow, 115, "%s", row_res[5]);
				mvprintw(iRow, 130, "%s", row_res[6]);

				attrset(A_NORMAL);
			}

			iRow += 2;
		}

		mysql_free_result(result_q);
	}
	else /* v.8.0+ */
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

		MYSQL_RES* result_q = mysql_store_result(pConn);
		MYSQL_ROW row_res;

		iRow += 3;
		attrset(A_BOLD | COLOR_PAIR(2));
		mvprintw(iRow, 1, "metadata locks");
		attrset(A_NORMAL);
		iRow = 12;

		while ((row_res = mysql_fetch_row(result_q)))
		{
			if (row_res != NULL)
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

				(row_res[1] != NULL) ? mvprintw(iRow, 1, "%s", row_res[1]) : mvprintw(iRow, 1, "-");
				(row_res[2] != NULL) ? mvprintw(iRow, 22, "%s", row_res[2]) : mvprintw(iRow, 22, "-");
				mvprintw(iRow, 40, "%s", row_res[0]);
				mvprintw(iRow, 53, "%s", row_res[3]);
				(row_res[4] != NULL) ? mvprintw(iRow, 76, "%s", row_res[4]) : mvprintw(iRow, 76, "-");
				(row_res[5] != NULL) ? mvprintw(iRow, 98, "%s", row_res[5]) : mvprintw(iRow, 98, "-");
				mvprintw(iRow, 109, "%s", row_res[6]);
				(row_res[7] != NULL) ? mvprintw(iRow, 124, "%s", row_res[7]) : mvprintw(iRow, 124, "-");
				mvprintw(iRow, 136, "%s", row_res[8]);

				attrset(A_NORMAL);
			}

			iRow += 2;
		}

		mysql_free_result(result_q);
	}
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

			case 't':
				iTime = (unsigned int) atoi(optarg);
				if (iTime < 100) {iTime = 100;}
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
	else if (pUser == NULL)
	{
		fprintf(stderr, "\n%s: use '%s -h for help\n\n", APP_NAME, aArgV[0]);
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
	fprintf(stdout, "\t%s -u <user> [-h <host>] [-t <time (ms)>] [-p <port>]\n\n", pFName);
}
