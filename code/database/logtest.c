#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <mysql/mysql.h>
#include <sys/time.h>

//#define INSERT_SQL "insert into emp values( %d,'test1111','test222222222','test3111111111' )"

#define INSERT_SQL "insert into `tlog.1` values ( %d, %d, TO_DAYS('2013-04-12'),\
TIME_TO_SEC('19:40:15'), %d, %d, %d, %d, %d, %d, %d, %d,\
%d, %d, %d, %d, %d, %d, %d, TO_DAYS('2013-4-01'), %d, %d)"

int main()
{
	MYSQL my_connection;
	//MYSQL_RES *res_ptri = NULL ;
	//MYSQL_ROW sqlrow;
	//MYSQL_FIELD *fd = NULL;
	int count = 0;
	int res = 0;

	struct  timeval    tv;
	struct  timezone   tz;

	struct  timeval    tv2;
	struct  timezone   tz2;

	mysql_init(&my_connection);

	if ( mysql_real_connect(&my_connection,"","root","","zgtest",
				0,"/tmp/mysql.sock",CLIENT_FOUND_ROWS ) )
	{
	
		char sql[ 1000 ] = { 0 };
		printf( "mysql connect success\n" );

		gettimeofday( &tv,&tz );

		mysql_query(&my_connection,"BEGIN");
		//mysql_query( &my_connection,"SET AUTOCOMMIT=0" );

		for( count = 0; count < 20000;count ++ )
		{	
			memset( sql,0x0,1000 );
			sprintf( sql,INSERT_SQL,count,count,count,count,count,count,count,count,count,count,count,count,count,count,count,count,count,count,count );

			res = mysql_query( &my_connection,sql );

			if( res )
                        {
                                printf( "mysql execute error\n" );

                        }


		}	
		
		mysql_query( &my_connection,"COMMIT" ); 
		mysql_query( &my_connection,"END" );
		
		gettimeofday( &tv2,&tz2 );

	}	

	printf( "time = %lu\n" ,tv2.tv_sec - tv.tv_sec );
	mysql_close(&my_connection);

	return 0;
}	

