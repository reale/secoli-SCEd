#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>

#include "consular_district.h"
#include "consulate.h"
#include "location.h"
#include "queue.h"
#include "report.h"
#include "sce.h"
#include "search.h"  /* SCE_CODE_UNSET */
#include "utils.h"  /* bool */
#include "xalloc.h"
#include "xsqlite.h"
#include "zipcode.h"


// http://www.sqlite.org/cintro.html
// http://beej.us/guide/bgnet/
// http://cs.baylor.edu/~donahoo/practical/CSockets/textcode.html

// http://stackoverflow.com/questions/1711631/how-do-i-improve-the-performance-of-sqlite


/*
 * sqlite> EXPLAIN QUERY PLAN SELECT DISTINCT Losi.Id Sce_Code, Es.Es4 Cons_Mae_Code, 'EE' Province, '000000' Istat_Code, Losi.Nome Synonym, CASE WHEN LENGTH(Losi.Sinonimo) > 0 THEN Losi.Sinonimo ELSE Losi.Nome END Name FROM Es INNER JOIN Losi ON Losi.Sede = Es.Es1 INNER JOIN Pc ON Losi.Id = Pc2 WHERE Synonym LIKE 'LONDRA%';
 *
 * 0|0|0|SCAN TABLE Es (~1000000 rows)
 * 0|1|1|SEARCH TABLE Losi USING INDEX losi3 (sede=?) (~5 rows)
 * 0|2|2|SEARCH TABLE Pc USING COVERING INDEX pc2 (pc2=?) (~10 rows)
 * 0|0|0|USE TEMP B-TREE FOR DISTINCT
 *
 *
 * sqlite> CREATE VIEW Locations AS SELECT DISTINCT Losi.Id Sce_Code, 'aa' Cons_Mae_Code, 'EE' Province, '000000' Istat_Code, Losi.Nome Synonym, CASE WHEN LENGTH(Losi.Sinonimo) > 0 THEN Losi.Sinonimo ELSE Losi.Nome END Name FROM Losi INNER JOIN Pc ON Losi.Id = Pc2 ;
 */


static int
xsqlite_query_locations(sqlite3 *db, const char *sql, const char *name, const char *province, long sce_code, const char *zipcode, Queue *locations);

int
xsqlite_load_datafile(sqlite3 **db, const char *filename)
{
	/*
	 * From: http://www.sqlite.org/backup.html
	 *
	 * This function is used to load the contents of a database file on disk 
	 * into the "main" database of open database connection pInMemory, or
	 * to save the current contents of the database opened by pInMemory into
	 * a database file on disk. pInMemory is probably an in-memory database, 
	 * but this function will also work fine if it is not.
	 *
	 * Parameter filename points to a nul-terminated string containing the
	 * name of the database file on disk to load from or save to. If parameter
	 * isSave is non-zero, then the contents of the file filename are 
	 * overwritten with the contents of the database opened by pInMemory. If
	 * parameter isSave is zero, then the contents of the database opened by
	 * pInMemory are replaced by data loaded from the file filename.
	 *
	 * If the operation is successful, SQLITE_OK is returned. Otherwise, if
	 * an error occurs, an SQLite error code is returned.
	 *
	 */

	int rc;  /* return code */
	sqlite3 *dbMemory, *dbFile;  /* database connections */
	sqlite3_backup *backup;  /* backup object used to copy data */

	rc = sqlite3_open_v2(filename, &dbFile, SQLITE_OPEN_READONLY, NULL);

	if (rc == SQLITE_OK) {

		rc = sqlite3_open(":memory:", &dbMemory);

		if (rc == SQLITE_OK) {

			/* 
	 		 * From: http://www.sqlite.org/backup.html
			 *
			 * Set up the backup procedure to copy from the "main" database of 
			 * connection dbFile to the main database of connection pInMemory.
			 * If something goes wrong, backup will be set to NULL and an error
			 * code and  message left in connection pTo.
			 *
			 * If the backup object is successfully created, call backup_step()
			 * to copy data from dbFile to pInMemory. Then call backup_finish()
			 * to release resources associated with the backup object.  If an
			 * error occurred, then  an error code and message will be left in
			 * connection pTo. If no error occurred, then the error code belonging
			 * to pTo is set to SQLITE_OK.
			 *
			 */

			backup = sqlite3_backup_init(dbMemory, "main", dbFile, "main");

			if (backup) {
				sqlite3_backup_step(backup, -1);
				sqlite3_backup_finish(backup);
			}

			rc = sqlite3_errcode(dbMemory);
		}
	}

	/* close the connection on the datafile */
	sqlite3_close(dbFile);

	/* pass the in-memory connection to the caller */
	*db = dbMemory;

	return rc;
}

void
xsqlite_close_connection(sqlite3 *db)
{
	if (!db)
		return;

	sqlite3_close(db);
}

int
xsqlite_get_memory_used(void)
{
	/* http://www.sqlite.org/c3ref/status.html */

	int current, highwater;

	if (sqlite3_status(SQLITE_STATUS_MEMORY_USED, &current, &highwater, 0) == SQLITE_OK)
		return current;
	else
		return -1;
}

int
xsqlite_exec_ddl(sqlite3 *db, const char *ddl)
{
	char *error;

	if (!db || !ddl)
		return -1;

	if (sqlite3_exec(db, ddl, NULL, NULL, &error) != SQLITE_OK) {
		report(RPT_ERR, "%s(): sqlite3_exec() failed with error message: %s", __FUNCTION__, error);
		sqlite3_free(error);
		return -1;
	}

	return 0;
}


#if 0
int
xsqlite_query2(sqlite3 *db, const char *sql, const char *value, Queue *locations, const char *province)
{
	int rc;
	sqlite3_stmt *stmt;

	if (!db || !sql)
		return -1;

	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

	if (rc != SQLITE_OK) {
		report(RPT_WARNING, "%s: Selecting data from database connection failed: %s",
			__FUNCTION__, sqlite3_errmsg(db));
		return -1;
	}
	
	if (sqlite3_bind_text(stmt, 1, value, -1, NULL) != SQLITE_OK) {
		report(RPT_WARNING, "%s: Binding of prepared statement failed: %s",
			__FUNCTION__, sqlite3_errmsg(db));
		return -1;
	}
	
	/* fetch query result */

	while(1) {

		rc = sqlite3_step(stmt);

		if(rc == SQLITE_ROW) {

			int column_count;
			Location *loc;

			column_count = sqlite3_column_count(stmt);
			if (column_count < 1)
				continue;

			/* now we can instantiate the location */
			loc = location_create();
			/* the cast to char * is needed because sqlite3_column_text()
			   returns an unsigned char *  */
			location_set_cod_mae(loc, (const char *) sqlite3_column_text(stmt, 0));

			/* is the location's name given as well?  */
			if (column_count > 1)
				location_set_name(loc, (const char *) sqlite3_column_text(stmt, 1));

			/* add the location to the result lot */
			queue_push(locations, loc);

		} else if (rc == SQLITE_DONE) {
			break;

		} else {
			report(RPT_WARNING, "%s: Error in fetching data: %s",
					__FUNCTION__, sqlite3_errmsg(db));
			sqlite3_finalize(stmt);
			return -1;
		}
	}

	sqlite3_finalize(stmt);

	return 0;
}
#endif

#if 0
int
xsqlite_query(sqlite3 *db, const char *sql, const char *value, Queue *locations)
{
	int rc;
	sqlite3_stmt *stmt;

	if (!db || !sql)
		return -1;

	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

	if (rc != SQLITE_OK) {
		report(RPT_WARNING, "%s: Selecting data from database connection failed: %s",
			__FUNCTION__, sqlite3_errmsg(db));
		return -1;
	}
	
	if (sqlite3_bind_text(stmt, 1, value, -1, NULL) != SQLITE_OK) {
		report(RPT_WARNING, "%s: Binding of prepared statement failed: %s",
			__FUNCTION__, sqlite3_errmsg(db));
		return -1;
	}
	
	/* fetch query result */

	while(1) {

		rc = sqlite3_step(stmt);

		if(rc == SQLITE_ROW) {

			int column_count;
			Location *loc;

			column_count = sqlite3_column_count(stmt);
			if (column_count < 1)
				continue;

			/* now we can instantiate the location */
			loc = location_create();
			/* the cast to char * is needed because sqlite3_column_text()
			   returns an unsigned char *  */
			location_set_cod_mae(loc, (const char *) sqlite3_column_text(stmt, 0));

			/* is the location's name given as well?  */
			if (column_count > 1)
				location_set_name(loc, (const char *) sqlite3_column_text(stmt, 1));

			/* add the location to the result lot */
			queue_push(locations, loc);

		} else if (rc == SQLITE_DONE) {
			break;

		} else {
			report(RPT_WARNING, "%s: Error in fetching data: %s",
					__FUNCTION__, sqlite3_errmsg(db));
			sqlite3_finalize(stmt);
			return -1;
		}
	}

	sqlite3_finalize(stmt);

	return 0;
}
#endif

static int
xsqlite_query_locations(sqlite3 *db, const char *sql, const char *name, const char *province, long sce_code, const char *zipcode, Queue *locations)
{
	int rc;
	sqlite3_stmt *stmt;

	if (!db || !sql)
		return -1;

	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

	if (rc != SQLITE_OK) {
		report(RPT_WARNING, "%s: Selecting data from database connection failed: %s",
			__FUNCTION__, sqlite3_errmsg(db));
		return -1;
	}
	
	if (name) {
		if (sqlite3_bind_text(stmt, 1, name, -1, NULL) != SQLITE_OK) {
			report(RPT_WARNING, "%s: Binding of prepared statement failed: %s",
					__FUNCTION__, sqlite3_errmsg(db));
			return -1;
		}

		if (province) {
			if (sqlite3_bind_text(stmt, 2, province, -1, NULL) != SQLITE_OK) {
				report(RPT_WARNING, "%s: Binding of prepared statement failed: %s",
						__FUNCTION__, sqlite3_errmsg(db));
				return -1;
			}
		}
	} else if (sce_code != SCE_CODE_UNSET) {
		if (sqlite3_bind_int64(stmt, 1, sce_code) != SQLITE_OK) {
			report(RPT_WARNING, "%s: Binding of prepared statement failed: %s",
				__FUNCTION__, sqlite3_errmsg(db));
			return -1;
		}
	} else if (zipcode) {
		if (sqlite3_bind_text(stmt, 1, zipcode, -1, NULL) != SQLITE_OK) {
			report(RPT_WARNING, "%s: Binding of prepared statement failed: %s",
				__FUNCTION__, sqlite3_errmsg(db));
			return -1;
		}
	}
	
	/* fetch query result */

	while(1) {

		rc = sqlite3_step(stmt);

		if(rc == SQLITE_ROW) {

			int column_count;
			Location *loc;

			column_count = sqlite3_column_count(stmt);
			if (column_count != 8)
				continue;

			/* now we can instantiate the location */
			loc = location_create();

			/* the casts to char * are needed because sqlite3_column_text()
			   returns an unsigned char *  */

			/* Sce_Code */
			location_set_sce_code(loc, (const char *) sqlite3_column_text(stmt, 0));

			/* Cons_Mae_Code */
			location_set_cons_mae_code(loc, (const char *) sqlite3_column_text(stmt, 1));

			/* Zip_Code */
			if (zipcode)
				location_set_zip_code(loc, zipcode);
			else
				location_set_zip_code(loc, (const char *) sqlite3_column_text(stmt, 2));

			/* Province */
			location_set_province(loc, (const char *) sqlite3_column_text(stmt, 3));

			/* Istat_Code */
			location_set_istat_code(loc, (const char *) sqlite3_column_text(stmt, 4));

			/* Synonym */
			location_set_synonym(loc, (const char *) sqlite3_column_text(stmt, 5));

			/* Name */
			location_set_name(loc, (const char *) sqlite3_column_text(stmt, 6));

			/* Full name (name + synonym) */
			location_set_full_name(loc, (const char *) sqlite3_column_text(stmt, 7));

			/* add the location to the result */
			queue_push(locations, loc);

		} else if (rc == SQLITE_DONE) {
			break;

		} else {
			report(RPT_WARNING, "%s: Error in fetching data: %s",
					__FUNCTION__, sqlite3_errmsg(db));
			sqlite3_finalize(stmt);
			return -1;
		}
	}

	sqlite3_finalize(stmt);

	return 0;
}

int
xsqlite_query_consulates(sqlite3 *db, const char *sql, int cod_state, bool territorial_competence, Queue *consulates)
{
	int rc;
	sqlite3_stmt *stmt;

	if (!db || !sql)
		return -1;

	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

	if (rc != SQLITE_OK) {
		report(RPT_WARNING, "%s: Selecting data from database connection failed: %s",
			__FUNCTION__, sqlite3_errmsg(db));
		return -1;
	}
	
	if (!territorial_competence) {
		if (sqlite3_bind_int(stmt, 1, cod_state) != SQLITE_OK) {
			report(RPT_WARNING, "%s: Binding of prepared statement failed: %s",
				__FUNCTION__, sqlite3_errmsg(db));
			return -1;
		}
	}

	/* fetch query result */

	while(1) {

		rc = sqlite3_step(stmt);

		if(rc == SQLITE_ROW) {

			Consulate *cons;

			if (sqlite3_column_count(stmt) != 1)
				continue;

			cons = consulate_create();

			consulate_set_mae_code(cons, (const char *) sqlite3_column_text(stmt, 0));

			queue_push(consulates, cons);

		} else if (rc == SQLITE_DONE) {
			break;

		} else {
			report(RPT_WARNING, "%s: Error in fetching data: %s",
					__FUNCTION__, sqlite3_errmsg(db));
			sqlite3_finalize(stmt);
			return -1;
		}
	}

	sqlite3_finalize(stmt);

	return 0;
}

int
xsqlite_query_consular_districts(sqlite3 *db, const char *sql, Queue *consular_districts)
{
	int rc;
	sqlite3_stmt *stmt;

	if (!db || !sql)
		return -1;

	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

	if (rc != SQLITE_OK) {
		report(RPT_WARNING, "%s: Selecting data from database connection failed: %s",
			__FUNCTION__, sqlite3_errmsg(db));
		return -1;
	}
	
	/* fetch query result */

	while(1) {

		rc = sqlite3_step(stmt);

		if(rc == SQLITE_ROW) {

			ConsularDistrict *cons;

			if (sqlite3_column_count(stmt) != 3)
				continue;

			cons = consular_district_create();

			consular_district_set_mae_code(cons, (const char *) sqlite3_column_text(stmt, 0));
			consular_district_set_territory_name(cons, (const char *) sqlite3_column_text(stmt, 1));
			consular_district_set_sce_code(cons, (const char *) sqlite3_column_text(stmt, 2));

			queue_push(consular_districts, cons);

		} else if (rc == SQLITE_DONE) {
			break;

		} else {
			report(RPT_WARNING, "%s: Error in fetching data: %s",
					__FUNCTION__, sqlite3_errmsg(db));
			sqlite3_finalize(stmt);
			return -1;
		}
	}

	sqlite3_finalize(stmt);

	return 0;
}

int
xsqlite_query_zip_code_specimina(sqlite3 *db, const char *sql, Queue *zip_code_specimina)
{
	int rc;
	sqlite3_stmt *stmt;

	if (!db || !sql)
		return -1;

	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

	if (rc != SQLITE_OK) {
		report(RPT_WARNING, "%s: Selecting data from database connection failed: %s",
			__FUNCTION__, sqlite3_errmsg(db));
		return -1;
	}
	
	/* fetch query result */

	while(1) {

		rc = sqlite3_step(stmt);

		if(rc == SQLITE_ROW) {

			Zipcode *zipcode;

			if (sqlite3_column_count(stmt) != 2)
				continue;

			zipcode = zipcode_create();

			zipcode_set_cod_state(zipcode, (const char *) sqlite3_column_text(stmt, 0));
			zipcode_set_code(zipcode, (const char *) sqlite3_column_text(stmt, 1));

			queue_push(zip_code_specimina, zipcode);

		} else if (rc == SQLITE_DONE) {
			break;

		} else {
			report(RPT_WARNING, "%s: Error in fetching data: %s",
					__FUNCTION__, sqlite3_errmsg(db));
			sqlite3_finalize(stmt);
			return -1;
		}
	}

	sqlite3_finalize(stmt);

	return 0;
}

int
xsqlite_list_locations(sqlite3 *db, Location **locations, int locations_max, bool with_codmae)
{
	char *sql;

	sqlite3_stmt *stmt;
	int count;
	int rc;



	if (!db)
		return -1;

	if (with_codmae)
		sql = strdup("SELECT DISTINCT es.es4, CASE WHEN LENGTH(losi.sinonimo) > 0 THEN losi.sinonimo ELSE losi.nome END FROM es INNER JOIN losi ON losi.sede = es.es1");
	else
		sql = strdup("SELECT CASE WHEN LENGTH(sinonimo) > 0 THEN sinonimo ELSE nome END FROM losi");
	
	// XXX free sql after processing


	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

	if (rc != SQLITE_OK) {
		report(RPT_WARNING, "%s: Selecting data from database connection failed: %s",
			__FUNCTION__, sqlite3_errmsg(db));
		return -1;
	}
	
	/* fetch query result */

	for (count = 0; count < locations_max; ) {

		rc = sqlite3_step(stmt);

		if (rc == SQLITE_ROW) {

			int column_count;
			Location *loc;

			column_count = sqlite3_column_count(stmt);
			if (column_count < 1)
				continue;

			/* now we can instantiate the location */
			loc = location_create();

			if (with_codmae) // XXX optimize, unwind the loop
			{
				/* the cast to char * is needed because sqlite3_column_text()
				   returns an unsigned char *  */
				location_set_cons_mae_code(loc, (const char *) sqlite3_column_text(stmt, 0));

				/* is the location's name given as well?  */
				if (column_count > 1)
					location_set_name(loc, (const char *) sqlite3_column_text(stmt, 1));
			}
			else
			{
				location_set_name(loc, (const char *) sqlite3_column_text(stmt, 0));
			}

			*locations++ = loc;

			count++;

		} else if (rc == SQLITE_DONE) {
			break;

		} else {
			report(RPT_WARNING, "%s: Error in fetching data: %s",
					__FUNCTION__, sqlite3_errmsg(db));
			sqlite3_finalize(stmt);
			return -1;
		}
	}

	sqlite3_finalize(stmt);

	return count;
}

/* sce synonym : name !! */

int
xsqlite_find_locations_by_name(sqlite3 *db, const char *name, const char *province, bool in_italy, Queue *locations)
{
	/*
	 * An excerpt from: http://www.mail-archive.com/sqlite-users@sqlite.org/msg14482.html
	 *
	 *
	 * Re: [sqlite] LIKE operator with prepared statements
	 * 
	 * drh Fri, 07 Apr 2006 17:26:24 -0700
	 * 
	 * Eric Bohlman <[EMAIL PROTECTED]> wrote:
	 * > Dennis Cote wrote:
	 * > > You could also do this:
	 * > > SELECT x from y WHERE y.x LIKE '%' || ? || '%' ;
	 * > > 
	 * > > The || operator concatenates the % characters with your string.  Now you 
	 * > > don't need to massage the string in the calling code.  Six of one, half 
	 * > > dozen of the other.
	 * > 
	 * > Note, though, that as currently implemented (DRH has said it might 
	 * > change in the future) the concatenation will be performed for each row 
	 * > evaluated (and that particular query will guarantee a full-table scan) 
	 * > so doing it in the calling code would be a better idea if large tables 
	 * > are involved.
	 * > 
	 * 
	 * 
	 * The full table scan will happen regardless of what you do
	 * if you are LIKE-ing with anything that starts with '%'.  But
	 * you can force the common subexpression elimination by putting
	 * the subexpression in a subquery:
	 * 
	 *   SELECT x FROM y WHERE y.x LIKE (SELECT '%' || ? || '%')
	 * 
	 * The subquery will do the concatenation exactly once and reuse
	 * the result for every row of the Y table that it scans.
	 * --
	 * D. Richard Hipp   <[EMAIL PROTECTED]>
	 *
	 */

	//const char sql[] = "SELECT DISTINCT es.es4, CASE WHEN LENGTH(losi.sinonimo) > 0 THEN losi.sinonimo ELSE losi.nome END FROM es INNER JOIN losi ON losi.sede = es.es1 WHERE losi.nome LIKE (SELECT '%' || ? || '%')";
	//const char sql[] = "SELECT DISTINCT es.es4, CASE WHEN LENGTH(losi.sinonimo) > 0 THEN losi.sinonimo ELSE losi.nome END FROM es INNER JOIN losi ON losi.sede = es.es1 WHERE losi.nome LIKE (SELECT ? || '%')";

	/*
	 * From a mail by Stefano Pispola, 19 April 2013, 14.51:
	 *
	 * Significato tabella `co':
	 *
	 *   co1  --> ID Comune
	 *   co2  --> Codice ISTAT
	 *   co3  --> ID Provincia (primi 3: versione SCE; ultimi 3: ISTAT)
	 *   co4  --> Sigla Provincia
	 *   co5  --> Descrizione Comune
	 *   co6  --> CAP Comune (generico per le città: es 00100 per Roma)
	 *   co7  --> Prefisso telefonico Comune
	 *   co8  --> Numero Telefonico del Comune
	 *   co9  --> Altro Numero Telefonico del Comune
	 *   co10 --> E-mail del Comune
	 *   co11 --> Codice Belfiore
	 *
	 */

	/* sql_abroad sets NULL zip_code since we may have several zip_codes associated to
	   a single name */
	char sql_abroad[] =
		"SELECT DISTINCT Losi.Id Sce_Code, Es.Es4 Cons_Mae_Code, NULL Zip_Code, 'EE' Province, "
		"'000000' Istat_Code, Losi.Nome Synonym, Losi.Sinonimo Name, "
		"CASE WHEN LENGTH(Losi.Sinonimo) > 0 THEN Losi.Nome||' ['||Losi.Sinonimo||']' ELSE Losi.Nome END Full_Name "
		"FROM Es INNER JOIN Losi ON Losi.Sede = Es.Es1 "
		"WHERE Synonym LIKE (SELECT ? || '%') ORDER BY Full_Name DESC";
	char sql_italy_without_province[] =
		"SELECT DISTINCT Co1 Sce_Code, '0000000' Cons_Mae_Code, Co6 Zip_code, "
		"Co4 Province, Co2 Istat_Code, Co5 Synonym, NULL Name, Co5 Full_Name FROM Co "
		"WHERE Synonym LIKE (SELECT ? || '%') ORDER BY Full_Name DESC";
	char sql_italy_with_province[] =
		"SELECT DISTINCT Co1 Sce_Code, '0000000' Cons_Mae_Code, Co6 Zip_code, "
		"Co4 Province, Co2 Istat_Code, Co5 Synonym, NULL Name, Co5 Full_Name FROM Co "
		"WHERE Synonym LIKE (SELECT ? || '%') AND PROVINCE = ? ORDER BY Full_Name DESC";

	char *sql;

	if (in_italy) {
		if (province)
			sql = sql_italy_with_province;
		else
			sql = sql_italy_without_province;
	} else {
		sql = sql_abroad;
	}

	printf("\n%s\n", sql);

	return xsqlite_query_locations(db, sql, name, province, SCE_CODE_UNSET, NULL, locations);
}

int
xsqlite_find_locations_by_sce_code(sqlite3 *db, long sce_code, bool in_italy, Queue *locations)
{
	char sql_abroad[] =
		"SELECT DISTINCT Losi.Id Sce_Code, Es.Es4 Cons_Mae_Code, Pc.Pc3 Zip_Code, 'EE' Province, "
		"'000000' Istat_Code, Losi.Nome Synonym, Losi.Sinonimo Name, "
		"CASE WHEN LENGTH(Losi.Sinonimo) > 0 THEN Losi.Nome||' ['||Losi.Sinonimo||']' ELSE Losi.Nome END Full_Name "
		"FROM Es INNER JOIN Losi ON Losi.Sede = Es.Es1 INNER JOIN Pc ON Losi.Id = Pc2 "
		"WHERE Sce_Code = ? ORDER BY Sce_Code DESC";
	char sql_italy[] =
		"SELECT DISTINCT Co1 Sce_Code, '0000000' Cons_Mae_Code, Co6 Zip_code, "
		"Co4 Province, Co2 Istat_Code, Co5 Synonym, NULL Name, Co5 Full_Name FROM Co "
		"WHERE Sce_Code = ? ORDER BY Sce_Code DESC";

	char *sql;

	if (in_italy)
		sql = sql_italy;
	else
		sql = sql_abroad;

	printf("\n%s\n", sql);

	return xsqlite_query_locations(db, sql, NULL, NULL, sce_code, NULL, locations);
}

int
xsqlite_find_locations_by_zipcode(sqlite3 *db, const char *zipcode, int cod_state, Queue *locations)
{
	char sql_abroad[] =
		"SELECT DISTINCT Losi.Id Sce_Code, Es.Es4 Cons_Mae_Code, '%s' Zip_Code, 'EE' Province, "
		"'000000' Istat_Code, Losi.Nome Synonym, Losi.Sinonimo Name, "
		"CASE WHEN LENGTH(Losi.Sinonimo) > 0 THEN Losi.Nome||' ['||Losi.Sinonimo||']' ELSE Losi.Nome END Full_Name "
		"FROM Es INNER JOIN Losi ON Losi.Sede = Es.Es1 INNER JOIN Pc ON Losi.Id = Pc2 "
		"WHERE Pc.Pc3 = UPPER(?) ORDER BY Full_Name DESC";
	char sql_united_kingdom[] =
		"SELECT DISTINCT Losi.Id Sce_Code, Es.Es4 Cons_Mae_Code, '%s' Zip_Code, 'EE' Province, "
		"'000000' Istat_Code, Losi.Nome Synonym, Losi.Sinonimo Name, "
		"CASE WHEN LENGTH(Losi.Sinonimo) > 0 THEN Losi.Nome||' ['||Losi.Sinonimo||']' ELSE Losi.Nome END Full_Name "
		"FROM Es INNER JOIN Losi ON Losi.Sede = Es.Es1 INNER JOIN Pc_%c ON Losi.Id = Pc2 "
		"WHERE Pc_%c.Pc3 LIKE (SELECT ? || ' %') ORDER BY Full_Name DESC LIMIT 1";
	char sql_italy[] =
		"SELECT DISTINCT Co1 Sce_Code, '0000000' Cons_Mae_Code, Co6 Zip_code, "
		"Co4 Province, Co2 Istat_Code, Co5 Synonym, NULL Name, Co5 Full_Name FROM Co "
		"WHERE Zip_Code = ? ORDER BY Full_Name DESC";
	
	char *sql, *zipcode2;
	int rc;

	if (cod_state == COD_STATE_ITALY) {
		sql = xmalloc(strlen(sql_italy) + 1, __FUNCTION__);
		strcpy(sql, sql_italy);
		zipcode2 = strdup(zipcode);
	} else if (cod_state == COD_STATE_UK) {
		char first_letter = (char) *zipcode;
		sql = xmalloc(strlen(sql_united_kingdom) + strlen(zipcode) + 1, __FUNCTION__);
		snprintf(sql, strlen(sql_united_kingdom) + strlen(zipcode), sql_united_kingdom, zipcode, first_letter, first_letter);
		zipcode2 = get_first_token(zipcode, " ");
	} else {
		sql = xmalloc(strlen(sql_abroad) + strlen(zipcode) + 1, __FUNCTION__);
		snprintf(sql, strlen(sql_abroad) + strlen(zipcode), sql_abroad, zipcode);
		zipcode2 = strdup(zipcode);
	}

	printf("\n%s\n", sql);

	rc = xsqlite_query_locations(db, sql, NULL, NULL, SCE_CODE_UNSET, zipcode2, locations);
	free(zipcode2);
	free(sql);

	return rc;
}

int
xsqlite_find_consulates(sqlite3 *db, int cod_state, bool territorial_competence, Queue *consulates)
{
	char sql_territorial_competence[] = "SELECT DISTINCT Es4 Mae_Code FROM Es";
	char sql_no_territorial_competence[] = "SELECT DISTINCT Es4 Mae_Code FROM Es WHERE Es5 = ?";
	char *sql;

	if (territorial_competence)
		sql = sql_territorial_competence;
	else
		sql = sql_no_territorial_competence;

	printf("\n%s\n", sql);

	return xsqlite_query_consulates(db, sql, cod_state, territorial_competence, consulates);
}

int
xsqlite_find_consular_districts(sqlite3 *db, Queue *consular_districts)
{
	char sql[] =
		"SELECT Es.Es4 Mae_Code, Te.Te2 Territory, Te.Te3 Sce_Code FROM Es, Te";

	printf("\n%s\n", sql);

	return xsqlite_query_consular_districts(db, sql, consular_districts);
}

int
xsqlite_find_zip_code_specimina(sqlite3 *db, Queue *zip_code_specimina)
{
	char sql[] =
		"SELECT Na1 Cod_State, CASE WHEN LENGTH(Pc3) > 0 THEN Pc3 ELSE 'UNAVAILABLE' END Zip_Code "
		"FROM (SELECT Pc3, RANDOM() r FROM Pc WHERE Pc3 <> '*' LIMIT 1000) JOIN Na ORDER BY r LIMIT 1";

	printf("\n%s\n", sql);

	return xsqlite_query_zip_code_specimina(db, sql, zip_code_specimina);
}

int
xsqlite_find_zip_code_specimen_italy(Queue *zip_code_specimina)
{
	/* since the AAA mth file does not know of a Na table,
	   we choose a static zip code */

	Zipcode *zipcode;

	zipcode = zipcode_create();
	if (!zipcode)
		return -1;

	zipcode_set_cod_state_integer(zipcode, COD_STATE_ITALY);
	zipcode_set_code(zipcode, "80067");

	queue_push(zip_code_specimina, zipcode);

	return 0;
}
