/*
 * Copyright (c) 2019 Neill Skelly
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Mongoose OS alarming utility library
 * 
 * An alarm is a boolean flag indicating that an error condition has arisen 
 * in a system. For example a high temperature alarm would indicate that a 
 * process temperature (PV) exceeds a desired setpoint (SV). When the PV
 * falls below the SV, the alarm should be cleared.
 * 
 * Tpically alarms are implemented with delay timers to ensure that a transient 
 * PV does not trigger an alarm warning.
 * 
 * The library supports two types of alarms:
 * - Digital alarms. Triggered by a boolean variable.  
 * - Analog alarms. Triggered by an analog variable.      
 * 
 */

#ifndef CS_FW_INCLUDE_MGOS_ALARM_H_
#define CS_FW_INCLUDE_MGOS_ALARM_H_

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "mgos.h"

/*
 * Event group which should be given to `mgos_event_add_group_handler()`
 * in order to subscribe to alarm events.
 *
 * Example:
 * ```c
 * static void my_alarm_ev_handler(int ev, void *evd, void *arg) {
 *   mgos_alarm_event ev = (mgos_alarm_event) ev;
 *   struct alarm_info *a_info = (struct alarm_info *) arg;
 *   switch(ev) :{
 *     case MGOS_ALARM_EV_SET) {
 *       LOG(LL_INFO, ("Alarm Name: /"%*s/"! - Type: %d", strlen(a_info->name) , a_info->name, a_info->type));
 *       //perform some logic, send alarm notification to user...
 *     }
 *     case MGOS_ALARM_EV_RESET) {
 *       LOG(LL_INFO, ("Alarm Name: /"%*s/"! - Type: %d", strlen(a_info->name) , a_info->name, a_info->type));
 *       //perform some logic, send alarm notification to user...
 *     }
 *   }
 *   (void) arg;
 * }
 *
 * // Somewhere else:
 * mgos_event_add_group_handler(MGOS_EVENT_GRP_ALARM, my_alarm_ev_handler, NULL);
 * ```
 */
#define MGOS_EVENT_GRP_ALARM MGOS_EVENT_BASE('A', 'L', 'M')

enum mgos_alarm_event{
  MGOS_ALARM_EV_RESET = MGOS_EVENT_GRP_ALARM,
  MGOS_ALARM_EV_SET
};

/*
 * Digital input mode
 * ACTIVE_LOW - alarm is triggered by digital pin going low
 * ACTIVE_HIGH - alarm is triggered by digital pin going high
 * 
 */
enum mgos_d_alarm_mode{
  ACTIVE_LOW,
  ACTIVE_HIGH
};

/*
 * Analog alarm state
 * NOM - nominal
 * LL - PV <= LL_SV 
 * L - LL_SV < PV <= L_SV 
 * H - HH_SV > PV >= H_SV 
 * HH - PV >= HH_SV
 * 
 */
enum mgos_a_alarm_state{
  NOM,
  LL,
  L,
  H,
  HH
};

/*
 * Alarm types
 * 
 */
enum mgos_alarm_type{
  DIGITAL,
  ANALOG
};

/*
 * Alarm info returned by mgos_list_alarms.
 * 
 * name - unique name of the alarm
 * enabled - whether the alarm is able to b e triggered
 * type - whether the alarm is digital or analog
 * state - a state union dependent on the alarm type,
 *   d_state is the digital alarm state and a_state is the 
 *   analog alarm state
 */
struct alarm_info{
  char *name;
  bool enabled;
  enum mgos_alarm_type type; 
  union{
    bool d_state;
    enum mgos_a_alarm_state a_state;
  } state;
};

/*
 * Alarm info returned by mgos_list_alarms.
 * 
 * name - unique name of the alarm
 * enabled - whether the alarm is able to b e triggered
 * type - whether the alarm is digital or analog
 * state - a state union dependent on the alarm type,
 *   d_state is the digital alarm state and a_state is the 
 *   analog alarm state
 */
struct alarm_list{
  struct alarm_info *info;
  size_t length;
};

/*
 * Add an analog alarm to the alarm list.
 * 
 * state - is the alarm currently active
 * enabled - is the alarm enabled
 * *pv - pointer to the alarm process value 
 * ll_sv - low low level setpoint
 * l_sv - low level setpoint
 * h_sv - high level setpoint
 * hh_sv - high high level setpoint
 * set_interval - the period that the pv must be greater (h_sv & hh_sv) or less (h_sv & hh_sv) 
 *   than the sv for the alarm to be set
 * reset_interval - the period that the pv must be less (h_sv & hh_sv) or greater (h_sv & hh_sv) 
 *   than the sv for the alarm to be reset
 * *name - the name of the alarm
 * 
 * ll_sv < l_sv < h_sv < hh_sv
 * If any of the sv values are set to NAN they will be omitted from set and reset logic
 * 
 */
bool mgos_add_a_alarm(bool enabled, float *pv, float ll_sv, float l_sv,
                      float h_sv, float hh_sv, int set_interval, 
                      char *name);

/*
 * Add a digital alarm to the alarm list.
 * 
 * enabled - If true the alarm can be triggered 
 * input - The pointer to the bool variable that triggers the alarm
 * mode - active high or low.
 * set_interval - how many ms the trigger must be active to trigger alarm.
 * reset_alarm - how many ms the trigger must be inactive to trigger alarm.
 * name - A unique (not empty) string
 * 
 * If either internal is negative the value defaults to 0
 * 
 * returns true if the alarm is added
 * returns false otherwise
 * 
 */
bool mgos_add_d_alarm(bool enabled, bool *input, enum mgos_d_alarm_mode mode,
                      int set_interval, int reset_interval, char *name);

/*
 * Remove an alarm from the alarm list with the passed name
 * returns true if the alarm is removed
 * returns false otherwise
 */
bool mgos_remove_alarm(char *name);

/*
 * Disable an alarm from the alarm list with the passed name
 * returns true if the alarm is disabled
 * returns false otherwise
 * 
 */
bool mgos_disable_alarm(char *name);

/*
 * Reset an alarm from the alarm list with the passed name
 * returns true if the alarm is reset
 * returns false otherwise
 */
bool mgos_reset_alarm(char *name);

/*
 * Returns an array of alarm_info structs based on alarms in the alarm list
 * returns null if no alarms are returned
 * caller must free returned struct
 */
 struct alarm_list * mgos_list_alarms(void);

/*
 * Initilise alarm list, rlock and main timer routine.
 * Must be called before any other mgos_alarm functions, typically in mgos_app_init().
 * Returns true if successfully intitilise
 * 
 * poll_interval - How often the trigger conditions should be checked and alarms
 *   updated. This should be around a factor of ten lower than the short0est set
 *   or reset interval of any alarms added  
 * 
 */
bool mgos_alarm_init(int poll_interval);

#endif /* CS_FW_INCLUDE_MGOS_ALARM_H_ */