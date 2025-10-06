#include "Util.h"

#include<iostream>
#include<cstring>
#include<signal.h>

void Util::handle_for_sigpipe()
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if(sigaction(SIGPIPE, &sa, NULL))return;
    else std::cerr<<"set ignore sigpipe failed"<<std::endl;
}