#include "dbixx.h"
#include <iostream>

namespace dbixx {
using namespace std;

row::~row()
{
	if(res && owner) {
		dbi_result_free(res);
	}
}

void row::reset()
{
	if(res && owner) {
		dbi_result_free(res);
	}
	res=NULL;
	owner=false;
}

bool row::isempty()
{
	return res==NULL;
}

void row::set(dbi_result &r)
{
	if(res && r!=res && owner) {
		dbi_result_free(res);
	}
	owner=false;
	res=r;
	current=0;
}

void row::assign(dbi_result &r)
{
	if(res && r!=res && owner) {
		dbi_result_free(res);
	}
	owner=true;
	res=r;
	current=0;
	if(!dbi_result_next_row(res)) {
		reset();
	}
}

void row::check_set()
{
	if(!res)
		throw dbixx_error("Using unititilized row");
}

bool row::isnull(int inx)
{
	check_set();
	int r;
	r=dbi_result_field_is_null_idx(res,inx);
	if(r == DBI_FIELD_FLAG_ERROR ) {
		throw dbixx_error("Invalid field");
	}
	return r;
}

bool row::isnull(std::string const &id)
{
	check_set();
	int r;
	r=dbi_result_field_is_null(res,id.c_str());
	if(r == DBI_FIELD_FLAG_ERROR ) {
		throw dbixx_error("Invalid field");
	}
	return r;
}

bool row::fetch(int pos,int &value)
{
	long long v;
	bool r=fetch(pos,v);
	if(r)
		value=v;
	return r;
}

bool row::fetch(int pos,unsigned &value)
{
	unsigned long long v;
	bool r=fetch(pos,v);
	if(r)
		value=v;
	return r;
}

bool row::fetch(int pos,long long &v)
{
	if(isnull(pos)) return false;
	int type=dbi_result_get_field_type_idx(res,pos);
	switch(type) {
	case DBI_TYPE_INTEGER:
	case DBI_TYPE_DECIMAL:
		v=dbi_result_get_longlong_idx(res,pos);
		break;
	case DBI_TYPE_STRING:
		v=atol(dbi_result_get_string_idx(res,pos));
		break;
	default:
		dbixx_error("Bad cast to integer type");
	}
	return true;	
}

bool row::fetch(int pos,unsigned long long &v)
{
	if(isnull(pos)) return false;
	int type=dbi_result_get_field_type_idx(res,pos);
	switch(type) {
	case DBI_TYPE_INTEGER:
	case DBI_TYPE_DECIMAL:
		v=dbi_result_get_ulonglong_idx(res,pos);
		break;
	case DBI_TYPE_STRING:
		v=atol(dbi_result_get_string_idx(res,pos));
		break;
	default:
		dbixx_error("Bad cast to integer type");
	}
	return true;	
}

bool row::fetch(int pos,string &v)
{
	char const *tmp;
	if(isnull(pos)) return false;
	int type=dbi_result_get_field_type_idx(res,pos);
	switch(type) {
	case DBI_TYPE_STRING:
		tmp=dbi_result_get_string_idx(res,pos);
		if(tmp)
			v=tmp;
		else
			return false;
		break;
	default:
		dbixx_error("Bad cast to string type");
	}
	return true;	
}

bool row::fetch(int pos,double &v)
{
	if(isnull(pos)) return false;
	int type=dbi_result_get_field_type_idx(res,pos);
	switch(type) {
	case DBI_TYPE_DECIMAL:
		if(dbi_result_get_field_attribs_idx(res,pos) & DBI_DECIMAL_SIZE8)
			v=dbi_result_get_double_idx(res,pos);
		else
			v=dbi_result_get_float_idx(res,pos);
		break;
	case DBI_TYPE_INTEGER:
		v=dbi_result_get_longlong_idx(res,pos);
		break;
	case DBI_TYPE_STRING:
		v=atof(dbi_result_get_string_idx(res,pos));
		break;
	default:
		dbixx_error("Bad cast to double type");
	}
	return true;	
}

bool row::fetch(int pos,std::tm &t)
{
	if(isnull(pos)) return false;
	int type=dbi_result_get_field_type_idx(res,pos);
	time_t v;
	switch(type) {
	case DBI_TYPE_DATETIME:
		v=dbi_result_get_datetime_idx(res,pos);
		gmtime_r(&v,&t);
		break;
	case DBI_TYPE_STRING:
		{
			char const *d=dbi_result_get_string_idx(res,pos);
			if(sscanf(d,"%d-%d-%d %d:%d:%d",
				&t.tm_year,&t.tm_mon,&t.tm_mday,
				&t.tm_hour,&t.tm_min,&t.tm_sec)!=6)
			{
				dbixx_error("Bad cast to datetime type");
			}
			t.tm_year-=1900;
			t.tm_mon-=1;
			// Fill correct rest of fields
			v=mktime(&t);
			localtime_r(&v,&t);
		}
		break;
	default:
		dbixx_error("Bad cast to datetime type");
	}
	return true;	
}

}// Namespace dbixx
