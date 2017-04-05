<?php

/**
	* Simple command-line script to query MySQL database table size properties, and if timestamps are available, provide an estimation of data fill rates.
	*
	* Usage:
	*        First ensure the database connection details and table column properties in the configuration section, then:
	*
	*        php -f table_sizer.php
	*
*/


##############################################
## CONFIGURATION
##############################################

interface IConfiguration {

    const

        HOST              =    '<host>',
        DATABASE          =    '<db>',
        DB_USERNAME       =    '<un>',
        DB_PASSWORD       =    '<pw>',

        TABLE             =    '<table>',
        DATE_COLUMN       =    '<col>',       # Unix timestamp column - to acquire the earliest and latest timestamps
        PRIMARY_KEY_ID    =    '<id>',        # need an ID in order to get start and end dates with SQL

        OUTPUT_IN_GBYTES  =    FALSE;         # GB or MB reporting
}

##############################################



final class TableDetails implements IConfiguration {

	/**
		* Query MySQL table size properties, and if timestamps are available, provide an estimation of data fill rates.
		*
		* Coded to PHP 5.5+
		*
		* @author        Martin Latter <copysense.co.uk>
		* @copyright     Martin Latter 06/03/2017
		* @version       0.08
		* @license       GNU GPL version 3.0 (GPL v3); http://www.gnu.org/licenses/gpl.html
		* @link          https://github.com/Tinram/MySQL.git
	*/


	private

		$oConnection = NULL,
		$bActiveConnection = FALSE,
		$aMessages = [],
		$iDatabaseSize = 0;


	public function __construct() {

		try {

			$this->oConnection = @new mysqli(self::HOST, self::DB_USERNAME, self::DB_PASSWORD, self::DATABASE);

			if ( ! $this->oConnection->connect_errno) {

				$this->bActiveConnection = TRUE;
				$this->init();
			}
			else { # unsuccessful DB connection

				throw new RuntimeException('Database connection failed: ' . $this->oConnection->connect_error . ' (error number: ' . $this->oConnection->connect_errno . ')');
			}
		}
		catch (RuntimeException $e) {
			echo $e;
		}

	} # end __contruct()


	public function __destruct() {

		if ($this->bActiveConnection) {
			$this->oConnection->close();
		}

	} # end __destruct()


	/**
		* Initialise: execution steps.
	*/

	private function init() {

		$bTableNameFail = $this->checkTableName();

		if ($bTableNameFail) {
			$this->outputMessages();
			return;
		}
		else {
			$this->getTableSize();
		}

		$bTableColFail = $this->checkTableColumns();

		if ($bTableColFail) {
			$this->outputMessages();
			return;
		}
		else {

			$this->calculate();
			$this->outputMessages();
		}

	} # end init()


	/**
		* Validate the existence of the specified table name in the specified database.
		*
		* @return  boolean, on success or failure to verify table name
	*/

	private function checkTableName() {

		$bFailure = FALSE;

		$sQuery = '
			SELECT TABLE_NAME
			FROM information_schema.TABLES
			WHERE
				TABLE_SCHEMA = "' . self::DATABASE . '"
			AND
				TABLE_NAME = "' . self::TABLE . '"';

		$rResult = $this->oConnection->query($sQuery);
		$aResult = $rResult->fetch_row();
		$rResult->close();

		$sTable = $aResult[0];

		if ($sTable !== self::TABLE) {

			$this->aMessages[] = 'ABORTED: the table \'' . self::TABLE . '\' does not exist in the database \'' . self::DATABASE . '\'!';
			$bFailure = TRUE;
		}

		return $bFailure;

	} # end checkTableName()


	/**
		* Acquire the size of the table in bytes and the row count.
	*/

	private function getTableSize() {

		$sQuery = '
			SELECT DATA_LENGTH
			FROM information_schema.TABLES WHERE TABLE_SCHEMA = ?
			ORDER BY DATA_LENGTH DESC';
				// full data: (data_length + index_length)

		$oStmt = $this->oConnection->stmt_init();
		$oStmt->prepare($sQuery);
		$sDatabaseName = self::DATABASE;
		$oStmt->bind_param('s', $sDatabaseName);
		$oStmt->execute();
		$oResult = $oStmt->get_result();
		$aResult = $oResult->fetch_row();
		$oStmt->free_result();
		$oStmt->close();

		$this->iDatabaseSize = $aResult[0];

		# get row count ('table_rows' from schema tables is only a rough estimate - avoided this)
		$sQuery = '
			SELECT COUNT(*)
			FROM ' . self::TABLE;

		$rResult = $this->oConnection->query($sQuery);
		$aResult = $rResult->fetch_row();
		$rResult->close();

		$this->aMessages[] = 'database:       ' . self::DATABASE;
		$this->aMessages[] = 'table:          ' . self::TABLE;
		$this->aMessages[] = 'table size:     ' . (self::OUTPUT_IN_GBYTES ? sprintf('%01.3f', ($this->iDatabaseSize / 1073741824)) . ' GB' : sprintf('%01.2f', $this->iDatabaseSize / 1048576) . ' MB') . ' (data, no indexes)';
		$this->aMessages[] = 'total rows:     ' . number_format($aResult[0]);

	} # end getTableSize()


	/**
		* Validate the existence of the specified columns PRIMARY_KEY_ID and DATE_COLUMN in table.
		*
		* @return  boolean, on success or failure to verify table columns
	*/

	private function checkTableColumns() {

		$bFailure = FALSE;

		$sQuery = '
			SELECT GROUP_CONCAT(COLUMN_NAME) AS cols
			FROM information_schema.COLUMNS
			WHERE TABLE_NAME = "' . self::TABLE . '"';

		$rResult = $this->oConnection->query($sQuery);
		$aResult = $rResult->fetch_row();
		$rResult->close();

		$sColumns = $aResult[0];

		if (strpos($sColumns, self::PRIMARY_KEY_ID) === FALSE) {

			$this->aMessages[] = PHP_EOL . 'ABORTED: the PRIMARY_KEY_ID \'' . self::PRIMARY_KEY_ID . '\' is not present in table \'' . self::TABLE . '\'!';
			$bFailure = TRUE;
		}

		if (strpos($sColumns, self::DATE_COLUMN) === FALSE) {

			$this->aMessages[] = PHP_EOL . 'ABORTED: the DATE_COLUMN \'' . self::DATE_COLUMN . '\' is not present in table \'' . self::TABLE . '\'!';
			$bFailure = TRUE;
		}

		return $bFailure;

	} # end checkTableColumns()


	/**
		* Acquire date range and make calculation.
	*/

	private function calculate() {

		# earliest row as a Unix timestamp; remove FROM_UNIXTIME() function if ISO timestamps are used
		$sQuery = '
			SELECT FROM_UNIXTIME(' . self::DATE_COLUMN . ')
			FROM ' . self::TABLE . '
			ORDER BY ' .self:: PRIMARY_KEY_ID . ' ASC
			LIMIT 1;';

		$rResult = $this->oConnection->query($sQuery);
		$aResult = $rResult->fetch_row();
		$rResult->close();

		$sStartDate = explode(' ', $aResult[0])[0];

		# last row
		$sQuery = '
			SELECT FROM_UNIXTIME(' . self::DATE_COLUMN . ')
			FROM ' . self::TABLE . '
			ORDER BY ' . self::PRIMARY_KEY_ID . ' DESC
			LIMIT 1;';

		$rResult = $this->oConnection->query($sQuery);
		$aResult = $rResult->fetch_row();
		$rResult->close();

		$sEndDate = explode(' ', $aResult[0])[0];

		# abort on unacquired empty date
		if (empty($sStartDate) || empty($sEndDate)) {
			$this->aMessages[] = PHP_EOL . 'ABORTED: a date range could not be acquired.';
			return;
		}

		# get time period: difference in days
		$aSD = explode('-', $sStartDate);
		$aED = explode('-', $sEndDate);

		$iS = cal_to_jd(CAL_GREGORIAN, $aSD[1], $aSD[2], $aSD[0]);
		$iE = cal_to_jd(CAL_GREGORIAN, $aED[1], $aED[2], $aED[0]);

		$iNumOfDays = $iE - $iS;

		# abort on zero, to avoid division by zero
		if ( ! $iNumOfDays) {
			$this->aMessages[] = 'ABORTED: a non-zero date range could not be acquired.';
			return;
		}

		$this->aMessages[] = 'day range:      ' . $iNumOfDays;

		# calculate data accumulation per day
		$fSizePerDay = $this->iDatabaseSize / $iNumOfDays;

		$this->aMessages[] = '>> data insertion rate: ' . (self::OUTPUT_IN_GBYTES ? sprintf('%01.4f', ($fSizePerDay / 1073741824)) . ' GB' : sprintf('%01.3f', ($fSizePerDay / 1048576)) . ' MB') . ' / day';

	} # end calculate()


	/**
		* Print accumulated message array.
	*/

	private function outputMessages() {
		echo PHP_EOL . join(PHP_EOL, $this->aMessages) . PHP_EOL;
	}

} # end {}


new TableDetails();


?>