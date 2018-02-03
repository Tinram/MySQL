#!/usr/bin/env php
<?php

/**
	* PHP 7 CLI script to split a large MySQL schema file dump by individual tables into separate files.
	*
	* Usage:
	*        First ensure the class variables $sStartTableMarker and $sEndTableMarker contain
	*        the correct table start and finish identifiers in the schema file, then:
	*
	*        php schema_splitter.php <filename>
	*
	*        (or ./schema_splitter.php <filename>)
*/


declare(strict_types = 1); # reduces execution time by ~1 sec

define('DUB_EOL', PHP_EOL . PHP_EOL);


if ( ! isset($_SERVER['argv'][1])) {

	$sUsage =
			PHP_EOL . ' ' . str_replace('_', ' ', ucwords(basename(__FILE__, '.php'), '_')) .
			DUB_EOL . "\tusage: php " . basename(__FILE__) . ' <filename>' . DUB_EOL;

	die($sUsage);
}

$sFile = $_SERVER['argv'][1];

if ( ! file_exists($sFile)) {
	die(PHP_EOL . ' The file \'' . $sFile . '\' does not exist in this directory!' . DUB_EOL);
}
else {

	$oSchemas = new SchemaSplitter($sFile);
	echo $oSchemas->displayMessages();
}


#######################################################################################



final class SchemaSplitter {

	/**
		* Split a large MySQL schema file dump by individual tables into separate files.
		*
		* Coded for PHP 7.0+
		*
		* @author          Martin Latter <copysense.co.uk>
		* @copyright       Martin Latter 05/01/2017
		* @version         0.29
		* @license         GNU GPL v3.0
		* @link            https://github.com/Tinram/MySQL.git
	*/


	private

		# CONFIGURATION #########################

		# schema start table marker
		$sStartTableMarker = 'DROP TABLE',

		# schema end table marker
		$sEndTableMarker = 'saved_cs_client */;',

		# debug toggle
		$bDebug = FALSE,

		# check that number of CREATE and DROP statements align in the schema
		$bMatchCreateDropStatements = FALSE,

		# subdirectory for output files
		$sOutputDirPrefix = 'schema_files',

		#########################################

		$sOutputDir = '',

		# messages holder for output
		$aMessages = [],

		# number of files saved
		$iFileCounter = 0,

		# number of schema tables to compare with number of files saved
		$iNumOfSchemaTables = 0;


	/**
		* Initialisation and process control.
		*
		* @param    string $sSchemaFilename, schema filename
	*/

	public function __construct(string $sSchemaFilename = '') {

		if (empty($sSchemaFilename)) {
			die(' ' . __METHOD__ . '(): No SQL schema file specified!');
		}

		$this->sOutputDir = $this->sOutputDirPrefix . '_' . basename($sSchemaFilename, '.sql');

		if ( ! file_exists($this->sOutputDir)) {

			$bCreateDir = mkdir($this->sOutputDir, 0755);

			if ( ! $bCreateDir) {
				die(PHP_EOL . ' Script could not create the required file output directory.' . DUB_EOL);
			}
		}

		$fT1 = microtime(TRUE);
		$this->parseSchemaFile($sSchemaFilename);
		$fT2 = microtime(TRUE);

		$this->wrapUp([$fT1, $fT2]);

	} # end __construct()


	/**
		* Parse SQL file to extract table schemas.
		*
		* @param    string $sSchemaFilename, schema filename
	*/

	private function parseSchemaFile(string $sSchemaFilename) {

		$aTableHolder = [];
		$aDropMatch = [];
		$aCreateMatch = [];

		# acquire SQL schema file
		$sFile = file_get_contents($sSchemaFilename);

		# check for start marker presence
		if ( ! strpos($sFile, $this->sStartTableMarker)) {
			die(PHP_EOL . ' No specified start markers found in schema file!' . DUB_EOL);
		}

		# check for end marker presence
		if ( ! strpos($sFile, $this->sEndTableMarker)) {
			die(' No specified end markers found in schema file!' . DUB_EOL);
		}

		# check that number of CREATE and DROP statements match in that type of mysql dump schema
		if ($this->bMatchCreateDropStatements) {

			# find number of instances of 'DROP TABLE'
			$sDropPrefix = '/' . $this->sStartTableMarker . '/';
			preg_match_all($sDropPrefix, $sFile, $aDropMatch);

			# find number of instances of 'CREATE TABLE'
			preg_match_all('/CREATE TABLE/', $sFile, $aCreateMatch);

			# check count
			if (count($aDropMatch[0]) !== count($aCreateMatch[0])) {
				die(PHP_EOL . ' Number of DROP TABLE statements does not match number of CREATE TABLE statements in schema file.' . DUB_EOL);
			}
			else {
				$this->iNumOfSchemaTables = count($aCreateMatch[0]);
			}
		}
		else {

			# find number of instances of 'CREATE TABLE'
			preg_match_all('/CREATE TABLE/', $sFile, $aCreateMatch);
			$this->iNumOfSchemaTables = count($aCreateMatch[0]);
		}

		# create array of table info
		if ( ! $this->bDebug) {
			$n = $this->iNumOfSchemaTables;
		}
		else {
			$n = 2;
		}

		for ($i = 0, $iOffset = 0; $i < $n; $i++) {

			if ( ! $iOffset) {

				$iStart = strpos($sFile, $this->sStartTableMarker);
				$iEnd = strpos($sFile, $this->sEndTableMarker, $iStart);
			}
			else {

				$iStart = strpos($sFile, $this->sStartTableMarker, $iEnd);
				$iEnd = strpos($sFile, $this->sEndTableMarker, $iStart);
			}

			# N.B. strpos() is much faster than stripos()  (1.5s vs 245s on 2MB schema file)

			$sTable = PHP_EOL;

			$sTable .= substr($sFile, $iStart, (($iEnd + ((strlen($this->sEndTableMarker)) - $iStart))));

			$sTable .= PHP_EOL;

			$iOffset = $iEnd;

			$aTableHolder[] = $sTable;
		}

		if ($this->bDebug) {
			print_r($aTableHolder);
		}

		# send each table string for processing
		foreach ($aTableHolder as $sTable) {
			$this->processSQLTable($sTable);
		}

	} # end parseSQLFile()


	/**
		* Process each table schema and save to an individual schema file.
		*
		* @param    string $sTable, table schema string
	*/

	private function processSQLTable(string $sTable) {

		$aRXResults = [];

		preg_match('/CREATE TABLE `([a-zA-Z0-9\-_]+)`/', $sTable, $aRXResults);
		$sTableName = $aRXResults[1];
		$sTableFile = $sTableName . '.sql';

		# remove autoincrement string
		$sTable = preg_replace('/ AUTO_INCREMENT=[0-9]+/i', '', $sTable, 1);

		$iFileWrite = file_put_contents($this->sOutputDir . DIRECTORY_SEPARATOR . $sTableFile, $sTable);

		if ( ! $iFileWrite) {
			$this->aMessages[] = ' ' . $sTableFile . ' could not be saved.';
		}
		else {
			$this->iFileCounter++;
		}

	} # end processSQLTable()


	/**
		* Create results messages.
		*
		* @param    array $aTiming, start and finish times
	*/

	private function wrapUp(array $aTiming) {

		$this->aMessages[] = ' schema files saved: ' . $this->iFileCounter;

		if ($this->iNumOfSchemaTables !== $this->iFileCounter) {
			$this->aMessages[] = ' FAILURE: number of files saved does not match number of CREATE TABLE statements in schema file!' . PHP_EOL;
		}
		else {
			$this->aMessages[] = ' processing time: ' . sprintf('%01.6f sec', $aTiming[1] - $aTiming[0]) . PHP_EOL;
		}
	}


	/**
		* Getter for class array of messages.
		*
		* @return   string
	*/

	public function displayMessages(): string {

		return join(PHP_EOL, $this->aMessages);
	}

} # end {}

?>