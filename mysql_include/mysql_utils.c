
/**
	* mysql_utils.c
	*
	* Shared functions for MySQL utilities.
	*
	* @author        Martin Latter
	* @copyright     Martin Latter, 17/07/2023
	* @version       0.01
	* @license       GNU GPL version 3.0 (GPL v3); https://www.gnu.org/licenses/gpl-3.0.html
	* @link          https://github.com/Tinram/MySQL.git
	*
*/


/**
	* Sigint handling.
	* Based on example by Greg Kemnitz.
	*
	* @param   integer iSig
	* @return  void
*/

void signalHandler(int iSig)
{
	if (iSig == SIGINT || iSig == SIGTERM || iSig == SIGSEGV)
	{
		iSigCaught = 1;
	}
}

/**
	* Assign the hostname character array.
	*
	* @param   MYSQL* pConn, connection pointer
	* @param   char* aHN, pointer to aHostname
	* @param   unsigned int* iHLen, size of aHostname
	* @return  void
*/

void assignHostname(MYSQL* pConn, char* const aHN, unsigned int iHLen)
{
	mysql_query(pConn, "SELECT @@hostname");
	MYSQL_RES* result_hn = mysql_store_result(pConn);
	MYSQL_ROW row_hn = mysql_fetch_row(result_hn);
	strncpy(aHN, row_hn[0], iHLen);
	aHN[iHLen] = '\0';
	mysql_free_result(result_hn);
}


/**
	* Identify the server's MySQL version.
	*
	* @param   MYSQL* pConn, connection pointer
	* @param   char* aV, pointer to aVersion
	* @param   char* pMaria, pointer to MariaDB character string
	* @param   unsigned int* pM, pointer to iMaria
	* @param   unsigned int* pV8, pointer to iV8
	* @param   unsigned int iVLen, size of aVersion
	* @return  void
*/

void identifyMySQLVersion(MYSQL* pConn, char* const aV, char* const pMaria, unsigned int* pM, unsigned int* pV8, unsigned int iVLen)
{
	mysql_query(pConn, "SELECT @@version");
	MYSQL_RES* result_ver = mysql_store_result(pConn);
	MYSQL_ROW row_ver = mysql_fetch_row(result_ver);
	if (strstr(row_ver[0], pMaria) != NULL)
	{
		*pM = 1;
	}
	strncpy(aV, row_ver[0], iVLen);
	aV[iVLen] = '\0';
	if ((unsigned int) atoi(row_ver[0]) >= 8)
	{
		*pV8 = 1;
	}
	mysql_free_result(result_ver);
}


/**
	* If server is Aurora, identify Aurora version.
	*
	* @param   MYSQL* pConn, connection pointer
	* @param   char* aAV, pointer to aAuroraVersion
	* @param   char* aASID, pointer to aAuroraServerId
	* @param   unsigned int* pAurora, pointer to iAurora
	* @param   unsigned int iAVLen, size of aAuroraVersion
	* @param   unsigned int iASIDLen, size of aAuroraServerId
	* @return  void
*/

void identifyAuroraVersion(MYSQL* pConn, char* const aAV, char* const aASID, unsigned int* pAurora, unsigned int iAVLen, unsigned int iASIDLen)
{
	mysql_query(pConn, "SHOW VARIABLES WHERE Variable_name = 'aurora_version'");
	MYSQL_RES* result_aur_ver = mysql_store_result(pConn);
	MYSQL_ROW row_aur_ver = mysql_fetch_row(result_aur_ver);
	mysql_free_result(result_aur_ver);

	if (row_aur_ver != NULL)
	{
		*pAurora = 1;
		strncpy(aAV, row_aur_ver[1], iAVLen);
		aAV[iAVLen] = '\0';

		mysql_query(pConn, "SELECT @@aurora_server_id");
		MYSQL_RES* result_aur_sid = mysql_store_result(pConn);
		MYSQL_ROW row_aur_sid = mysql_fetch_row(result_aur_sid);
		mysql_free_result(result_aur_sid);

		strncpy(aASID, row_aur_sid[0], iASIDLen);
		aASID[iASIDLen] = '\0';
	}
}

/**
	* Check whether performance schema is enabled.
	*
	* @param   MYSQL* pConn, connection pointer
	* @param   unsigned int* pPS, pointer to iPS
	* @return  void
*/

void checkPerfSchema(MYSQL* pConn, unsigned int* pPS)
{
	mysql_query(pConn, "SELECT @@performance_schema");
	MYSQL_RES* result_ps = mysql_store_result(pConn);
	MYSQL_ROW row_ps = mysql_fetch_row(result_ps);

	if (strstr(row_ps[0], "1") != NULL)
	{
		*pPS = 1;
	}

	mysql_free_result(result_ps);
}

/**
	* Character replacer to clean multi-line SQL strings.
	* Credit: Fabio Cabral.
	*
	* @param   char* aSQL, pointer to aSQL character array
	* @param   char cOrg, original character
	* @param   char cRep, replacement character
	* @return  void
*/

void replaceChar(char* const aSQL, char const cOrg, char const cRep) {

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
