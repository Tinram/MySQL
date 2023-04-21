<?php

/**
    * Simple command-line script to query MySQL database table size properties,
    * and if timestamps are available, provide an estimation of data fill rates.
    *
    * Usage:
    *        Ensure the database connection details and table column properties in the configuration section.
    *        php table_sizer.php
    *
*/


declare(strict_types=1);


##############################################
## CONFIGURATION
##############################################

interface IConfiguration
{
    const HOST              =    '<host>';
    const DATABASE          =    '<db>';
    const DB_USERNAME       =    '<un>';
    const DB_PASSWORD       =    '<pw>';

    const TABLE             =    '<table>';
    const DATE_COLUMN       =    '<col>';       # Unix timestamp column - to acquire the earliest and latest timestamps
    const PRIMARY_KEY_ID    =    '<id>';        # need an ID in order to get start and end dates with SQL

    const OUTPUT_IN_GBYTES  =    false;         # GB or MB reporting
}

##############################################



final class TableDetails implements IConfiguration
{
    /**
        * Query MySQL table size properties, and if timestamps are available, provide an estimation of data fill rates.
        *
        * Coded to PHP 7.2
        *
        * @author        Martin Latter
        * @copyright     Martin Latter 06/03/2017
        * @version       0.10
        * @license       GNU GPL version 3.0 (GPL v3); http://www.gnu.org/licenses/gpl.html
        * @link          https://github.com/Tinram/MySQL.git
    */


    /** @var object $oConnection */
    private $oConnection = null;

    /** @var boolean $bActiveConnection */
    private $bActiveConnection = false;

    /** @var array<string> $aMessages */
    private $aMessages = [];

    /** @var int $iDatabaseSize */
    private $iDatabaseSize = 0;


    public function __construct()
    {
        try
        {
            $this->oConnection = new mysqli(self::HOST, self::DB_USERNAME, self::DB_PASSWORD, self::DATABASE);

            if ( ! $this->oConnection->connect_errno)
            {
                $this->bActiveConnection = true;
                $this->init();
            }
            else
            {
                throw new RuntimeException('Database connection failed: ' . $this->oConnection->connect_error . ' (error number: ' . $this->oConnection->connect_errno . ')');
            }
        }
        catch (RuntimeException $e)
        {
            echo $e->getMessage();
        }

    }


    public function __destruct()
    {
        if ($this->bActiveConnection)
        {
            $this->oConnection->close();
        }
    }


    /**
        * Initialise: execution steps.
    */

    private function init(): void
    {
        $bTableNameFail = $this->checkTableName();

        if ($bTableNameFail)
        {
            $this->outputMessages();
            return;
        }
        else
        {
            $this->getTableSize();
        }

        $bTableColFail = $this->checkTableColumns();

        if ($bTableColFail)
        {
            $this->outputMessages();
            return;
        }
        else
        {
            $this->calculate();
            $this->outputMessages();
        }
    }


    /**
        * Validate the existence of the specified table name in the specified database.
        *
        * @return  boolean, on success or failure to verify table name
    */

    private function checkTableName()
    {
        $bFailure = false;

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

        if ($sTable !== self::TABLE)
        {
            $this->aMessages[] = 'ABORTED: the table \'' . self::TABLE . '\' does not exist in the database \'' . self::DATABASE . '\'!';
            $bFailure = true;
        }

        return $bFailure;
    }


    /**
        * Acquire the size of the table in bytes and the row count.
    */

    private function getTableSize(): void
    {
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
        $this->aMessages[] = 'total rows:     ' . number_format((float) $aResult[0]);
    }


    /**
        * Validate the existence of the specified columns PRIMARY_KEY_ID and DATE_COLUMN in table.
        *
        * @return  boolean, on success or failure to verify table columns
    */

    private function checkTableColumns(): bool
    {
        $bFailure = false;

        $sQuery = '
            SELECT GROUP_CONCAT(COLUMN_NAME) AS cols
            FROM information_schema.COLUMNS
            WHERE TABLE_NAME = "' . self::TABLE . '"';

        $rResult = $this->oConnection->query($sQuery);
        $aResult = $rResult->fetch_row();
        $rResult->close();

        $sColumns = $aResult[0];

        if (strpos($sColumns, self::PRIMARY_KEY_ID) === false)
        {
            $this->aMessages[] = PHP_EOL . 'ABORTED: the PRIMARY_KEY_ID \'' . self::PRIMARY_KEY_ID . '\' is not present in table \'' . self::TABLE . '\'!';
            $bFailure = true;
        }

        if (strpos($sColumns, self::DATE_COLUMN) === false)
        {
            $this->aMessages[] = PHP_EOL . 'ABORTED: the DATE_COLUMN \'' . self::DATE_COLUMN . '\' is not present in table \'' . self::TABLE . '\'!';
            $bFailure = true;
        }

        return $bFailure;
    }


    /**
        * Acquire date range and make calculation.
    */

    private function calculate(): void
    {
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
        if ($sStartDate === '' || $sEndDate === '')
        {
            $this->aMessages[] = PHP_EOL . 'ABORTED: a date range could not be acquired.';
            return;
        }

        # get time period: difference in days
        $aSD = explode('-', $sStartDate);
        $aED = explode('-', $sEndDate);
        $iS = cal_to_jd(CAL_GREGORIAN, (int) $aSD[1], (int) $aSD[2], (int) $aSD[0]);
        $iE = cal_to_jd(CAL_GREGORIAN, (int) $aED[1], (int) $aED[2], (int) $aED[0]);
        $iNumOfDays = $iE - $iS;

        # abort on zero, to avoid division by zero
        if ($iNumOfDays === 0)
        {
            $this->aMessages[] = 'ABORTED: a non-zero date range could not be acquired.';
            return;
        }

        $this->aMessages[] = 'day range:      ' . $iNumOfDays;

        # calculate data accumulation per day
        $fSizePerDay = $this->iDatabaseSize / $iNumOfDays;

        $this->aMessages[] = '>> data insertion rate: ' . (self::OUTPUT_IN_GBYTES ? sprintf('%01.4f', ($fSizePerDay / 1073741824)) . ' GB' : sprintf('%01.3f', ($fSizePerDay / 1048576)) . ' MB') . ' / day';
    }


    /**
        * Print accumulated message array.
    */

    private function outputMessages(): void
    {
        echo PHP_EOL . join(PHP_EOL, $this->aMessages) . PHP_EOL;
    }
}


new TableDetails();
