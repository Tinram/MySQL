<?php

/**
	* Configuration file for TableBench class.
*/


interface Configuration {

	const

		/* database connection details */

		HOST = 'localhost',
		DATABASE_NAME = 'table_bench',
		TABLE_NAME = 'bench',

		DATABASE_USERNAME = 'bencher',
		DATABASE_PASSWORD = 'password',

		DATABASE_CHARSET = 'utf-8',
		DATABASE_ENGINE = 'mysqli', # mysqli or pdo


		/* MySQL configuration */

		# number of table rows to add
		NUM_ROWS = 50,

		# toggle disable keys before table operations
		DISABLE_INDEXES = FALSE,

		# toggle parameterized queries
		PREPARED_STATEMENTS = FALSE,

		# toggle transactions
		TRANSACTIONS = FALSE,


		/* mysqld parameters - only possible with DATABASE_USERNAME = root */
		/* other parameters require my.cnf editing */

		MAX_ALLOWED_PACKET = 268435456,

		INNODB_FLUSH_LOG_AT_TRX_COMMIT = 2;


		public function DBConnect();


		public function getResults();
}

?>