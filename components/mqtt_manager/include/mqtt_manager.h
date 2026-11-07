#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

void mqtt_app_start(const char *broker_uri);
void mqtt_send_status(const char *topic, const char *message);

#endif