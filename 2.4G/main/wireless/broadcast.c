#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "esp_now.h"

uint8_t broadcast_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void broadcast_init() {
    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, broadcast_mac, 6);
    peer.channel = 1;
    peer.encrypt = false;
    
    if (!esp_now_is_peer_exist(broadcast_mac)) {
        esp_now_add_peer(&peer);
    }
}