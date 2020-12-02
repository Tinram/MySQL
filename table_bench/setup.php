<?php

/**
    * Simple install script to set-up the MySQL database and database user for TableBench.
    * Define the constants values in the configuration section below, then execute via command-line: php setup.php
    * (or a web server).
    *
    * @author        Martin Latter
    * @copyright     29/06/2014
    * @version       0.06
    * @license       GNU GPL version 3.0 (GPL v3); http://www.gnu.org/licenses/gpl.html
    * @link          https://github.com/Tinram/MySQL.git
*/


/* CONFIGURATION */

define('SUPER_USER', 'root');
define('SUPER_USER_PASSWORD', '**password**');

define('APP_NAME', 'Table Bench');
define('APP_USERNAME', 'bencher');
define('APP_PASSWORD', 'P@55w0rd');

define('HOST', 'localhost');
define('DATABASE', 'table_bench');
define('TABLE', 'bench');

define('CHARSET', 'utf8');
define('COLLATION', 'utf8_general_ci');

/* END CONFIGURATION */


# command-line or server line-break output
define('LINE_BREAK', (PHP_SAPI === 'cli') ? "\n" : '<br>');


$oConnection = new mysqli(HOST, SUPER_USER, SUPER_USER_PASSWORD);

if ($oConnection->connect_errno > 0)
{
    die('Database connection failed: (' . $oConnection->connect_errno . ') ' . $oConnection->connect_error . LINE_BREAK);
}
else
{
    $sTitle = APP_NAME . ' Database Setup';

    if (PHP_SAPI !== 'cli')
    {
        echo '<h2>' . $sTitle . '</h2>';
    }
    else
    {
        echo $sTitle . LINE_BREAK . LINE_BREAK;
    }

    # create database
    $sQuery = 'CREATE DATABASE IF NOT EXISTS ' . DATABASE . ' CHARACTER SET ' . CHARSET . ' COLLATE ' . COLLATION;
    $rResults = $oConnection->query($sQuery);

    if ($rResults === true)
    {
        echo 'Created database ' . DATABASE . '.' . LINE_BREAK;
    }
    else
    {
        die('ERROR: could not create the ' . DATABASE . ' database.' . LINE_BREAK);
    }

    # select database
    $sQuery = 'USE ' . DATABASE;
    $rResults = $oConnection->query($sQuery);

    # create table
    $sQuery = '
        CREATE TABLE IF NOT EXISTS `' . TABLE . '`
        (
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

    if ($rResults === true)
    {
        echo 'Created table ' . TABLE . '.' . LINE_BREAK;
    }
    else
    {
        die('ERROR: could not create the ' . TABLE . ' table.' . LINE_BREAK);
    }

    # create user, avoiding errors if user was previously created
    $sQuery = 'CREATE USER IF NOT EXISTS "' . APP_USERNAME . '"@"' . HOST . '"';
    $rResults = $oConnection->query($sQuery);

    if ($rResults === true)
    {
        # MySQL 8 / user already exists on 5.x

        echo 'Created ' . APP_NAME . ' database user (' . APP_USERNAME . ').' . LINE_BREAK;

        # create CChat user password
        $sQuery = 'SET PASSWORD FOR "' . APP_USERNAME . '"@"' . HOST . '" = ' . '"' . APP_PASSWORD . '"';
        $rResults = $oConnection->query($sQuery);

        if ($rResults === true)
        {
            echo 'Created ' . APP_NAME . ' database user password.' . LINE_BREAK;
        }
        else
        {
            die('ERROR: could not create database user password (check password complexity requirements).' . LINE_BREAK);
        }

        # create CChat user permissions
        $sQuery = 'GRANT SELECT, INSERT ON ' . DATABASE . '.* TO "' . APP_USERNAME . '"@"' . HOST . '"';
        $rResults = $oConnection->query($sQuery);

        if ($rResults === true)
        {
            echo 'Created database user permissions.' . LINE_BREAK;
        }
        else
        {
            die('ERROR: could not create ' . APP_NAME . ' database user (' . APP_USERNAME . ') permissions.' . LINE_BREAK);
        }
    }
    else
    {
        # MySQL 5.x

        echo 'Could not create ' . APP_NAME . ' database user (' . APP_USERNAME . ') with method 1' . LINE_BREAK;
        echo 'trying method 2 ...' . LINE_BREAK;

        # for password requirements blocking
        $sQuery = 'GRANT SELECT, INSERT ON ' . DATABASE . '.* TO "' . APP_USERNAME . '"@"' . HOST . '" IDENTIFIED BY "' . APP_PASSWORD . '"';
        $rResults = $oConnection->query($sQuery);

        if ($rResults === true)
        {
            echo 'Created ' . APP_NAME . ' database user (' . APP_USERNAME . ') and permissions.' . LINE_BREAK;
        }
        else
        {
            die('ERROR: could not create the ' . APP_NAME . ' database user.' . LINE_BREAK);
        }
    }

    /*
        * Circumvent MySQL 8's sha256_password default authentication.
        * This is just to get operational on MySQL 8 with PHP version < 7.4, and is best replaced with caching_sha2_password authentication ASAP.
    */
    $sQuery = 'SELECT VERSION()';
    $rResults = $oConnection->query($sQuery);
    $aVersion = $rResults->fetch_row()[0];

    if (substr($aVersion, 0, 1) === '8')
    {
        $sQuery = 'ALTER USER "' . APP_USERNAME . '"@"' . HOST . '" IDENTIFIED WITH mysql_native_password BY "' . APP_PASSWORD . '"';
        $rResults = $oConnection->query($sQuery);

        if ($rResults === true)
        {
            echo 'Bypassed MySQL 8 sha256_password authentication.' . LINE_BREAK;
        }
        else
        {
            die('ERROR: could not bypass MySQL 8 sha256_password authentication.' . LINE_BREAK);
        }
    }

    # flush
    $sQuery = 'FLUSH PRIVILEGES';
    $rResults = $oConnection->query($sQuery);

    # close connection
    $oConnection->close();
}
