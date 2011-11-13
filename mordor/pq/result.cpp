// Copyright (c) 2010 - Mozy, Inc.
#include "stdafx.h"

#include "mordor/predef.h"
#include "result.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>

#include "mordor/assert.h"
#include "mordor/endian.h"
#include "mordor/socket.h"

#define BOOLOID 16
#define CHAROID 18
#define INT8OID 20
#define INT2OID 21
#define INT4OID 23
#define CIDROID 650
#define FLOAT4OID 700
#define FLOAT8OID 701
#define INETOID 869
#define TIMESTAMPOID 1114
#define TIMESTAMPTZOID 1184
#define INT4ARRAYOID 1007

namespace Mordor {
namespace PQ {

size_t
Result::rows() const
{
    return (size_t)PQntuples(m_result.get());
}

size_t
Result::columns() const
{
    return (size_t)PQnfields(m_result.get());
}

size_t
Result::column(const char *name) const
{
    return (size_t)PQfnumber(m_result.get(), name);
}

const char *
Result::column(size_t index) const
{
    return PQfname(m_result.get(), (int)index);
}

Oid
Result::getType(size_t column) const
{
    return PQftype(m_result.get(), (int)column);
}

bool Result::getIsNull(size_t row, size_t column) const {
    return PQgetisnull(m_result.get(), (int)row, (int)column) == 1;
}

template <>
std::string
Result::get<std::string>(size_t row, size_t column) const
{
    return std::string(PQgetvalue(m_result.get(), (int)row, (int)column),
        PQgetlength(m_result.get(), (int)row, (int)column));
}

template <>
const char *
Result::get<const char *>(size_t row, size_t column) const
{
    return PQgetvalue(m_result.get(), (int)row, (int)column);
}

template <>
bool
Result::get<bool>(size_t row, size_t column) const
{
	MORDOR_ASSERT(getType(column) == BOOLOID);
	if (m_format == BINARY || PQbinaryTuples(m_result.get()) == 1) {
		MORDOR_ASSERT(PQgetlength(m_result.get(), (int)row, (int)column) == 1);
		return !!*PQgetvalue(m_result.get(), (int)row, (int)column);
	} else {
		//convert from ascii
		char* value = PQgetvalue(m_result.get(), (int)row, (int)column);
		return value[0] == 't' || value[0] == 'T' || value[0] == 'y' || value[0] == 'Y' || value[0] == '1';
	}
}

template <>
char
Result::get<char>(size_t row, size_t column) const
{
	MORDOR_ASSERT(getType(column) == CHAROID);
	if (m_format == BINARY || PQbinaryTuples(m_result.get()) == 1) {
		MORDOR_ASSERT(PQgetlength(m_result.get(), (int)row, (int)column) == 1);
		return *PQgetvalue(m_result.get(), (int)row, (int)column);
	} else {
		return PQgetvalue(m_result.get(), (int)row, (int)column)[0];
	}
}

template <>
long long
Result::get<long long>(size_t row, size_t column) const
{
	if (m_format == BINARY || PQbinaryTuples(m_result.get()) == 1) {
		switch (getType(column)) {
			case INT8OID:
				MORDOR_ASSERT(PQgetlength(m_result.get(), (int)row, (int)column) == 8);
				return byteswapOnLittleEndian(*(long long *)PQgetvalue(m_result.get(), (int)row, (int)column));
			case INT2OID:
				MORDOR_ASSERT(PQgetlength(m_result.get(), (int)row, (int)column) == 2);
				return byteswapOnLittleEndian(*(short *)PQgetvalue(m_result.get(), (int)row, (int)column));
			case INT4OID:
				MORDOR_ASSERT(PQgetlength(m_result.get(), (int)row, (int)column) == 4);
				return byteswapOnLittleEndian(*(int *)PQgetvalue(m_result.get(), (int)row, (int)column));
			default:
				MORDOR_NOTREACHED();
		}
	} else {
		switch (getType(column)) {
			case INT8OID:
			case INT2OID:
			case INT4OID:
				return boost::lexical_cast<long long>(PQgetvalue(m_result.get(), (int)row, (int)column));
			default:
				MORDOR_NOTREACHED();
		}
	}
}

template <>
short
Result::get<short>(size_t row, size_t column) const
{
    MORDOR_ASSERT(getType(column) == INT2OID);
	if (m_format == BINARY || PQbinaryTuples(m_result.get()) == 1) {
		MORDOR_ASSERT(PQgetlength(m_result.get(), (int)row, (int)column) == 2);
		return byteswapOnLittleEndian(*(short *)PQgetvalue(m_result.get(), (int)row, (int)column));
	} else {
		return boost::lexical_cast<short>(PQgetvalue(m_result.get(), (int)row, (int)column));
	}
}

template <>
int
Result::get<int>(size_t row, size_t column) const
{
	if (m_format == BINARY || PQbinaryTuples(m_result.get()) == 1) {
		switch (getType(column)) {
			case INT2OID:
				MORDOR_ASSERT(PQgetlength(m_result.get(), (int)row, (int)column) == 2);
				return byteswapOnLittleEndian(*(short *)PQgetvalue(m_result.get(), (int)row, (int)column));
			case INT4OID:
				MORDOR_ASSERT(PQgetlength(m_result.get(), (int)row, (int)column) == 4);
				return byteswapOnLittleEndian(*(int *)PQgetvalue(m_result.get(), (int)row, (int)column));
			default:
				MORDOR_NOTREACHED();
		}
	} else {
		switch (getType(column)) {
			case INT2OID:
			case INT4OID:
				return boost::lexical_cast<int>(PQgetvalue(m_result.get(), (int)row, (int)column));
			default:
				MORDOR_NOTREACHED();
		}
	}
}

template <>
float
Result::get<float>(size_t row, size_t column) const
{
    MORDOR_ASSERT(getType(column) == FLOAT4OID);
	if (m_format == BINARY || PQbinaryTuples(m_result.get()) == 1) {
		MORDOR_ASSERT(PQgetlength(m_result.get(), (int)row, (int)column) == 4);
		int temp = byteswapOnLittleEndian(*(int *)PQgetvalue(m_result.get(), (int)row, (int)column));
		return *(float *)&temp;
	} else {
		return boost::lexical_cast<float>(PQgetvalue(m_result.get(), (int)row, (int)column));
	}
}

template <>
double
Result::get<double>(size_t row, size_t column) const
{
	if (m_format == BINARY || PQbinaryTuples(m_result.get()) == 1) {
		int templ;
		long long templl;
		switch (getType(column)) {
			case FLOAT4OID:
				MORDOR_ASSERT(PQgetlength(m_result.get(), (int)row, (int)column) == 4);
				templ = byteswapOnLittleEndian(*(int *)PQgetvalue(m_result.get(), (int)row, (int)column));
				return *(float *)&templ;
			case FLOAT8OID:
				MORDOR_ASSERT(PQgetlength(m_result.get(), (int)row, (int)column) == 8);
				templl = byteswapOnLittleEndian(*(long long *)PQgetvalue(m_result.get(), (int)row, (int)column));
				return *(double *)&templl;
			default:
				MORDOR_NOTREACHED();
		}
	} else {
		switch (getType(column)) {
			case FLOAT4OID:
			case FLOAT8OID:
				return boost::lexical_cast<double>(PQgetvalue(m_result.get(), (int)row, (int)column));
			default:
				MORDOR_NOTREACHED();
		}
	}
}

static const boost::posix_time::ptime postgres_epoch(boost::gregorian::date(2000, 1, 1));

template<>
boost::posix_time::ptime
Result::get<boost::posix_time::ptime>(size_t row, size_t column) const
{
    MORDOR_ASSERT(getType(column) == TIMESTAMPOID ||
        getType(column) == TIMESTAMPTZOID);
	if (m_format == BINARY || PQbinaryTuples(m_result.get()) == 1) {
		if (PQgetlength(m_result.get(), (int)row, (int)column) == 0)
			return boost::posix_time::ptime();
		MORDOR_ASSERT(PQgetlength(m_result.get(), (int)row, (int)column) == 8);
		long long microseconds = byteswapOnLittleEndian(*(long long *)PQgetvalue(m_result.get(), (int)row, (int)column));
		return postgres_epoch +
			boost::posix_time::seconds((long)(microseconds / 1000000)) +
			boost::posix_time::microseconds(microseconds % 1000000);
	} else {
		return boost::posix_time::time_from_string(PQgetvalue(m_result.get(), (int)row, (int)column));
	}
}

template<>
std::vector<int>
Result::get<std::vector<int> >(size_t row, size_t column) const
{
    std::vector<int> result;
    MORDOR_ASSERT(getType(column) == INT4ARRAYOID);
	if (m_format == BINARY || PQbinaryTuples(m_result.get()) == 1) {
		MORDOR_ASSERT(PQgetlength(m_result.get(), (int)row, (int)column) >= 12);
		const int *array = (const int *)PQgetvalue(m_result.get(), (int)row, (int)column);
		// No embedded NULLs
		MORDOR_ASSERT(array[1] == 0);
		// Correct element type
		MORDOR_ASSERT(byteswapOnLittleEndian(array[2]) == INT4OID);
		// Number of dimensions
		switch (byteswapOnLittleEndian(array[0])) {
			case 0:
				return result;
			case 1:
				MORDOR_ASSERT(PQgetlength(m_result.get(), (int)row, (int)column) >= 20);
				break;
			default:
				MORDOR_NOTREACHED();
		}
		int numberOfElements = byteswapOnLittleEndian(array[3]);
		// Ignore starting index
		array = &array[5];
		// Now verify we have the entire array, as described
		MORDOR_ASSERT(PQgetlength(m_result.get(), (int)row, (int)column) == 20 + numberOfElements * 8);
		result.resize(numberOfElements);
		for (int i = 0; i < numberOfElements; ++i) {
			// Correct element size
			MORDOR_ASSERT(byteswapOnLittleEndian(array[i * 2]) == 4);
			result[i] = byteswapOnLittleEndian(array[i * 2 + 1]);
		}
	} else {
		std::string res(PQgetvalue(m_result.get(), (int)row, (int)column));
		MORDOR_ASSERT(res.size() >=2);
		if (res.size() > 2){
			size_t lastpos = 1;
			size_t nextpos = res.find(',');
			while(nextpos != std::string::npos){
				result.push_back( boost::lexical_cast<int>(res.substr(lastpos, nextpos-lastpos)));
				lastpos = nextpos+1;
				MORDOR_ASSERT(lastpos < res.size());
			}
			result.push_back( boost::lexical_cast<int>(res.substr(lastpos, res.size()-1-lastpos)));
		}
	}
    return result;
}

template<>
std::pair<IPAddress::ptr, unsigned int>
Result::get<std::pair<IPAddress::ptr, unsigned int> >(size_t row, size_t column) const
{
    MORDOR_ASSERT(getType(column) == INETOID || getType(column) == CIDROID);
	if (m_format == BINARY || PQbinaryTuples(m_result.get()) == 1) {
		MORDOR_ASSERT(PQgetlength(m_result.get(), (int)row, (int)column) >= 2);
		const char *bytes = PQgetvalue(m_result.get(), (int)row, (int)column);
		std::pair<IPAddress::ptr, unsigned int> result;
		switch (bytes[0]) {
			case AF_INET:
				MORDOR_ASSERT(PQgetlength(m_result.get(), (int)row, (int)column) == 8);
				result.second = bytes[1];
				result.first.reset(new IPv4Address(byteswapOnLittleEndian(*(unsigned int*)&bytes[4])));
				return result;
			case AF_INET + 1:
				MORDOR_ASSERT(PQgetlength(m_result.get(), (int)row, (int)column) == 20);
				result.second = bytes[1];
				result.first.reset(new IPv6Address((const unsigned char *)&bytes[4]));
				return result;
			default:
				MORDOR_NOTREACHED();
		}
	} else {
		std::string res(PQgetvalue(m_result.get(), (int)row, (int)column));
		size_t netDeviderPos = res.find('/');
		unsigned int net=0;
		if (netDeviderPos != std::string::npos){
			net = boost::lexical_cast<unsigned int>(res.substr(netDeviderPos+1));
			res.resize(netDeviderPos);
		}
		if (res.find(':') != std::string::npos){
			return std::make_pair<IPAddress::ptr, unsigned int>(IPAddress::ptr(new IPv6Address(res.c_str())), net);
		}
		return std::make_pair<IPAddress::ptr, unsigned int>(IPAddress::ptr(new IPv4Address(res.c_str())), net);
	}
}

template<>
IPAddress::ptr
Result::get<IPAddress::ptr>(size_t row, size_t column) const
{
    return get<std::pair<IPAddress::ptr, unsigned int> >(row, column).first;
}

}}
