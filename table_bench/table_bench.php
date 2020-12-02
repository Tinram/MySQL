#!/usr/bin/env php
<?php

/**
    * Execute TableBench class using values from configuration file.
*/

declare(strict_types=1);

require('config.php');
require('tablebench.class.php');


if (PHP_SAPI !== 'cli')
{
    header('Content-Type: text/html; charset=utf-8');
}

$oTF = new TableBench();
echo $oTF->getResults();
