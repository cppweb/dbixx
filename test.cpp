#include "dbixx.h"
#include <iostream>
using namespace dbixx;
using namespace std;

int main()
{
	session sql("sqlite3");
	sql.param("dbname","test.db");
	sql.param("sqlite3_dbdir","./");

//	session sql("mysql");
//	sql.param("dbname","cppcms");
//	sql.param("username","root");
//	sql.param("password","root");

//	session sql("pgsql");
//	sql.param("dbname","cppcms");
//	sql.param("username","artik");


	sql.connect();
	
	//sql<<"drop table if exists test",
	//	exec();
	try {
		sql<<"drop table test",
			exec();	
	}
	catch(dbixx_error const &e) {
	}
	//sql<<"create table test ( id integer primary key auto_increment not null,n integer, f real , t timestamp ,name text )",
	//	exec();
	sql<<"create table test ( id integer primary key autoincrement not null,n integer, f real , t timestamp ,name text )",
		exec();
	//sql<<"create table test ( id  serial  primary key not null ,n integer, f real , t timestamp ,name text )",
	//	exec();
	std::tm t;
	time_t tt;
	tt=time(NULL);
	localtime_r(&tt,&t);

	
	int n;
	sql<<"insert into test(n,f,t,name) values(?,?,?,?)",
		10,3.1415926565,t,"Hello \'World\'",
		exec();
	//cout<<"ID:"<<sql.rowid()<<endl;
	cout<<"ID:"<<sql.rowid("test_id_seq")<<endl;
	sql<<"insert into test(n,f,t,name) values(?,?,?,?)",
		use(10,true),use(3.1415926565,true),use(t,true),use("Hello \'World\'",true),
		exec();
	//cout<<"ID:"<<sql.rowid()<<endl;
	cout<<"ID:"<<sql.rowid("test_id_seq")<<endl;

	row r;
	result res;
	sql<<"select id,n,f,t,name from test limit 10",
		res;
	cout<<"Rows:"<<res.rows()<<endl;
	n=0;
	while(res.next(r)){
		double f=-1;
		int id=-1,k=-1;
		std::tm atime={0};
		string name="nonset";
		r >> id >> k >> f >> atime >> name;
		cout <<id << ' '<<k <<' '<<f<<' '<<name<<' '<<asctime(&atime)<<endl;
		n++;
	}
	return 0;
}
