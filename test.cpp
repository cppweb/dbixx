#include "dbixx.h"
#include <iostream>
#include <list>
using namespace dbixx;
using namespace std;

struct e_t {
	double f;
	int id,k;
	std::tm atime;
	string name;
};

int main()
{
	try {
	session sql("sqlite3");
	sql.param("dbname","test.db");
	sql.param("sqlite3_dbdir","./");

/*	session sql("mysql");
	sql.param("dbname","cppcms");
	sql.param("username","root");
	sql.param("password","root");
	sql.param("host","127.0.0.1");
	sql.param("port",3306);*/

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
	cout<<asctime(&t);

	
	int n;
	sql<<"insert into test(n,f,t,name) values(?,?,?,?)",
		10,3.1415926565,t,"Hello \'World\'",
		exec();
	//cout<<"ID:"<<sql.rowid()<<endl;
	cout<<"ID:"<<sql.rowid("test_id_seq")<<", Affected rows"<<sql.affected()<<endl;
	sql<<"insert into test(n,f,t,name) values(?,?,?,?)",
		use(10,true),use(3.1415926565,true),use(t,true),use("Hello \'World\'",true),
		exec();
	//cout<<"ID:"<<sql.rowid()<<endl;
	cout<<"ID:"<<sql.rowid("test_id_seq")<<", Affected rows"<<sql.affected()<<endl;

	row r;
	result res;
	sql<<"select id,n,f,t,name from test limit 10",
		res;
	cout<<"Rows:"<<res.rows()<<endl;
	cout<<"Cols:"<<res.cols()<<endl;
	n=0;
	while(res.next(r)){
		double f=-1;
		int id=-1,k=-1;
		std::tm atime={0};
		string name="nonset";
		r >> id >> k >> f >> atime >> name;
		cout <<id << ' '<<k <<' '<<f<<' '<<name<<' '<<asctime(&atime)<<endl;
		cout<<"has "<<r.cols()<<" columns\n";
		n++;
	}

	{
	vector<e_t> elements;
	sql<<"select id,n,f,t,name from test limit 10";
	sql.fetch(elements)
		% &e_t::id % &e_t::k % &e_t::f % &e_t::atime % &e_t::name;


	unsigned i;
	for(i=0;i<elements.size();i++){
		cout <<elements[i].id << ' '<< elements[i].k
		 <<' '<<elements[i].f<<' '<<elements[i].name<<' '<<
		 asctime(&elements[i].atime)<<endl;
	}
	}

	{
	list<e_t> elements;
	sql<<"select id,n,f,t,name from test limit 10",res;
	res.bind(elements)
		% &e_t::id % &e_t::k % &e_t::f % &e_t::atime % &e_t::name;

	for(list<e_t>::iterator i=elements.begin();i!=elements.end();++i){
		cout <<i->id << ' '<< i->k
		 <<' '<<i->f<<' '<<i->name<<' '<<
		 asctime(&i->atime)<<endl;
	}
	}
	for(int k=10;k<=11;k++) {
		string name;
		sql<<"SELECT name FROM test WHERE n=?",k;
		if(sql.single() % name) {
			cout<<k<<":"<<name<<endl;
		}
		else {
			cout<<k<<":"<<"Not found\n";
		}
	}

	sql<<"delete from test",
		exec();
	cout<<"Deleted "<<sql.affected()<<" rows\n";
	return 0;
	}
	catch(std::exception const &e) {
		cerr<<"Failed:"<<e.what()<<endl;
		return 1;
	}
}
