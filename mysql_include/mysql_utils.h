
/**
	* mysql_utils.h
	*
	* Shared functions and variables for MySQL utilities.
	*
	* @author        Martin Latter
	* @copyright     Martin Latter, 17/07/2023
	* @version       0.01
	* @license       GNU GPL version 3.0 (GPL v3); https://www.gnu.org/licenses/gpl-3.0.html
	* @link          https://github.com/Tinram/MySQL.git
	*
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


void signalHandler(int iSig);
void assignHostname(MYSQL* pConn, char* const aHN, unsigned int iHLen);
void identifyMySQLVersion(MYSQL* pConn, char* const aV, char* const pMaria, unsigned int* pM, unsigned int* pV8, unsigned int iVLen);
void identifyAuroraVersion(MYSQL* pConn, char* const aAV, char* const aASID, unsigned int* pAurora, unsigned int iAVLen, unsigned int iASIDLen);
void checkPerfSchema(MYSQL* pConn, unsigned int* pPS);
void replaceChar(char* const aSQL, char const cOrg, char const cRep);
int msSleep(unsigned int ms);
unsigned int options(int iArgCount, char* const aArgV[]);
void menu(char* const pFName);


char* pHost = NULL;
char* pUser = NULL;
char* pPassword = NULL;
char* pLogfile = NULL;
char* pProgname = NULL;
char* const pRoot = "root";

unsigned int iSigCaught = 0;
unsigned int iPort = 3306;
