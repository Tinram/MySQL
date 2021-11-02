<?php

declare(strict_types=1);

$aConfiguration =
[
    'query_database' => 'basketball',

    ##################################

    'host'     => 'localhost',
    'database' => 'information_schema',
    'username' => 'general',
    'password' => 'password',
];


$o = new FKShow($aConfiguration);
echo $o->displayMessages();


final class FKShow
{
    /**
        * Summary of primary/foreign key relationships of a database.
        *
        * @author         Martin Latter
        * @copyright      Martin Latter 26/10/2021
        * @version        0.01
        * @license        GNU GPL v3.0
        * @link           https://github.com/Tinram/MySQL.git
    */

    /** @var object $db */
    private $db;

    /** @var string $sEncoding, database connection encoding */
    private $sEncoding = 'utf8';

    /** @var boolean $bActiveConnection */
    private $bActiveConnection = false;

    /** @var array<array> $aFK, foreign keys */
    private $aFK = [];

    /** @var array<string|null> $aMessages */
    private $aMessages = [];

    /**
        * Constructor: set-up configuration class variables.
        *
        * @param   array<string> $aConfig, configuration details
    */
    public function __construct(array $aConfig)
    {
        if ( ! isset($aConfig['host']) || ! isset($aConfig['database']) || ! isset($aConfig['username']) || ! isset($aConfig['password']))
        {
            $this->aMessages[] = 'Database connection details are not fully specified in the configuration array.';
            return;
        }
        else
        {
            $this->db = new mysqli($aConfig['host'], $aConfig['username'], $aConfig['password'], $aConfig['database']);

            if ($this->db->connect_errno === 0)
            {
                $this->db->set_charset($this->sEncoding);
                $this->bActiveConnection = true;
            }
            else
            {
                $this->aMessages[] ='Database connection failed: ' . $this->db->connect_error . ' (error number: ' . $this->db->connect_errno . ')';
                return;
            }
        }

        $this->getFK($aConfig['query_database']);

        $this->processFK();
    }

    /**
        * Close database connection if active.
    */
    public function __destruct()
    {
        if ($this->bActiveConnection)
        {
            $this->db->close();
        }
    }

    /**
        * Get the foreign keys for the database.
        *
        * @param   string $sDatabase, database name
        *
        * @return  void
    */
    private function getFK(string $sDatabase): void
    {
        $sFKQuery = '
            SELECT
                kcu.TABLE_NAME "table",
                kcu.COLUMN_NAME "fk_column",
                kcu.REFERENCED_TABLE_NAME "ref_table",
                kcu.REFERENCED_COLUMN_NAME "ref_table_key",
                UNIQUE_CONSTRAINT_NAME "key_type"
            FROM
                information_schema.KEY_COLUMN_USAGE AS kcu
            INNER JOIN
                REFERENTIAL_CONSTRAINTS ON REFERENTIAL_CONSTRAINTS.CONSTRAINT_NAME = kcu.CONSTRAINT_NAME 
            WHERE
                kcu.REFERENCED_TABLE_SCHEMA = "' . $sDatabase . '"';

        $rR = $this->db->query($sFKQuery);

        $aInfo = $rR->fetch_all(MYSQLI_ASSOC);

        foreach ($aInfo as $aRow)
        {
            $this->aFK[$aRow['table']][] =
            [
                'fk_column' => $aRow['fk_column'],
                'ref_table' => $aRow['ref_table'],
                'ref_table_key' => $aRow['ref_table_key'],
                'key_type' => (($aRow['key_type'] === 'PRIMARY') ? 'PK' : 'FK')
            ];
        }
    }

    /**
        * Process and construct information.
        *
        * @return  void
    */
    private function processFK(): void
    {
        $this->aMessages[] = PHP_EOL;

        foreach ($this->aFK as $sTable => $aKeys)
        {
            //if ($sTable !== 'approvals') {continue;}

            $this->aMessages[] = strtoupper($sTable);

            foreach ($aKeys as $a)
            {
                if ($a['key_type'] === 'PK')
                {
                    $this->aMessages[] = "\t" . $a['fk_column'] . ' << '  . $a['ref_table'] . '.' . $a['ref_table_key'] . ' (' . $a['key_type']  . ')';
                }
                else
                {
                    $this->aMessages[] = "\t" . $a['fk_column'] . ' << ' . $a['ref_table'] . '.' . $a['ref_table_key'] . ' (' . $a['key_type']  . ')';
                }
            }

            $this->aMessages[] = PHP_EOL;
        }
    }


    /**
        * Getter for class array of messages.
        *
        * @return  string
    */
    public function displayMessages(): string
    {
        return join(PHP_EOL, $this->aMessages) . PHP_EOL;
    }
}
