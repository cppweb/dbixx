#include "dbixx.h"

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

void session::connect()
{
	check_open();
	map<string,string>::const_iterator i;
	for(i=params.begin();i!=params.end();i++){
		if(dbi_conn_set_option(conn,i->first.c_str(),i->second.c_str())) {
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

session::session(string const &backend)
{
	conn=NULL;
	driver(backend);
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

void session::check_open(void) 
{
	if(!conn) throw dbixx_error("Backend is not open");
}

void session::error()
{
	char const *e;
	dbi_conn_error(conn,&e);
	throw dbixx_error(e);
}

void session::param(string const &par,string const &val)
{
	params[par]=val;
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

void session::bind(long long const &v,bool isnull)
{
	check_input();
	// The representation of a number in decimal form
	// is more compact then in binary so it should be enough
	char buffer[sizeof(long long)*8];
	if(isnull) {
		escaped_query+="NULL";
	}
	else {
		if((int)sizeof(buffer)<snprintf(buffer,sizeof(buffer),"%lld",v)) {
			throw dbixx_error("Internal Error - buffer_size");
		}
		escaped_query+=buffer;
	}
	ready_for_input=false;
	escape();
}

void session::bind(double const &v,bool isnull)
{
	check_input();
	// The representation of a number in decimal form
	// is more compact then in binary so it should be enough
	char buffer[sizeof(double)*8];
	if(isnull) {
		escaped_query+="NULL";
	}
	else {
		if((int)sizeof(buffer)<snprintf(buffer,sizeof(buffer),"%e",v)) {
			throw dbixx_error("Internal Error - buffer_size");
		}
		escaped_query+=buffer;
	}
	ready_for_input=false;
	escape();
}
void session::bind(unsigned long long const &v,bool isnull)
{
	check_input();
	// The representation of a number in decimal form
	// is more compact then in binary so it should be enough
	char buffer[sizeof(long long)*8];
	if(isnull) {
		escaped_query+="NULL";
	}
	else {
		if((int)sizeof(buffer)<snprintf(buffer,sizeof(buffer),"%llu",v)) {
			throw dbixx_error("Internal Error - buffer_size");
		}
		escaped_query+=buffer;
	}
	ready_for_input=false;
	escape();
}

void session::bind(std::tm const &v,bool isnull)
{
	check_input();
	// The representation of a number in decimal form
	// is more compact then in binary so it should be enough
	char buffer[1+4+1+2+1+2+1+2+1+2+1+2+1+1]; // '2008-01-01 12:23:43' 
	if(isnull) {
		escaped_query+="NULL";
	}
	else {
		if((int)sizeof(buffer)<snprintf(buffer,sizeof(buffer),
			"'%04d-%02d-%02d %02d:%02d:%02d'",
			v.tm_year+1900,v.tm_mon+1,v.tm_mday,
			v.tm_hour,v.tm_min,v.tm_sec)) 
		{
			throw dbixx_error("Internal Error - buffer_size");
		}
		escaped_query+=buffer;
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


} // END OF NAMESPACE DBIXX
