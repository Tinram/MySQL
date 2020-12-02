<?php

/**
    * Configuration file for TableBench class.
*/

declare(strict_types=1);

interface Configuration
{
    /* database connection credentials */

    const HOST = 'localhost';
    const DATABASE_NAME = 'table_bench';
    const TABLE_NAME = 'bench';

    const DATABASE_USERNAME = 'bencher';
    const DATABASE_PASSWORD = 'P@55w0rd';

    const DATABASE_CHARSET = 'utf-8';
    const DATABASE_ENGINE = 'mysqli'; # mysqli or pdo


    /* MySQL configuration */

    # number of table rows to add
    const NUM_ROWS = 50;

    # toggle disable keys before table operations
    const DISABLE_INDEXES = false;

    # toggle parameterized queries
    const PREPARED_STATEMENTS = false;

    # toggle transactions
    const TRANSACTIONS = false;


    /*
        * mysqld parameters: only possible with DATABASE_USERNAME = 'root'
        * other parameters require my.cnf editing.
    */

    const MAX_ALLOWED_PACKET = 268435456;

    const INNODB_FLUSH_LOG_AT_TRX_COMMIT = 2;


    public function DBConnect(): void;

    public function getResults(): string;
}
