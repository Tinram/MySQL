<?php

declare(strict_types=1);

class TableBench implements Configuration
{
    /**
        * Fill a pre-created table with junk data for benchmarking INSERTs.
        *
        * Coded to PHP 7.2
        *
        * Usage:
        *                $oTF = new TableBench();
        *                echo $oTF->getResults();
        *
        * @author        Martin Latter
        * @copyright     Martin Latter 04/01/2017
        * @version       0.05
        * @license       GNU GPL version 3.0 (GPL v3); http://www.gnu.org/licenses/gpl.html
        * @link          https://github.com/Tinram/MySQL.git
    */


    const LINE_BREAK = ((PHP_SAPI === 'cli') ? "\n" : '<br>');

    /** @var object $oConnection */
    private $oConnection;

    /** @var boolean $bActiveConnection, connection active flag */
    private $bActiveConnection = false;

    /** @var string $sDBEngine, database engine string holder to ensure lowercase */
    private $sDBEngine = '';

    /** @var array<string> $aResults, messages holder for output */
    private $aResults = [];


    public function __construct()
    {
        $this->sDBEngine = strtolower(self::DATABASE_ENGINE);
        $this->DBConnect();
        $this->populateTable();
    }


    /**
        * Close DB connection on script termination.
    */

    public function __destruct()
    {
        if ($this->bActiveConnection)
        {
            if ($this->sDBEngine === 'mysqli')
            {
                $this->oConnection->close();
            }
            else if ($this->sDBEngine === 'pdo')
            {
                $this->oConnection = null;
            }
        }
    }


    /**
        * Connect to DB and set connection character set.
        *
        * @return  void
    */

    public function DBConnect(): void
    {
        if ($this->sDBEngine === 'mysqli')
        {
            $this->oConnection = new mysqli(self::HOST, self::DATABASE_USERNAME, self::DATABASE_PASSWORD, self::DATABASE_NAME);

            if ($this->oConnection->connect_errno > 0)
            {
                die('MySQLi database connection failed:' . self::LINE_BREAK . $this->oConnection->connect_error . ' (error#: ' . $this->oConnection->connect_errno . ')');
            }
            else
            {
                $this->bActiveConnection = true;
                $this->oConnection->set_charset(self::DATABASE_CHARSET);
            }
        }
        else if ($this->sDBEngine === 'pdo')
        {
            try
            {
                $sPDOCharset = str_replace('-', '', self::DATABASE_CHARSET);
                $this->oConnection = new PDO("mysql:host=" . self::HOST . ";dbname=" . self::DATABASE_NAME . ";charset={$sPDOCharset}", self::DATABASE_USERNAME, self::DATABASE_PASSWORD);
                $this->oConnection->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
                $this->bActiveConnection = true;
            }
            catch (PDOException $e)
            {
                die('PDO database connection failed:' . self::LINE_BREAK . $e->getMessage() . self::LINE_BREAK);
            }
        }
    }


    /**
        * Create SQL query and populate table.
        *
        * @return  void
    */

    private function populateTable(): void
    {
        $aValues = [];
        $sAlpha = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ';
        $iOrderNo = mt_rand(1, 200000);

        $this->aResults[] = self::LINE_BREAK . 'TABLE BENCH';

        $this->aResults[] = self::LINE_BREAK . 'configuration:' . self::LINE_BREAK;

        $this->aResults[] = ' • using ' . $this->sDBEngine;

        # disable as many checks as possible
        $this->oConnection->query('SET unique_checks=0');
        $this->oConnection->query('SET foreign_key_checks=0');

        if ( ! self::PREPARED_STATEMENTS)
        {
            $this->aResults[] = ' • not using prepared statements';

            # create SQL query value sets, separating this from insertion code
            for ($i = 0; $i < self::NUM_ROWS; $i++)
            {
                $aValues[] = '(' . $iOrderNo++ . ',"' . substr(str_shuffle($sAlpha), 0, 15) . '","' . substr(str_shuffle($sAlpha), 0, 9) . '","' . hash('md5', substr(str_shuffle($sAlpha), 0, 31)) . '")';
            }

            # create SQL query string
            $sInsert = 'INSERT INTO ' . self::TABLE_NAME;
            $sInsert .= ' (order_no, origin, status_update, tracking_code)';
            $sInsert .= ' VALUES ' . join(',', $aValues);
        }
        else
        {
            $this->aResults[] = ' • using prepared statements';
        }

        if (self::DATABASE_USERNAME === 'root' && self::NUM_ROWS > 999)
        {
            $this->oConnection->query('SET GLOBAL max_allowed_packet = ' . self::MAX_ALLOWED_PACKET);
            $this->oConnection->query('SET GLOBAL innodb_flush_log_at_trx_commit = ' . self::INNODB_FLUSH_LOG_AT_TRX_COMMIT);
        }

        if (self::DISABLE_INDEXES)
        {
            $this->oConnection->query('LOCK TABLES ' . self::TABLE_NAME . ' WRITE');
            $this->oConnection->query('ALTER TABLE ' . self::TABLE_NAME . ' DISABLE KEYS');
            $this->aResults[] = ' • indexes disabled';
        }
        else
        {
            $this->aResults[] = ' • indexes enabled';
        }

        $fT1 = microtime(true);

        if (self::TRANSACTIONS)
        {
            if ($this->sDBEngine === 'mysqli')
            {
                $this->oConnection->begin_transaction();
            }
            else if ($this->sDBEngine === 'pdo')
            {
                $this->oConnection->beginTransaction();
            }

            $this->aResults[] = ' • using transactions';
        }
        else
        {
            $this->aResults[] = ' • not using transactions';
        }

        # simple query with parameter binding
        if ( ! self::PREPARED_STATEMENTS)
        {
            $rResult = $this->oConnection->query($sInsert);
        }
        else # parameterised query
        {
            if ($this->sDBEngine === 'mysqli')
            {
                $iON = $sOR = $sSU = $sTC = '';

                # create SQL parameterized query string
                $sInsert = 'INSERT INTO ' . self::TABLE_NAME;
                $sInsert .= ' (order_no, origin, status_update, tracking_code)';
                $sInsert .= ' VALUES (?, ?, ?, ?)';

                $oStmt = $this->oConnection->stmt_init();
                $oStmt->prepare($sInsert);
                $oStmt->bind_param('isss', $iON, $sOR, $sSU, $sTC);

                for ($i = 0; $i < self::NUM_ROWS; $i++)
                {
                    $iON = $iOrderNo++;
                    $sOR = substr(str_shuffle($sAlpha), 0, 15);
                    $sSU = substr(str_shuffle($sAlpha), 0, 9);
                    $sTC = hash('md5', substr(str_shuffle($sAlpha), 0, 31));
                    $oStmt->execute();
                }

                $rResult = ($i === self::NUM_ROWS) ? true : false;

                $oStmt->close();
            }
            else if ($this->sDBEngine === 'pdo')
            {
                $iON = $sOR = $sSU = $sTC = '';

                # create SQL parameterized query string
                $sInsert = 'INSERT INTO ' . self::TABLE_NAME;
                $sInsert .= ' (order_no, origin, status_update, tracking_code)';
                $sInsert .= ' VALUES (:orderNo, :origin, :statusUpdate, :trackingCode)';

                $oStmt = $this->oConnection->prepare($sInsert);
                $oStmt->bindParam(':orderNo', $iON, PDO::PARAM_INT);
                $oStmt->bindParam(':origin', $sOR, PDO::PARAM_STR);
                $oStmt->bindParam(':statusUpdate', $sSU, PDO::PARAM_STR);
                $oStmt->bindParam(':trackingCode', $sTC, PDO::PARAM_STR);

                for ($i = 0; $i < self::NUM_ROWS; $i++)
                {
                    $iON = $iOrderNo++;
                    $sOR = substr(str_shuffle($sAlpha), 0, 15);
                    $sSU = substr(str_shuffle($sAlpha), 0, 9);
                    $sTC = hash('md5', substr(str_shuffle($sAlpha), 0, 31));
                    $oStmt->execute();
                }

                $rResult = ($i === self::NUM_ROWS) ? true : false;

                $oStmt->closeCursor();
            }
        }

        if (self::TRANSACTIONS)
        {
            $this->oConnection->commit();
        }

        $fT2 = microtime(true);

        if (self::DISABLE_INDEXES)
        {
            $this->oConnection->query('ALTER TABLE ' . self::TABLE_NAME . ' ENABLE KEYS');
            $this->oConnection->query('UNLOCK TABLES');
        }

        if ($rResult)
        {
            $this->aResults[] = self::LINE_BREAK . 'added ' . self::NUM_ROWS . ' rows of data to table `' . self::TABLE_NAME . '`:';

            $fTimeElapsed = $fT2 - $fT1;

            $this->aResults[] = self::LINE_BREAK . '  SQL insertion time: ' . sprintf('%01.6f sec', $fTimeElapsed);
            $this->aResults[] = '  rows per sec: ' . sprintf('%d', self::NUM_ROWS / $fTimeElapsed) . self::LINE_BREAK . self::LINE_BREAK;
        }
        else
        {
            $this->aResults[] = 'There were ERRORS attempting to add ' . self::NUM_ROWS . ' rows of data to table ' . self::TABLE_NAME;
            $rResult = $this->oConnection->query('SHOW WARNINGS');
            $aErrors = $rResult->fetch_row();
            $rResult->close();
            $this->aResults[] = join(' | ', $aErrors);
        }
    }


    /**
        * Getter to output result string generated in populateTable()
        *
        * @return   string
    */

    public function getResults(): string
    {
        return join(self::LINE_BREAK, $this->aResults);
    }
}
