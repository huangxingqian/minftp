#include "tunable.h"

int tunable_pasr_enable = 1;
int tunable_port_enable = 1;
unsigned int tunable_listen_port = 21;
unsigned int tunable_max_clients = 2000;
unsigned int tunable_max_per_ip = 50;
unsigned int tunable_accept_tineout = 60;
unsigned int tunable_connect_tineout = 60;
unsigned int tunable_idle_session_tineout = 300;
unsigned int tunable_data_connection_tineout = 700;
unsigned int tunable_local_unask = 077;
unsigned int tunable_upload_max_rate = 0;
unsigned int tunable_download_max_rate = 0;
const char *tunable_listen_address="192.168.1.100";