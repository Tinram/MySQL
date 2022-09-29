#!/usr/bin/env php
<?php

/**
    * PHP CLI script to minimise and tidy a MySQL schema from a mysqldump output file.
    *
    * Usage:
    *        php schema_cleaner.php <filename>
*/


declare(strict_types=1);

define('DUB_EOL', PHP_EOL . PHP_EOL);


if ( ! isset($_SERVER['argv'][1]))
{
    $sUsage =
        PHP_EOL . ' ' . str_replace('_', ' ', ucwords(basename(__FILE__, '.php'), '_')) .
        DUB_EOL . "\tusage: php " . basename(__FILE__) . ' <filename>' . DUB_EOL;
    die($sUsage);
}

$sFile = $_SERVER['argv'][1];

if ( ! file_exists($sFile))
{
    die(PHP_EOL . ' The file \'' . $sFile . '\' does not exist in this directory!' . DUB_EOL);
}
else
{
    $oSchemas = new SchemaCleaner($sFile);
    echo $oSchemas->displayMessages();
}


#######################################################################################


final class SchemaCleaner
{
    /**
        * Minimise large MySQL schema file dumps, stripping back to just table definitions.
        *
        * Coded to PHP 7.3
        *
        * @author          Martin Latter
        * @copyright       Martin Latter 23/09/2021
        * @version         0.03, from schema_splitter.php
        * @license         GNU GPL v3.0
        * @link            https://github.com/Tinram/MySQL.git
    */

    # CONFIGURATION #########################

    /** @var string $sStartTableMarker, schema start table marker */
    private $sStartTableMarker = 'DROP TABLE';

    /** @var string $sEndTableMarker, schema end table marker */
    private $sEndTableMarker = 'saved_cs_client */;';

    /** @var boolean $bDebug, debug toggle */
    private $bDebug = false;

    /** @var boolean $bRemoveAutoIncrement, toggle AUTO_INCREMENT string removal */
    private $bRemoveAutoIncrement = true;

    /** @var boolean $bRemoveCollates, toggle COLLATE definition removal */
    private $bRemoveCollates = true;

    /** @var boolean $bMatchCreateDropStatements, check that number of CREATE and DROP statements align in the schema */
    private $bMatchCreateDropStatements = false;

    #########################################

    /** @var string $sOutputFile, output file */
    private $sOutputFile = '';

    /** @var array<string> $aMessages, messages holder for output */
    private $aMessages = [];

    /** @var integer $iNumOfSchemaTables, number of schema tables to compare with number of files saved */
    private $iNumOfSchemaTables = 0;


    /**
        * Constructor: initialisation and process control.
        *
        * @param   string $sSchemaFilename, schema filename
    */

    public function __construct(string $sSchemaFilename = '')
    {
        if ($sSchemaFilename === '')
        {
            die(' ' . __METHOD__ . '(): No SQL schema file specified!');
        }

        $this->sOutputFile = $sSchemaFilename . '_cleaned.sql';

        $fT1 = microtime(true);
        $this->parseSchemaFile($sSchemaFilename);
        $fT2 = microtime(true);

        $this->wrapUp([$fT1, $fT2]);
    }


    /**
        * Parse SQL file to extract table schemas.
        *
        * @param   string $sSchemaFilename, schema filename
        *
        * @return  void
    */

    private function parseSchemaFile(string $sSchemaFilename): void
    {
        $aTableHolder = [];
        $aDropMatch = [];
        $aCreateMatch = [];

        # acquire SQL schema file
        $sFile = file_get_contents($sSchemaFilename);

        if ($sFile === false)
        {
            die(PHP_EOL . ' Schema file could not be opened.' . DUB_EOL);
        }

        # check for start marker presence
        if (strpos($sFile, $this->sStartTableMarker) === 0)
        {
            die(PHP_EOL . ' No specified start markers found in schema file!' . DUB_EOL);
        }

        # check for end marker presence
        if (strpos($sFile, $this->sEndTableMarker) === 0)
        {
            die(' No specified end markers found in schema file!' . DUB_EOL);
        }

        # check that number of CREATE and DROP statements match in that type of mysql dump schema
        if ($this->bMatchCreateDropStatements)
        {
            # find number of instances of 'DROP TABLE'
            $sDropPrefix = '/' . $this->sStartTableMarker . '/';
            preg_match_all($sDropPrefix, $sFile, $aDropMatch);

            # find number of instances of 'CREATE TABLE'
            preg_match_all('/CREATE TABLE/', $sFile, $aCreateMatch);

            # check count
            if (count($aDropMatch[0]) !== count($aCreateMatch[0]))
            {
                die(PHP_EOL . ' Number of DROP TABLE statements does not match number of CREATE TABLE statements in schema file.' . DUB_EOL);
            }
            else
            {
                $this->iNumOfSchemaTables = count($aCreateMatch[0]);
            }
        }
        else
        {
            # find number of instances of 'CREATE TABLE'
            preg_match_all('/CREATE TABLE/', $sFile, $aCreateMatch);
            $this->iNumOfSchemaTables = count($aCreateMatch[0]);
        }

        # create array of table info
        if ( ! $this->bDebug)
        {
            $n = $this->iNumOfSchemaTables;
        }
        else
        {
            $n = 2;
        }

        for ($i = 0, $iOffset = 0; $i < $n; $i++)
        {
            if ($iOffset === 0)
            {
                $iStart = strpos($sFile, $this->sStartTableMarker);
                $iEnd = strpos($sFile, $this->sEndTableMarker, $iStart);
            }
            else
            {
                $iStart = strpos($sFile, $this->sStartTableMarker, $iEnd);
                $iEnd = strpos($sFile, $this->sEndTableMarker, $iStart);
            }

            # N.B. strpos() is much faster than stripos() (1.5s vs 245s on 2MB schema file)

            $sTable = PHP_EOL;

            $sTable .= substr($sFile, $iStart, (($iEnd + ((strlen($this->sEndTableMarker)) - $iStart))));

            $sTable .= PHP_EOL;

            $iOffset = $iEnd;

            $aTableHolder[] = $sTable;
        }

        if ($this->bDebug)
        {
            print_r($aTableHolder);
        }

        $sFileOutput = PHP_EOL;

        # process each table string
        foreach ($aTableHolder as $sTable)
        {
            $iStart = strpos($sTable, 'CREATE TABLE');
            $iEnd = strpos($sTable, $this->sEndTableMarker, $iStart);
            $sTable = substr($sTable, $iStart, (($iEnd + ((strlen($this->sEndTableMarker)) - $iStart))));

            $iStart = strpos($sTable, '/*!');
            $sTable = substr($sTable, 0, $iStart);

            # remove autoincrement string
            if ($this->bRemoveAutoIncrement)
            {
                $sTable = preg_replace('/ AUTO_INCREMENT=[0-9]+/i', '', $sTable, 1);
            }

            # remove collate strings
            if ($this->bRemoveCollates)
            {
                $sTable = preg_replace('/COLLATE [\w]+ /', '', $sTable);
                $sTable = preg_replace('/COLLATE=[\w]+;/', ';', $sTable);
                $sTable = str_replace(' ;', ';', $sTable);
            }

            # add whitespace to improve readability
            $sTable = preg_replace('/(CREATE TABLE .+ \()/', '${1}' . "\n", $sTable, 1);
            $sTable = str_replace(') ENGINE', "\n) ENGINE", $sTable);
            $sTable = preg_replace('/(.+PRIMARY KEY \()/', "\n" . '${1}', $sTable, 1);

            # capitalise data types
            $aTypes = ['INT', 'TINYINT', 'SMALLINT', 'MEDIUMINT', 'BIGINT', 'FLOAT', 'DOUBLE', 'DECIMAL', 'CHAR', 'VARCHAR', 'TEXT', 'TINYTEXT', 'MEDIUMTEXT', 'LONGTEXT', 'BLOB', 'MEDIUMBLOB', 'LONGBLOB', 'BINARY', 'VARBINARY', 'DATE', 'DATETIME', 'TIME', 'TIMESTAMP', 'YEAR', 'BIT', 'BOOLEAN', 'ENUM', 'UNSIGNED'];

            foreach ($aTypes as $sType)
            {
                $sTable = str_ireplace(' ' . $sType, ' ' . $sType, $sTable);
            }

            $sFileOutput .= $sTable . DUB_EOL . PHP_EOL;
        }

        $iFileWrite = file_put_contents($this->sOutputFile, $sFileOutput);

        if ($iFileWrite === false)
        {
            $this->aMessages[] = ' ' . $this->sOutputFile . ' could not be saved.';
        }
        else
        {
            $this->aMessages[] = ' ' . $this->sOutputFile . ' successfully saved.';
        }
    }


    /**
        * Create results messages.
        *
        * @param   array<float> $aTiming, start and finish times
        *
        * @return  void
    */

    private function wrapUp(array $aTiming): void
    {
        $this->aMessages[] = ' processing time: ' . sprintf('%01.6f sec', $aTiming[1] - $aTiming[0]) . PHP_EOL;
    }


    /**
        * Getter for class array of messages.
        *
        * @return   string
    */

    public function displayMessages(): string
    {
        return join(PHP_EOL, $this->aMessages);
    }
}
