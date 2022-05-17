
/**
	* MySQL Mon
	* mysqlmon.c
	*
	* MySQLd monitor.
	*
	* @author        Martin Latter
	* @copyright     Martin Latter, 06/11/2020
	* @version       0.16
	* @license       GNU GPL version 3.0 (GPL v3); https://www.gnu.org/licenses/gpl-3.0.html
	* @link          https://github.com/Tinram/MySQL.git
	*
	* Compile:
	* (Linux GCC x64)
	*                required dependencies: libmysqlclient-dev, libncurses5-dev
	*                gcc mysqlmon.c $(mysql_config --cflags) $(mysql_config --libs) -o mysqlmon -lncurses -Ofast -Wall -Wextra -Wuninitialized -Wunused -Werror -std=gnu99 -s
	*
	* Usage:
	*                ./mysqlmon --help
	*                ./mysqlmon -u <username> [-h <host>] [-p <port>]
*/


#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <curses.h>
#include <getopt.h>
#include <mysql.h>


#define APP_NAME "MySQL Mon"
#define MB_VERSION "0.16"


void menu(char* const pFName);
unsigned int options(int iArgCount, char* aArgV[]);
void signal_handler(int iSig);


char* pHost = NULL;
char* pUser = NULL;
char* pPassword = NULL;
char* pProgname = NULL;
unsigned int iPort = 3306;
unsigned int iSigCaught = 0;


/**
	* Sigint handling based on an example by Greg Kemnitz.
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
	const char* pMaria = "MariaDB";
	unsigned int iMenu = options(iArgCount, aArgV);
	unsigned int iAccess = 0;
	unsigned int iMaria = 0;
	signed int iOldQueries = 0;
	signed int iQueries = 0;

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

	/* Identify MariaDB which does not natively possess sys schema. */
	mysql_query(pConn, "SHOW variables WHERE Variable_name = 'version'");
	MYSQL_RES *result_ver = mysql_store_result(pConn);
	MYSQL_ROW row_ver = mysql_fetch_row(result_ver);
	if (strstr(row_ver[1], pMaria) != NULL)
	{
		iMaria = 1;
	}
	mysql_free_result(result_ver);

	mysql_query(pConn, "SHOW GLOBAL STATUS WHERE Variable_name = 'Queries'"); /* total queries, not conn questions */
	MYSQL_RES *result_queries = mysql_store_result(pConn);
	MYSQL_ROW row_queries = mysql_fetch_row(result_queries);
	iOldQueries = atoi(row_queries[1]);
	mysql_free_result(result_queries);

	start_color();
	init_pair(1, COLOR_GREEN, 0);
	curs_set(0);

	while ( ! iSigCaught)
	{
		clear();

		printw("\n");
		mysql_query(pConn, "SHOW variables WHERE Variable_name = 'hostname'");
		MYSQL_RES *result_hn = mysql_store_result(pConn);
		MYSQL_ROW row_hn = mysql_fetch_row(result_hn);
		attron(A_BOLD);
		printw(" %s\n", row_hn[1]);
		attroff(A_BOLD);
		if (iMaria == 1)
		{
			printw(" %s\n", pMaria);
		}
		mysql_free_result(result_hn);
		printw("\n");

		mysql_query(pConn, "SHOW STATUS WHERE Variable_name = 'Threads_connected'");
		MYSQL_RES *result_tc = mysql_store_result(pConn);
		MYSQL_ROW row_tc = mysql_fetch_row(result_tc);
		attron(COLOR_PAIR(1));
		attron(A_BOLD);
		printw(" threads connected: %s\n", row_tc[1]);
		attroff(A_BOLD);
		attroff(COLOR_PAIR(1));
		mysql_free_result(result_tc);

		mysql_query(pConn, "SHOW STATUS WHERE Variable_name = 'Aborted_connects'");
		MYSQL_RES *result_acon = mysql_store_result(pConn);
		MYSQL_ROW row_acon = mysql_fetch_row(result_acon);
		printw(" aborted connects: %s\n", row_acon[1]);
		mysql_free_result(result_acon);

		mysql_query(pConn, "SHOW STATUS WHERE Variable_name = 'Aborted_clients'");
		MYSQL_RES *result_acl = mysql_store_result(pConn);
		MYSQL_ROW row_acl = mysql_fetch_row(result_acl);
		printw(" aborted clients: %s\n\n", row_acl[1]);
		mysql_free_result(result_acl);

		mysql_query(pConn, "SHOW STATUS WHERE Variable_name = 'Max_used_connections'");
		MYSQL_RES *result_muc = mysql_store_result(pConn);
		MYSQL_ROW row_muc = mysql_fetch_row(result_muc);
		attron(COLOR_PAIR(1));
		attron(A_BOLD);
		printw(" max used connections: %s\n", row_muc[1]);
		attroff(A_BOLD);
		attroff(COLOR_PAIR(1));
		mysql_free_result(result_muc);

		mysql_query(pConn, "SHOW GLOBAL VARIABLES WHERE Variable_name = 'max_connections'");
		MYSQL_RES *result_mc = mysql_store_result(pConn);
		MYSQL_ROW row_mc = mysql_fetch_row(result_mc);
		printw(" max connections: %s\n", row_mc[1]);
		mysql_free_result(result_mc);

		mysql_query(pConn, "SHOW STATUS WHERE Variable_name = 'Connection_errors_max_connections'");
		MYSQL_RES *result_cem = mysql_store_result(pConn);
		MYSQL_ROW row_cem = mysql_fetch_row(result_cem);
		printw(" max conns exceeded: %s\n\n", row_cem[1]);
		mysql_free_result(result_cem);

		mysql_query(pConn, "SHOW STATUS WHERE Variable_name = 'Threads_running'");
		MYSQL_RES *result_tr = mysql_store_result(pConn);
		MYSQL_ROW row_tr = mysql_fetch_row(result_tr);
		attron(COLOR_PAIR(1));
		attron(A_BOLD);
		printw(" threads running: %s\n", row_tr[1]);
		attroff(A_BOLD);
		attroff(COLOR_PAIR(1));
		mysql_free_result(result_tr);

		mysql_query(pConn, "SHOW VARIABLES WHERE Variable_name = 'thread_cache_size'");
		MYSQL_RES *result_tcs = mysql_store_result(pConn);
		MYSQL_ROW row_tcs = mysql_fetch_row(result_tcs);
		printw(" thread cache size: %s\n", row_tcs[1]);
		mysql_free_result(result_tcs);

		mysql_query(pConn, "SHOW STATUS WHERE Variable_name = 'Threads_cached'");
		MYSQL_RES *result_tca = mysql_store_result(pConn);
		MYSQL_ROW row_tca = mysql_fetch_row(result_tca);
		printw(" threads cached: %s\n", row_tca[1]);
		mysql_free_result(result_tca);

		mysql_query(pConn, "SHOW STATUS WHERE Variable_name = 'Threads_created'");
		MYSQL_RES *result_tcr = mysql_store_result(pConn);
		MYSQL_ROW row_tcr = mysql_fetch_row(result_tcr);
		printw(" threads created: %s\n\n", row_tcr[1]);
		mysql_free_result(result_tcr);

		mysql_query(pConn, "SHOW GLOBAL STATUS WHERE Variable_name = 'Created_tmp_tables'");
		MYSQL_RES *result_ctt = mysql_store_result(pConn);
		MYSQL_ROW row_ctt = mysql_fetch_row(result_ctt);
		printw(" tmp tables: %s\n", row_ctt[1]);
		mysql_free_result(result_ctt);

		mysql_query(pConn, "SHOW GLOBAL STATUS WHERE Variable_name = 'Created_tmp_disk_tables'");
		MYSQL_RES *result_ctdt = mysql_store_result(pConn);
		MYSQL_ROW row_ctdt = mysql_fetch_row(result_ctdt);
		printw(" tmp disk tables: %s\n", row_ctdt[1]);
		mysql_free_result(result_ctdt);

		mysql_query(pConn, "SHOW GLOBAL STATUS WHERE Variable_name = 'Sort_merge_passes'");
		MYSQL_RES *result_smp = mysql_store_result(pConn);
		MYSQL_ROW row_smp = mysql_fetch_row(result_smp);
		printw(" sort merge passes: %s\n\n", row_smp[1]);
		mysql_free_result(result_smp);

		/* TRX at MySQL layer */
		mysql_query(pConn, "SELECT COUNT(*) FROM performance_schema.events_transactions_current WHERE state = 'ACTIVE' AND timer_wait > 1000000000000 * 1");
			/* requires performance_schema setup_consumers.events_transactions_current and setup_instruments.transaction to be enabled */
		MYSQL_RES *result_trx1 = mysql_store_result(pConn);

		if (mysql_errno(pConn) == 0) /* upon access denied error, set limit for less privileged users */
		{
			iAccess = 1;
		}

		if (iAccess == 1)
		{
			MYSQL_ROW row_trx1 = mysql_fetch_row(result_trx1);
			printw(" active trx (mysql): %s\n", row_trx1[0]);
		}

		mysql_free_result(result_trx1);

		if (iAccess == 1)
		{
			/* TRX at InnoDB layer */
			mysql_query(pConn, "SELECT COUNT(*) FROM information_schema.INNODB_TRX"); /* includes all trx_state: RUNNING, LOCK WAIT, ROLLING BACK, COMMITTING */
			MYSQL_RES *result_trx2 = mysql_store_result(pConn);
			MYSQL_ROW row_trx2 = mysql_fetch_row(result_trx2);
			printw(" active trx (innodb): %s\n", row_trx2[0]);
			mysql_free_result(result_trx2);

			/* History List Length */
			mysql_query(pConn, "SELECT COUNT FROM information_schema.INNODB_METRICS WHERE name = 'trx_rseg_history_len'");
			MYSQL_RES *result_hll = mysql_store_result(pConn);
			MYSQL_ROW row_hll = mysql_fetch_row(result_hll);
			attron(COLOR_PAIR(1));
			attron(A_BOLD);
			printw(" history list length: %s\n\n", row_hll[0]);
			attroff(A_BOLD);
			attroff(COLOR_PAIR(1));
			mysql_free_result(result_hll);
		}

		mysql_query(pConn, "SHOW GLOBAL STATUS WHERE Variable_name = 'Innodb_row_lock_current_waits'");
		MYSQL_RES *result_irlcw = mysql_store_result(pConn);
		MYSQL_ROW row_irlcw = mysql_fetch_row(result_irlcw);
		printw(" row lock current waits: %s\n", row_irlcw[1]);
		mysql_free_result(result_irlcw);

		mysql_query(pConn, "SHOW GLOBAL STATUS WHERE Variable_name = 'Innodb_row_lock_time'");
		MYSQL_RES *result_irlt = mysql_store_result(pConn);
		MYSQL_ROW row_irlt = mysql_fetch_row(result_irlt);
		signed int irlt = ((atoi(row_irlt[1])) / 1000);
		printw(" row lock time: %ds\n", irlt);
		mysql_free_result(result_irlt);

		mysql_query(pConn, "SHOW GLOBAL STATUS WHERE Variable_name = 'Innodb_row_lock_time_avg'");
		MYSQL_RES *result_irlta = mysql_store_result(pConn);
		MYSQL_ROW row_irlta = mysql_fetch_row(result_irlta);
		float irlta = ((atoi(row_irlta[1])) / 1000);
		printw(" row lock time avg: %2.1fs\n", irlta);
		mysql_free_result(result_irlta);

		mysql_query(pConn, "SHOW GLOBAL STATUS WHERE Variable_name = 'Innodb_row_lock_time_max'");
		MYSQL_RES *result_irltm = mysql_store_result(pConn);
		MYSQL_ROW row_irltm = mysql_fetch_row(result_irltm);
		signed int irltm = ((atoi(row_irltm[1])) / 1000);
		printw(" row lock time max: %ds\n", irltm);
		mysql_free_result(result_irltm);

		mysql_query(pConn, "SHOW GLOBAL STATUS WHERE Variable_name = 'Innodb_row_lock_waits'");
		MYSQL_RES *result_irlw = mysql_store_result(pConn);
		MYSQL_ROW row_irlw = mysql_fetch_row(result_irlw);
		printw(" row lock waits: %s\n", row_irlw[1]);
		mysql_free_result(result_irlw);

		if (iAccess == 1)
		{
			mysql_query(pConn, "SELECT COUNT FROM information_schema.INNODB_METRICS WHERE NAME = 'lock_timeouts'");
			MYSQL_RES *result_lto = mysql_store_result(pConn);
			MYSQL_ROW row_lto = mysql_fetch_row(result_lto);
			printw(" lock timeouts: %s\n", row_lto[0]);
			mysql_free_result(result_lto);

			mysql_query(pConn, "SELECT COUNT FROM information_schema.INNODB_METRICS WHERE NAME = 'lock_deadlocks'");
			MYSQL_RES *result_dl = mysql_store_result(pConn);
			MYSQL_ROW row_dl = mysql_fetch_row(result_dl);
			printw(" deadlocks: %s\n", row_dl[0]);
			mysql_free_result(result_dl);
		}

		mysql_query(pConn, "SHOW GLOBAL STATUS WHERE Variable_name = 'Innodb_rows_read'");
		MYSQL_RES *result_irr = mysql_store_result(pConn);
		MYSQL_ROW row_irr = mysql_fetch_row(result_irr);
		printw("\n rows read: %s\n", row_irr[1]);
		mysql_free_result(result_irr);

		mysql_query(pConn, "SHOW GLOBAL STATUS WHERE Variable_name = 'Innodb_rows_inserted'");
		MYSQL_RES *result_iri = mysql_store_result(pConn);
		MYSQL_ROW row_iri = mysql_fetch_row(result_iri);
		printw(" rows inserted: %s\n", row_iri[1]);
		mysql_free_result(result_iri);

		mysql_query(pConn, "SHOW GLOBAL STATUS WHERE Variable_name = 'Innodb_rows_updated'");
		MYSQL_RES *result_iru = mysql_store_result(pConn);
		MYSQL_ROW row_iru = mysql_fetch_row(result_iru);
		printw(" rows updated: %s\n", row_iru[1]);
		mysql_free_result(result_iru);

		mysql_query(pConn, "SHOW GLOBAL STATUS WHERE Variable_name = 'Innodb_rows_deleted'");
		MYSQL_RES *result_ird = mysql_store_result(pConn);
		MYSQL_ROW row_ird = mysql_fetch_row(result_ird);
		printw(" rows deleted: %s\n\n", row_ird[1]);
		mysql_free_result(result_ird);

		mysql_query(pConn, "SHOW GLOBAL STATUS WHERE Variable_name = 'Queries'"); /* total queries, not conn questions */
		MYSQL_RES *result_queries2 = mysql_store_result(pConn);
		MYSQL_ROW row_queries2 = mysql_fetch_row(result_queries2);
		iQueries = atoi(row_queries2[1]);
		/* in tests close to Innotop's QPS */
		signed int iDiff = iQueries - iOldQueries;
		iOldQueries = iQueries;
		attron(COLOR_PAIR(1));
		attron(A_BOLD);
		printw(" QPS: %u\n\n", iDiff);
		attroff(A_BOLD);
		attroff(COLOR_PAIR(1));
		mysql_free_result(result_queries2);

		if (iAccess == 1)
		{
			mysql_query(pConn, "SELECT ROUND(100 * (SELECT Variable_value FROM performance_schema.global_status WHERE Variable_name = 'Innodb_buffer_pool_pages_data') / (SELECT Variable_value FROM performance_schema.global_status WHERE Variable_name = 'Innodb_buffer_pool_pages_total'), 2)");
			MYSQL_RES *result_bpf = mysql_store_result(pConn);
			MYSQL_ROW row_bpf = mysql_fetch_row(result_bpf);
			printw(" BP pct fill: %s%\n", row_bpf[0]);
			mysql_free_result(result_bpf);
		}

		if (iAccess == 1 && iMaria == 0) /* block sys access to MariaDB */
		{
			mysql_query(pConn, "SELECT ROUND(100 - (100 * (SELECT Variable_value FROM sys.metrics WHERE Variable_name = 'Innodb_pages_read') / (SELECT Variable_value FROM sys.metrics WHERE Variable_name = 'Innodb_buffer_pool_read_requests')), 2)");
			MYSQL_RES *result_bphr = mysql_store_result(pConn);
			MYSQL_ROW row_bphr = mysql_fetch_row(result_bphr);
			printw(" BP hit rate: %s%\n", row_bphr[0]);
			mysql_free_result(result_bphr);

			mysql_query(pConn, "SELECT ROUND((Variable_value / 3600), 2) FROM sys.metrics WHERE Variable_name = 'uptime'");
			MYSQL_RES *result_ut = mysql_store_result(pConn);
			MYSQL_ROW row_ut = mysql_fetch_row(result_ut);
			printw(" uptime: %s hrs\n\n", row_ut[0]);
			mysql_free_result(result_ut);
		}

		refresh();

		sleep(1);
	}

	curs_set(1);

	endwin();

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

	while ((iOpts = getopt_long(iArgCount, aArgV, "ivh:w:u:p:", aLongOpts, &iOptsIdx)) != -1)
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

			case 'p':
				iPort = (unsigned int) atoi(optarg);
				break;

			case '?':

				if (optopt == 'h' || optopt == 'w' || optopt == 'u' || optopt == 'p')
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
	* @param   char* pFName, filename from aArgV[0]
	* @return  void
*/

void menu(char* const pFName)
{
	fprintf(stdout, "\n%s v.%s\nby Tinram", APP_NAME, MB_VERSION);
	fprintf(stdout, "\n\nUsage:\n");
	fprintf(stdout, "\t%s -u <user> [-h <host>] [-p <port>]\n\n", pFName);
}
