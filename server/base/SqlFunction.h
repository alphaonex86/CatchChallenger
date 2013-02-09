#include <QString>

#ifndef CATCHCHALLENGER_SQLFUNCTION_H
#define CATCHCHALLENGER_SQLFUNCTION_H

namespace CatchChallenger {
class SqlFunction
{
public:
	//do a fake login
	static QString quoteSqlVariable(QString variable);
};
}

#endif
