#include "CliClientOptions.hpp"

QString CliClientOptions::serverName;
QString CliClientOptions::host;
uint16_t CliClientOptions::port=0;
bool CliClientOptions::autologin=false;
QString CliClientOptions::characterName;
bool CliClientOptions::closeWhenOnMap=false;
int CliClientOptions::closeWhenOnMapAfter=0;
bool CliClientOptions::dropSendDataAfterOnMap=false;
bool CliClientOptions::autosolo=false;
