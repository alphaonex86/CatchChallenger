#include <QString>

#ifndef POKECRAFT_SQLFUNCTION_H
#define POKECRAFT_SQLFUNCTION_H

namespace Pokecraft {
class SqlFunction
{
public:
	//do a fake login
	static QString quoteSqlVariable(QString variable);
};
}

#endif
