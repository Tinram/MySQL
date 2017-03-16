<?php

/**
	* Very simple install script to set-up the database for TableBench.
	* Define your values in the configuration section below, then execute on the command-line (php -f setup.php) or through your web server.
	*
	* @author        Martin Latter <copysense.co.uk>
	* @copyright     29/06/2014
	* @version       0.05
	* @license       GNU GPL version 3.0 (GPL v3); http://www.gnu.org/licenses/gpl.html
	* @link          https://github.com/Tinram/MySQL.git
*/


/* CONFIGURATION */

define('SUPER_USER', 'root');
define('SUPER_USER_PASSWORD', '**password**');

define('HOST', 'localhost');
define('DATABASE', 'table_bench');
define('TABLE', 'bench');

define('CHARSET', 'utf8');
define('COLLATION', 'utf8_general_ci');

define('APP_NAME', 'Table Bench');
define('APP_USERNAME', 'bencher');
define('APP_PASSWORD', 'password');

/* END CONFIGURATION */


# command-line or server line-break output
define('LINE_BREAK', (PHP_SAPI === 'cli') ? "\n" : '<br>');


$oConnection = new mysqli(HOST, SUPER_USER, SUPER_USER_PASSWORD);

if ($oConnection->connect_errno) {
	die('Database connection failed: ' . $oConnection->connect_errno . ') ' . $oConnection->connect_error . LINE_BREAK);
}
else {

	$sTitle = APP_NAME . ' Database Setup';

	if (PHP_SAPI !== 'cli') {
		echo '<h2>' . $sTitle . '</h2>';
	}
	else {
		echo $sTitle . "\n";
	}

	# create database
	$sQuery = 'CREATE DATABASE IF NOT EXISTS ' . DATABASE . ' CHARACTER SET ' . CHARSET . ' COLLATE ' . COLLATION;
	$rResults = $oConnection->query($sQuery);

	if ($rResults) {
		echo 'Created database ' . DATABASE . '.' . LINE_BREAK;
	}
	else {
		die('ERROR: could not create the ' . DATABASE . ' database.' . LINE_BREAK);
	}

	# select database
	$sQuery = 'USE ' . DATABASE;
	$rResults = $oConnection->query($sQuery);

	# create table
	$sQuery = '
		CREATE TABLE IF NOT EXISTS `' . TABLE . '` (
			`id` INT(10) UNSIGNED NOT NULL AUTO_INCREMENT,
			`order_no` INT(10) UNSIGNED NOT NULL,
			`origin` CHAR(16) NOT NULL DEFAULT "",
			`status_update` CHAR(10) NOT NULL DEFAULT "default",
			`tracking_code` CHAR(32) NOT NULL DEFAULT "",
			`added` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
			KEY `order_no` (`order_no`),
			PRIMARY KEY (`id`)
		) ENGINE=InnoDB DEFAULT CHARSET=' . CHARSET;

	$rResults = $oConnection->query($sQuery);

	if ($rResults) {
		echo 'Created table ' . TABLE . '.' . LINE_BREAK;
	}
	else {
		die('ERROR: could not create the ' . TABLE . ' table.' . LINE_BREAK);
	}

	# create grants to app user
	$sQuery = 'GRANT SELECT, INSERT, UPDATE, ALTER, LOCK TABLES ON ' . DATABASE . '.* TO "' . APP_USERNAME . '"@"' . HOST . '" IDENTIFIED BY "' . APP_PASSWORD . '"';
	$rResults = $oConnection->query($sQuery);

	if ($rResults) {
		echo 'Created ' . APP_NAME . ' database user.' . LINE_BREAK;
	}
	else {
		die('ERROR: could not create the required ' . APP_NAME . ' database user.' . LINE_BREAK);
	}

	# flush
	$sQuery = 'FLUSH PRIVILEGES';
	$rResults = $oConnection->query($sQuery);

	# close connection
	$oConnection->close();
}

?>