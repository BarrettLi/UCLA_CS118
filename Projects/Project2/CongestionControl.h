#include <stdbool.h>

#define SlowStart 1
#define CongestionAvoidance 2
#define FastRecovery 3


bool in_SS(int cc_state){
    return cc_state == SlowStart;
}

bool in_CA(int cc_state){
    return cc_state == CongestionAvoidance;
}

bool in_FR(int cc_state){
    return cc_state == FastRecovery;
}
