#include "dbixx.h"
#include <stdio.h>
#include <limits>
#include <iomanip>
#include <sstream>

namespace dbixx {

using namespace std;


class loader {
public:
	loader() { dbi_initialize(NULL); };
	~loader() { dbi_shutdown(); };
};

static loader backend_loader;

session::session()
{
	conn=NULL;
}

void session::connect(std::string const &connection_string)
{
	size_t p = connection_string.find(':');
	if( p == std::string::npos )
		throw dbixx_error("Invalid connection string - no driver given");
	driver(connection_string.substr(0,p));
	p++;
	while(p<connection_string.size()) {
		size_t n=connection_string.find('=',p);
		if(n==std::string::npos)
			throw dbixx_error("Invalid connection string - invalid property");
		std::string key = connection_string.substr(p,n-p);
		p=n+1;
		std::string value;
		bool is_string = true;
		if(p>=connection_string.size()) {
			/// Nothing - empty property
		}
		else if(connection_string[p]=='\'') {
			p++;
			while(true) {
				if(p>=connection_string.size()) {
					throw dbixx_error("Invalid connection string unterminated string");
				}
				if(connection_string[p]=='\'') {
					if(p+1 < connection_string.size() && connection_string[p+1]=='\'') {
						value+='\'';
						p+=2;
					}
					else {
						p++;
						break;
					}
				}
				else {
					value+=connection_string[p];
					p++;
				}
			}
		}
		else {
			size_t n=connection_string.find(';',p);
			if(n==std::string::npos) {
				value=connection_string.substr(p);
				p=connection_string.size();
			}
			else {
				value=connection_string.substr(p,n-p);
				p=n;
			}
			if(!value.empty()) {
				size_t pos = 0;
				if(value[0]=='-') {
					pos = 1;
				}
				bool digit_detected = false;
				bool only_digits = true;
				while(pos < value.size() && only_digits) {
					if('0'<=value[pos] && value[pos]<='9')
						digit_detected = true;
					else
						only_digits = false;
					pos++;
				}
				if(only_digits && digit_detected) {
					is_string = false;
				}
			}
		}
		if(is_string) {
			param(key,value);
		}
		else {
			param(key,atoi(value.c_str()));
		}
		if(p < connection_string.size() && connection_string[p]==';')
			++p;

	}
	connect();
}

void session::connect()
{
	check_open();
	map<string,string>::const_iterator sp;
	for(sp=string_params.begin();sp!=string_params.end();sp++){
		if(dbi_conn_set_option(conn,sp->first.c_str(),sp->second.c_str())) {
			error();
		}
	}

	map<string,int>::const_iterator ip;
	for(ip=numeric_params.begin();ip!=numeric_params.end();ip++){
		if(dbi_conn_set_option_numeric(conn,ip->first.c_str(),ip->second)) {
			error();
		}
	}

	if(dbi_conn_connect(conn)<0) {
		error();
	}
}

void session::reconnect()
{
	close();
	driver(this->backend);
	connect();
}

session::session(string const &backend_or_conn_str)
{
	conn=NULL;

	if(backend_or_conn_str.find(':')==std::string::npos)
		driver(backend_or_conn_str);
	else
		connect(backend_or_conn_str);
}

void session::close()
{
	if(conn) {
		dbi_conn_close(conn);
		conn=NULL;
	}
}

session::~session()
{
	close();
}

void session::driver(string const &backend)
{
	close();
	this->backend=backend;
	conn=dbi_conn_new(backend.c_str());
	if(!conn) {
		throw dbixx_error("Failed to load backend");
	}
}

std::string session::driver()
{
	return backend;
}

void session::check_open(void) 
{
	if(!conn) throw dbixx_error("Backend is not open");
}

unsigned long long session::rowid(char const *name)
{
	check_open();
	return dbi_conn_sequence_last(conn,name);
}

void session::error()
{
	char const *e;
	dbi_conn_error(conn,&e);
	throw dbixx_error(e,escaped_query);
}

void session::param(string const &par,string const &val)
{
	string_params[par]=val;
}

void session::param(string const &par,int val)
{
	numeric_params[par]=val;
}

void session::escape()
{
	while(pos_read<query_in.size()) {
		if(query_in[pos_read]=='\'') {
			escaped_query+='\'';
			pos_read++;
			while(query_in[pos_read]!='\'' && pos_read!=query_in.size()){
				escaped_query+=query_in[pos_read];
				pos_read++;
			}
			if(pos_read==query_in.size())
				throw dbixx_error("Unexpected end of query after \"'\"");
			escaped_query+='\'';
			pos_read++;
			continue;
		}
		if(query_in[pos_read]=='?') {
			ready_for_input=true;
			pos_read++;
			break;
		}
		escaped_query+=query_in[pos_read];
		pos_read++;
	}
	if(ready_for_input)
		return;
	if(pos_read==query_in.size()) {
		complete=true;
		return;
	}
	throw dbixx_error("Internal dbixx error");
}

void session::check_input()
{
	if(!ready_for_input) {
		throw dbixx_error("More parameters given then inputs in query");
	}
}

template<typename T>
void session::do_bind(T v,bool is_null)
{
	check_input();
	// The representation of a number in decimal form
	// is more compact then in binary so it should be enough
	if(is_null) {
		escaped_query+="NULL";
	}
	else {
		std::ostringstream ss;
		ss.imbue(std::locale::classic());
		
		if(!std::numeric_limits<T>::is_integer) {
			ss<<std::setprecision(std::numeric_limits<T>::digits10+1);
		}

		ss << v;
		escaped_query+=ss.str();
	}
	ready_for_input=false;
	escape();
}

void session::bind(int v,bool isnull) { do_bind(v,isnull); }
void session::bind(unsigned v,bool isnull) { do_bind(v,isnull); }
void session::bind(long v,bool isnull) { do_bind(v,isnull); }
void session::bind(unsigned long v,bool isnull) { do_bind(v,isnull); }
void session::bind(long long v,bool isnull) { do_bind(v,isnull); }
void session::bind(unsigned long long v,bool isnull) { do_bind(v,isnull); }
void session::bind(double v,bool isnull) { do_bind(v,isnull); }
void session::bind(long double v,bool isnull) { do_bind(v,isnull); }

void session::bind(std::tm const &v,bool isnull)
{
	check_input();
	// The representation of a number in decimal form
	// is more compact then in binary so it should be enough
	if(isnull) {
		escaped_query+="NULL";
	}
	else {
		std::ostringstream ss;
		ss.imbue(std::locale::classic());
		ss<<std::setfill('0');
		ss <<"'";
		ss << std::setw(4) << v.tm_year+1900 <<'-';
		ss << std::setw(2) << v.tm_mon+1 <<'-';
		ss << std::setw(2) << v.tm_mday <<' ';
		ss << std::setw(2) << v.tm_hour <<':';
		ss << std::setw(2) << v.tm_min  <<':'; 
		ss << std::setw(2) << v.tm_sec;
		ss <<"'";

		escaped_query+=ss.str();
	}
	ready_for_input=false;
	escape();
}

void session::bind(null const &m,bool isnull)
{
	check_input();
	escaped_query+="NULL";
	ready_for_input=false;
	escape();
}


void session::bind(string const &s,bool isnull)
{
	check_input();
	check_open();
	if(isnull) {
		escaped_query+="NULL";
	}
	else {
		if(s.size()!=0){
			char *new_str=NULL;
			size_t sz=dbi_conn_quote_string_copy(conn,s.c_str(),&new_str);
			if(sz==0) {
				error();	
			}
			try {
				escaped_query+=new_str;
			}
			catch(...) {
				free(new_str);
				throw;
			};
			free(new_str);
		}
		else {
			escaped_query+="\'\'";
		}
	}
	ready_for_input=false;
	escape();
}



void session::query(std::string const &q)
{
	complete=false;
	ready_for_input=false;
	query_in=q;
	pos_read=0;
	escaped_query="";
	escaped_query.reserve(q.size()*3/2);
	pos_write=0;
	escape();
}

void session::exec()
{
	check_open();
	if(!complete)
		throw dbixx_error("Not all parameters are bind");
	dbi_result res=dbi_conn_query(conn,escaped_query.c_str());
	if(!res) error();
	if(dbi_result_get_numrows(res)!=0) {
		dbi_result_free(res);
		throw dbixx_error("exec() query may not return results");
	}
	affected_rows=dbi_result_get_numrows_affected(res);
	dbi_result_free(res);
}

void session::fetch(result &r)
{
	check_open();
	if(!complete)
		throw dbixx_error("Not all parameters are bind");
	dbi_result res=dbi_conn_query(conn,escaped_query.c_str());
	if(!res) error();
	r.assign(res);
}

bool session::single(row &r)
{
	check_open();
	if(!complete)
		throw dbixx_error("Not all parameters are bind");
	dbi_result res=dbi_conn_query(conn,escaped_query.c_str());
	if(!res) error();
	int n;
	if((n=dbi_result_get_numrows(res))!=0 && n!=1) {
		dbi_result_free(res);
		throw dbixx_error("signle() must return 1 or 0 rows");
	}
	if(n==1) {
		r.assign(res);
		return true;
	}
	else {
		r.reset();
	}
	return false;
}

transaction::~transaction()
{
	if(!commited){
		try {
			sql<<"ROLLBACK",exec();
		}
		catch(...){}
	}
}

void transaction::begin()
{
	sql<<"BEGIN",exec();
}

void transaction::commit()
{
	sql<<"COMMIT",exec();
	commited=true;
}

void transaction::rollback()
{
	sql<<"ROLLBACK",exec();
	commited=true;
}

} // END OF NAMESPACE DBIXX
