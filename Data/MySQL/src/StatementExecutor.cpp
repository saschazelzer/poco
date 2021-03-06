//
// StatementExecutor.cpp
//
// $Id: //poco/1.3/Data/MySQL/src/StatementExecutor.cpp#1 $
//
// Library: Data
// Package: MySQL
// Module:  StatementExecutor
//
// Copyright (c) 2008, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#include <mysql.h>
#include "Poco/Data/MySQL/StatementExecutor.h"
#include "Poco/Format.h"


namespace Poco {
namespace Data {
namespace MySQL {


StatementExecutor::StatementExecutor(MYSQL* mysql)
	: _pSessionHandle(mysql)
	, _affectedRowCount(0)
{
	if (!(_pHandle = mysql_stmt_init(mysql)))
		throw StatementException("mysql_stmt_init error");

	_state = STMT_INITED;
}


StatementExecutor::~StatementExecutor()
{
	mysql_stmt_close(_pHandle);
}


int StatementExecutor::state() const
{
	return _state;
}


void StatementExecutor::prepare(const std::string& query)
{
	if (_state >= STMT_COMPILED)
	{
		_state = STMT_COMPILED;
		return;
	}
	
	if (mysql_stmt_prepare(_pHandle, query.c_str(), static_cast<unsigned int>(query.length())) != 0)
		throw StatementException("mysql_stmt_prepare error", _pHandle, query);

	_query = query;
	_state = STMT_COMPILED;
}


void StatementExecutor::bindParams(MYSQL_BIND* params, std::size_t count)
{
	if (_state < STMT_COMPILED)
		throw StatementException("Statement is not compiled yet");

	if (count != mysql_stmt_param_count(_pHandle))
		throw StatementException("wrong bind parameters count", 0, _query);

	if (count == 0) return;

	if (mysql_stmt_bind_param(_pHandle, params) != 0)
		throw StatementException("mysql_stmt_bind_param() error ", _pHandle, _query);
}


void StatementExecutor::bindResult(MYSQL_BIND* result)
{
	if (_state < STMT_COMPILED)
		throw StatementException("Statement is not compiled yet");

	if (mysql_stmt_bind_result(_pHandle, result) != 0)
		throw StatementException("mysql_stmt_bind_result error ", _pHandle, _query);
}


void StatementExecutor::execute()
{
	if (_state < STMT_COMPILED)
		throw StatementException("Statement is not compiled yet");

	if (mysql_stmt_execute(_pHandle) != 0)
		throw StatementException("mysql_stmt_execute error", _pHandle, _query);

	_state = STMT_EXECUTED;

	my_ulonglong affectedRows = mysql_affected_rows(_pSessionHandle);
	if (affectedRows != ((my_ulonglong) - 1))
		_affectedRowCount = static_cast<std::size_t>(affectedRows); //Was really a DELETE, UPDATE or INSERT statement
}


bool StatementExecutor::fetch()
{
	if (_state < STMT_EXECUTED)
		throw StatementException("Statement is not executed yet");

	int res = mysql_stmt_fetch(_pHandle);

	// we have specified zero buffers for BLOBs, so DATA_TRUNCATED is normal in this case
	if ((res != 0) && (res != MYSQL_NO_DATA) && (res != MYSQL_DATA_TRUNCATED)) 
		throw StatementException("mysql_stmt_fetch error", _pHandle, _query);

	return (res == 0) || (res == MYSQL_DATA_TRUNCATED);
}


bool StatementExecutor::fetchColumn(std::size_t n, MYSQL_BIND *bind)
{
	if (_state < STMT_EXECUTED)
		throw StatementException("Statement is not executed yet");

	int res = mysql_stmt_fetch_column(_pHandle, bind, static_cast<unsigned int>(n), 0);

	if ((res != 0) && (res != MYSQL_NO_DATA))
		throw StatementException(Poco::format("mysql_stmt_fetch_column(%z) error", n), _pHandle, _query);

	return (res == 0);
}

std::size_t StatementExecutor::getAffectedRowCount() const
{
	return _affectedRowCount;
}


}}}
