#!/usr/bin/env php
<?php

/**
    * PHP CLI script to extract a brief summary of table names and foreign keys from large schema files.
    *
    * Usage:
    *        php schema_summary.php <schema_file.sql> [keyword]
*/


declare(strict_types=1);

define('DUB_EOL', PHP_EOL . PHP_EOL);


if ( ! isset($_SERVER['argv'][1]))
{
    $sUsage =
        PHP_EOL . ' ' . str_replace('_', ' ', ucwords(basename(__FILE__, '.php'), '_')) .
        DUB_EOL . "\tusage: php " . basename(__FILE__) . ' <filename> [keyword]' . DUB_EOL;
    die($sUsage);
}

$sKeyword = (isset($_SERVER['argv'][2])) ? $_SERVER['argv'][2] : '';
$sFile = $_SERVER['argv'][1];

if ( ! file_exists($sFile))
{
    die(PHP_EOL . ' The file \'' . $sFile . '\' does not exist in this directory!' . DUB_EOL);
}
else
{
    $oSS = new SchemaSummary($sFile, $sKeyword);
    echo $oSS->displayMessages();
}


##############################################################################################


final class SchemaSummary
{
    /**
        * Parse large MySQL schema files to extract a brief summary of table names and foreign keys.
        *
        * Coded to PHP 7.2
        *
        * @author         Martin Latter
        * @copyright      Martin Latter 06/08/2020
        * @version        0.03, derived from Database Filler v.0.53
        * @license        GNU GPL v3.0
        * @link           https://github.com/Tinram/MySQL.git
    */


    /** @var array<string|null> $aMessages */
    private $aMessages = [];

    /** @var string $sSearchTerm, search term */
    private $sSearchTerm = '';

    /** @var boolean $bSearch, keyword search toggle */
    private $bSearch = false;

    /** @const INDENT, format string */
    const INDENT = '        ';


    /**
        * Constructor
        *
        * @param   string $sFile, SQL file
        * @param   string $sKeyword, keyword search term
    */

    public function __construct(string $sFile, string $sKeyword = '')
    {
        if ($sFile === '')
        {
            $this->aMessages[] = 'No schema file specified.';
            return;
        }

        if ($sKeyword !== '')
        {
            $this->sSearchTerm = $sKeyword;
            $this->bSearch = true;
        }

        $this->parseSQLFile($sFile);
    }


    /**
        * Parse SQL file to extract table schema.
        *
        * @param   string $sFileName, schema filename
        *
        * @return  void
    */

    private function parseSQLFile(string $sFileName): void
    {
        $aTableHolder = [];
        $aMatch = [];

        if ( ! file_exists($sFileName))
        {
            $this->aMessages[] = 'The schema file \'' . htmlentities(strip_tags($sFileName)) . '\' does not exist in this directory.';
            return;
        }

        # parse SQL schema
        $sFile = file_get_contents($sFileName);

        # find number of instances of 'CREATE TABLE'
        preg_match_all('/CREATE TABLE/', $sFile, $aMatch);

        # create array of table info
        for ($i = 0, $iOffset = 0, $n = count($aMatch[0]); $i < $n; $i++)
        {
            if ($iOffset === 0)
            {
                $iStart = stripos($sFile, 'CREATE TABLE');
                $iEnd = stripos($sFile, 'ENGINE=');
            }
            else
            {
                $iStart = stripos($sFile, 'CREATE TABLE', $iEnd);
                $iEnd = stripos($sFile, 'ENGINE=', $iStart);
            }

            $sTable = substr($sFile, $iStart, ($iEnd - $iStart));

            $iOffset = $iEnd;

            $aTableHolder[] = $sTable;
        }

        # send each table string for processing
        if ( ! $this->bSearch)
        {
            foreach ($aTableHolder as $sTable)
            {
                $this->processSQLTable($sTable);
            }
        }
        else
        {
            foreach ($aTableHolder as $sTable)
            {
                if (strpos($sTable, $this->sSearchTerm) !== false)
                {
                    $this->processSQLTable($sTable);
                }
            }
        }
    }


    /**
        * Process each table schema.
        *
        * @param   string $sTable, table schema string
        *
        * @return  void
    */

    private function processSQLTable(string $sTable): void
    {
        $aRXResults = [];
        $aLines = explode(PHP_EOL, $sTable);

        # extract table name
        preg_match('/`([\w\-]+)`/', $aLines[0], $aRXResults);
        $sTableName = $aRXResults[1];
        $this->aMessages[] = PHP_EOL . '+ ' . $sTableName;

        # extract foreign key lines
        $iFKStart = stripos($sTable, 'FOREIGN KEY');

        if ($iFKStart !== false)
        {
            $iFKEnd = strpos($sTable, PHP_EOL, $iFKStart); # EOL marker
            $sFKCont = substr($sTable, $iFKStart, $iFKEnd);
            $sFKCont = rtrim($sFKCont, ') ');
            $sFKCont = str_replace('  ', self::INDENT, $sFKCont);
            $this->aMessages[] = self::INDENT . $sFKCont;
        }
    }


    /**
        * Getter for class array of messages.
        *
        * @return  string
    */

    public function displayMessages(): string
    {
        return PHP_EOL . 'TABLES:' . PHP_EOL . join(PHP_EOL, $this->aMessages) . DUB_EOL;
    }
}
