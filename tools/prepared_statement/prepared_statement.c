#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>

void do_exit(PGconn *conn)
{
    PQfinish(conn);
    exit(1);
}

int main(int argc, char *argv[])
{
    const char *paramValues[1]={"1"/*table id*/};
    //connect
    PGconn *conn = PQconnectdb("user=postgres dbname=test");
    if (PQstatus(conn) == CONNECTION_BAD)
    { //if failed quit
        fprintf(stderr, "Connection to database failed: %s\n",PQerrorMessage(conn));
        do_exit(conn);
    }
    char *stm = "SELECT * FROM cars WHERE id=$1";//the query
    //prepare, mean: fix, optimise and cache the execution plan, this prevent any change to execution plan and improve the security too
    PGresult *resprep = PQprepare(conn, "testprepared",stm, 1, NULL/*paramTypes*/);    
    if (PQresultStatus(resprep) != PGRES_COMMAND_OK)
    { //if failed quit
        printf("Problem to prepare it\n");        
        PQclear(resprep);
        do_exit(conn);
    }
    //execute it
    PGresult *res = PQexecPrepared(conn, "testprepared", 1, paramValues, NULL, NULL, 0);    
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    { //if failed quit
       printf("No data retrieved\n");        
        PQclear(res);
        do_exit(conn);
    }    
    //print the result
    printf("%s %s\n", PQgetvalue(res, 0, 0), PQgetvalue(res, 0, 1));    
    //close and quit
    PQclear(res);
    PQfinish(conn);
    return 0;
}