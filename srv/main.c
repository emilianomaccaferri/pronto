#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include "lib/h/config.h"
#include "lib/h/pronto.h"

int main(void){

    struct pronto* pronto_instance = malloc(sizeof(struct pronto));
    pronto_init(
        pronto_instance,
        CLUSTER_WORKERS
    );
    pronto_start_http(pronto_instance);
}
