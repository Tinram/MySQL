
/**
	* CSV Generator
	* csv_gen.c
	*
	* Generate a large CSV file as quickly as possible.
	*
	* @author        Martin Latter
	* @copyright     Martin Latter, 02/09/2022
	* @version       0.02
	* @license       GNU GPL version 3.0 (GPL v3); https://www.gnu.org/licenses/gpl-3.0.html
	* @link          https://github.com/Tinram/MySQL.git
	*
	* compile        gcc csv_gen.c -o csv_gen -Ofast -Wall -Wextra -Wuninitialized -Wunused -Werror -mtune=native -march=native -std=gnu99 -s
*/


#include <stdio.h>
#include <stdlib.h>
#include <time.h>


/* CONFIGURATION */
#define FILENAME "junk.csv"
#define FIELD_LEN 20
#define NUM_ROWS 10000000
#define NUM_STANDARD_FIELDS 3


int main()
{
	FILE* pFile;
	int iMSec = 0;
	clock_t tDiff = 0;
	clock_t tStart = 0;

	/* standard fields */
	struct buffer {
		char field[FIELD_LEN + 1];
	} b[NUM_STANDARD_FIELDS];

	/* 2-character field */
	char aCC[3];

	srand((unsigned int) time(NULL));

	pFile = fopen(FILENAME, "w");

	/* CSV header */
	fprintf(pFile, "firstname,lastname,country,country_code\n");

	for (unsigned int row = 0; row < NUM_ROWS; row++)
	{
		for (unsigned int i = 0; i < NUM_STANDARD_FIELDS; i++)
		{
			for (unsigned int j = 0; j < FIELD_LEN; j++)
			{
				b[i].field[j] = (rand() % 26) + 97;
			}
			b[i].field[FIELD_LEN] = '\0';
		}

		for (unsigned int k = 0; k < 2; k++)
		{
			aCC[k] = (rand() % 26) + 65;
		}
		aCC[2] = '\0';

		fprintf(pFile, "%s,%s,%s,%s\n", b[0].field, b[1].field, b[2].field, aCC);
	}

	/* timer display, Ben Alpert */
	tDiff = clock() - tStart;
	iMSec = tDiff * 1000 / CLOCKS_PER_SEC;
	printf("time: %d s %d ms\n", iMSec / 1000, iMSec % 1000);

	fclose(pFile);

	return 0;
}
