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

#include "mgos_alarm.h"

/*
 * digital_alarm structure
 * 
 * active - is the alarm currently active
 * enabled - is the alarm enabled
 * *input - pointer to the alarm trigger boolean
 * mode - is the alarm active high or low 
 * set_interval - the period that the trigger must be active for the alarm to be set
 * reset_interval - the period that the trigger must be false for the alarm to be reset
 * *name - the name of the alarm
 * timer_id - the mgos timer id of the alarm
 * LIST ENTRY - the next alarm in the list part of the List data structure
 */
struct d_alarm_info{
  bool active, enabled;
  bool *input; 
  enum mgos_d_alarm_mode mode; 
  int set_interval, reset_interval;
  char *name;
  mgos_timer_id timer_id;
  LIST_ENTRY (d_alarm_info) d_alarm_entries;
};

/*
 * digital alarm list structure
 */
struct d_alarm_data{
  struct d_alarm_info *current;
  int length;
  LIST_HEAD(d_alarms, d_alarm_info) d_alarms;
};

static struct d_alarm_data *s_d_alarm_data = NULL;
static struct mgos_rlock_type *s_d_alarm_data_lock = NULL;

/*
 * analog_alarm structure
 * 
 * enabled - is the alarm enabled
 * state - the current state of the alarm as defined by the mgos_a_alarm_state enum
 * *pv - pointer to the alarm process value that triggers alarms
 * set_interval - the period that the trigger must be active for the alarm to be set
 * reset_interval - the period that the trigger must be false for the alarm to be reset
 * *name - the name of the alarm
 * timer_id - the mgos timer id of the alarm
 * LIST ENTRY - the next alarm in the list part of the List data structure
 */
struct a_alarm_info{
  bool enabled;
  enum mgos_a_alarm_state state;
  float *pv, ll_sv, l_sv, h_sv, hh_sv; 
  int set_interval;
  char *name;
  mgos_timer_id up_timer_id, down_timer_id;
  LIST_ENTRY (a_alarm_info) a_alarm_entries;
};

/*
 * analog alarm list structure
 */
struct a_alarm_data{
  struct a_alarm_info *current;
  int length;
  LIST_HEAD(a_alarms, a_alarm_info) a_alarms;
};

static struct a_alarm_data *s_a_alarm_data = NULL;
static struct mgos_rlock_type *s_a_alarm_data_lock = NULL;

/*
 * Add an analog alarm to the analog alarm list
 * 
 */
bool mgos_add_a_alarm(bool enabled, float *pv, float ll_sv, float l_sv,
                      float h_sv, float hh_sv, int set_interval, 
                      char *name){
  //ensure the name is not null
  if(name == NULL){
    LOG(LL_ERROR, ("Analog alarm failed to init as name is NULL"));
    return false;
  }
  //ensure the name is not empty
  if(strcmp(name, "") == 0){
    LOG(LL_ERROR, ("Analog alarm failed to init as name is empty"));
    return false;
  }
  //allocate memory for struck
  struct a_alarm_info *aa_info = (struct a_alarm_info *) calloc(1, sizeof(*aa_info));
  //ensure the accolated memory is not null
  if(aa_info == NULL){
    LOG(LL_ERROR, ("Analog alarm \"%*s\" failed to init as allocated memory returned NULL", strlen(name) , name));
    return false;
  } 
  //iterate through the alarm list to ensure that the alarm has a unique name 
  struct a_alarm_info *aa_info_2;
  LIST_FOREACH(aa_info_2, &s_a_alarm_data->a_alarms, a_alarm_entries) {
    if(strcmp(name, aa_info_2->name) == 0){
      LOG(LL_ERROR, ("Analog alarm \"%*s\" failed to init as name is not unique", strlen(name) , name));
      free(aa_info);
      return false;
    }
  }
  //ensure the alarm sv levels are set correctly ll_sv < l_sv < h_sv < hh_sv
  float tf = ll_sv;
  float arr[] = {l_sv, h_sv, hh_sv};
  for(int i = 0; i < 3; i++){
    if(arr[i] != NAN){
      if(tf != NAN){
        if(tf > arr[i]){
          LOG(LL_ERROR, ("Analog alarm \"%*s\" failed to init due to invalid sv values", strlen(name) , name));
          return false;
        }
      }
      tf = arr[i];
    }
  }
  if(tf == NAN){
    LOG(LL_ERROR, ("Analog alarm \"%*s\" failed to init as all sv values NAN", strlen(name) , name));
    return false;
  }
  //set alarm struct vars
  aa_info->name = name;
  aa_info->enabled = enabled;
  aa_info->pv = pv;
  aa_info->ll_sv = ll_sv;
  aa_info->l_sv = l_sv;
  aa_info->h_sv = h_sv;
  aa_info->hh_sv = hh_sv;
  //increment analog list length
  ++s_a_alarm_data->length;
  //ensure that the set & reset intervals are at least 0ms
  if(set_interval < 0) set_interval = 0;
  aa_info->set_interval = set_interval;
  //insert the alarm into the list
  mgos_rlock(s_a_alarm_data_lock);
  LOG(LL_ERROR, ("Analog alarm \"%*s\"", strlen(name) , name));
  LIST_INSERT_HEAD(&s_a_alarm_data->a_alarms, aa_info, a_alarm_entries);
  mgos_runlock(s_a_alarm_data_lock);
  return true;
}

/*
 * Add a digital alarm to the digital alarm list
 * 
 */
bool mgos_add_d_alarm(bool enabled, bool *input, enum mgos_d_alarm_mode mode,
                      int set_interval, int reset_interval, char *name){
  //ensure the name is not null
  if(name == NULL){
    LOG(LL_ERROR, ("Digital alarm failed to init as name is NULL"));
    return false;
  }
  //ensure the name is not empty
  if(strcmp(name, "") == 0){
    LOG(LL_ERROR, ("Digital alarm failed to init as name is empty"));
    return false;
  }
  //allocate memory for struck
  struct d_alarm_info *da_info = (struct d_alarm_info *) calloc(1, sizeof(*da_info));
  //ensure the accolated memory is not null
  if(da_info == NULL){
    LOG(LL_ERROR, ("Digital alarm \"%*s\" failed to init as allocated memory returned NULL", strlen(name) , name));
    return false;
  } 
  //iterate through the alarm list to ensure that the alarm has a unique name 
  struct d_alarm_info *da_info2;
  LIST_FOREACH(da_info2, &s_d_alarm_data->d_alarms, d_alarm_entries) {
    if(strcmp(name, da_info2->name) == 0){
      LOG(LL_ERROR, ("Digital alarm \"%*s\" failed to init as an alarm with this name already exists", strlen(name) , name));
      free(da_info);
      return false;
    }
  }
  //set alarm struct vars
  da_info->name = name;
  da_info->enabled = enabled;
  da_info->input = input;
  da_info->mode = mode;
  //increment digital list length
  ++s_d_alarm_data->length;
  //ensure that the set & reset intervals are at least 0ms
  if(set_interval < 0) set_interval = 0;
  da_info->set_interval = set_interval;
  if(reset_interval < 0) reset_interval = 0;
  da_info->reset_interval = reset_interval;
  //insert the alarm into the list
  mgos_rlock(s_d_alarm_data_lock);
  LIST_INSERT_HEAD(&s_d_alarm_data->d_alarms, da_info, d_alarm_entries);
  mgos_runlock(s_d_alarm_data_lock);
  return true;
}

/*
 * Remove an alarm from the alarm lists if it has a name identical
 * to passed string 
 */
bool mgos_remove_alarm(char *name){
  if(name == NULL) return false;

  //loop through digital alarms and check if an alarm with passed name exists
  struct d_alarm_info *da_info;
  mgos_rlock(s_d_alarm_data_lock);
  LIST_FOREACH(da_info, &s_d_alarm_data->d_alarms, d_alarm_entries) {
    if(strcmp(name, da_info->name) == 0){
      LIST_REMOVE(da_info, d_alarm_entries);
      mgos_runlock(s_d_alarm_data_lock);
      LOG(LL_INFO, ("Digital alarm \"%*s\" has been removed", strlen(name) , name));
      if(da_info != NULL) free(da_info);
      //decrement digital alarm list length
      --s_d_alarm_data->length;
      return true;
    }
  }
  mgos_runlock(s_d_alarm_data_lock);

  //loop through analog alarms and check if an alarm with passed name exists
  struct a_alarm_info *aa_info;
  mgos_rlock(s_a_alarm_data_lock);
  LIST_FOREACH(aa_info, &s_a_alarm_data->a_alarms, a_alarm_entries) {
    if(strcmp(name, aa_info->name) == 0){
      LIST_REMOVE(aa_info, a_alarm_entries);
      mgos_runlock(s_a_alarm_data_lock);
      LOG(LL_INFO, ("Analog alarm \"%*s\" has been removed", strlen(name) , name));
      if(aa_info != NULL) free(aa_info);
      //decrement analog alarm list length
      --s_a_alarm_data->length;
      return true;
    }
  }
  mgos_runlock(s_a_alarm_data_lock);

  //return false if the alarm does not exist
  LOG(LL_INFO, ("Alarm \"%*s\" does not exist", strlen(name) , name));
  return false;
}

/*
 * Disable an alarm from the alarm list with the passed name
 * returns true if the alarm is disabled
 * returns false otherwise
 * 
 */
bool mgos_disable_alarm(char *name){
  if(name == NULL) return false;

  //loop through digital alarms and check if an alarm with passed name exists
  struct d_alarm_info *da_info;
  mgos_rlock(s_d_alarm_data_lock);
  LIST_FOREACH(da_info, &s_d_alarm_data->d_alarms, d_alarm_entries) {
    if(strcmp(name, da_info->name) == 0){
      da_info->active = false;
      da_info->enabled = false;
      mgos_runlock(s_d_alarm_data_lock);
      LOG(LL_INFO, ("Digital alarm \"%*s\" has been disabled", strlen(name) , name));
      return true;
    }
  }
  mgos_runlock(s_d_alarm_data_lock);

  //loop through analog alarms and check if an alarm with passed name exists
  struct a_alarm_info *aa_info;
  mgos_rlock(s_a_alarm_data_lock);
  LIST_FOREACH(aa_info, &s_a_alarm_data->a_alarms, a_alarm_entries) {
    if(strcmp(name, aa_info->name) == 0){
      aa_info->state = NOM;
      aa_info->enabled = false;
      mgos_runlock(s_a_alarm_data_lock);
      LOG(LL_INFO, ("Analog alarm \"%*s\" has been disabled", strlen(name) , name));
      return true;
    }
  }
  mgos_runlock(s_a_alarm_data_lock);

  //return false if the alarm does not exist
  LOG(LL_INFO, ("Alarm \"%*s\" does not exist", strlen(name) , name));
  return false;
}

/*
 * Reset an alarm from the alarm list with the passed name
 * returns true if the alarm is reset
 * returns false otherwise
 */
bool mgos_reset_alarm(char *name){
  if(name == NULL) return false;

  //loop through digital alarms and check if an alarm with passed name exists
  struct d_alarm_info *da_info;
  mgos_rlock(s_d_alarm_data_lock);
  LIST_FOREACH(da_info, &s_d_alarm_data->d_alarms, d_alarm_entries) {
    if(strcmp(name, da_info->name) == 0){
      da_info->active = false;
      mgos_runlock(s_d_alarm_data_lock);
      LOG(LL_INFO, ("Digital alarm \"%*s\" has been reset", strlen(name) , name));
      return true;
    }
  }
  mgos_runlock(s_d_alarm_data_lock);

  //loop through analog alarms and check if an alarm with passed name exists
  struct a_alarm_info *aa_info;
  mgos_rlock(s_a_alarm_data_lock);
  LIST_FOREACH(aa_info, &s_a_alarm_data->a_alarms, a_alarm_entries) {
    if(strcmp(name, aa_info->name) == 0){
      aa_info->state = NOM;
      mgos_runlock(s_a_alarm_data_lock);
      LOG(LL_INFO, ("Analog alarm \"%*s\" has been reset", strlen(name) , name));
      return true;
    }
  }
  mgos_runlock(s_a_alarm_data_lock);

  //return false if the alarm does not exist
  LOG(LL_INFO, ("Alarm \"%*s\" does not exist", strlen(name) , name));
  return false;
}

/*
 * Returns an array of alarm_info structs based on alarms in the alarm list
 * returns null if no alarms are returned
 */
struct alarm_list * mgos_list_alarms(void){
  //calc total number of alarms and allocate memory for them
  int total_alarms = s_a_alarm_data->length + s_d_alarm_data->length;
  //LOG(LL_INFO, ("%i", total_alarms));
  struct alarm_list *a_list = calloc(1, sizeof(*a_list));
  struct alarm_info *a_info = calloc(total_alarms, sizeof(*a_info));
  a_list->info = a_info;
  a_list->length = 0;

  //loop through digital alarms and 
  struct d_alarm_info *da_info;
  mgos_rlock(s_d_alarm_data_lock);
  LIST_FOREACH(da_info, &s_d_alarm_data->d_alarms, d_alarm_entries) {
    //LOG(LL_INFO, ("name %*s", strlen(da_info->name), da_info->name));
    a_info[a_list->length].name = da_info->name;
    a_info[a_list->length].enabled = da_info->enabled;
    a_info[a_list->length].type = DIGITAL;
    a_info[a_list->length].state.d_state = da_info->active;
    ++a_list->length;
  }
  LOG(LL_INFO, ("%i", a_list->length));
  mgos_runlock(s_d_alarm_data_lock);

  //loop through analog alarms and check if an alarm with passed name exits
  struct a_alarm_info *aa_info;
  mgos_rlock(s_a_alarm_data_lock);
  LIST_FOREACH(aa_info, &s_a_alarm_data->a_alarms, a_alarm_entries) {
    a_info[a_list->length].name = aa_info->name;
    a_info[a_list->length].enabled = aa_info->enabled;
    a_info[a_list->length].type = ANALOG;
    a_info[a_list->length].state.a_state = aa_info->state;
    ++a_list->length;
  }
  mgos_runlock(s_a_alarm_data_lock);

  return a_list;
}

/*
 * Digital alarm timer callback function
 */
static void d_alarm_timer(void *arg) {
  struct d_alarm_info *da_info = (struct d_alarm_info *) arg;
  //toggle the alarm state
  da_info->active = !da_info->active;
  //build generic alarm info struct 
  //allocate memory for struc
  struct alarm_info *a_info = (struct alarm_info *) calloc(1, sizeof(*a_info));
  //set struct parameters
  a_info->name = da_info->name;
  a_info->enabled = da_info->enabled;
  a_info->type = DIGITAL;
  a_info->state.d_state = da_info->active ;
  //if the alarm is now active trigger set ev else trigger reset ev
  if(da_info->active){
    mgos_event_trigger(MGOS_ALARM_EV_SET, a_info);
  }
  else{
    mgos_event_trigger(MGOS_ALARM_EV_RESET, a_info);
  }
}

/*
 * Digital alarm set/reset timer logic
 */
static void mgos_d_alarm_logic(struct d_alarm_info *da_info) {
  bool trigger = false;
  if((da_info->mode == ACTIVE_HIGH && *da_info->input) ||
    (da_info->mode == ACTIVE_LOW && !(*da_info->input))){
    trigger = true;
  }
  //if the alarm is active
  if(da_info->active){
    //if the input is high and the reset timer has been started clear it
    if(trigger && da_info->timer_id != MGOS_INVALID_TIMER_ID){
      mgos_clear_timer(da_info->timer_id);
      da_info->timer_id = MGOS_INVALID_TIMER_ID;
    }
    //if the input is low start the reset timer
    else if(!trigger && da_info->timer_id == MGOS_INVALID_TIMER_ID){
      da_info->timer_id = mgos_set_timer(da_info->reset_interval, 0, d_alarm_timer, da_info);
    }
    return;
  }
  //if the input is low and the set timer has been started clear it
  if(!trigger && da_info->timer_id != MGOS_INVALID_TIMER_ID){
    mgos_clear_timer(da_info->timer_id);
    da_info->timer_id = MGOS_INVALID_TIMER_ID;
  }
  //if the input is high start the set timer 
  else if(trigger && da_info->timer_id == MGOS_INVALID_TIMER_ID){
    da_info->timer_id = mgos_set_timer(da_info->set_interval, 0, d_alarm_timer, da_info);
  }
}

/*
 * Analog alarm timer callback function
 */
static void a_alarm_timer(void *arg) {

}

/*
 * Analog alarm set/reset timer logic
 */
static void mgos_a_alarm_logic(struct a_alarm_info *aa_info) {
  if(aa_info->h_sv != NAN && *aa_info->pv >= aa_info->h_sv && *aa_info->pv < aa_info->hh_sv){
    if(aa_info->state != H && aa_info->up_timer_id == MGOS_INVALID_TIMER_ID){
      aa_info->up_timer_id = mgos_set_timer(aa_info->set_interval, 0, a_alarm_timer, aa_info);
    }
    if(aa_info->down_timer_id != MGOS_INVALID_TIMER_ID){
      aa_info->down_timer_id = mgos_set_timer(aa_info->set_interval, 0, a_alarm_timer, aa_info);
    }
    return;
  }
  else if(aa_info->l_sv != NAN && *aa_info->pv <= aa_info->l_sv && *aa_info->pv > aa_info->ll_sv){
    if(aa_info->state != L && aa_info->down_timer_id == MGOS_INVALID_TIMER_ID){
      aa_info->down_timer_id = mgos_set_timer(aa_info->set_interval, 0, a_alarm_timer, aa_info);
    }
    if(aa_info->up_timer_id != MGOS_INVALID_TIMER_ID){
      aa_info->up_timer_id = mgos_set_timer(aa_info->set_interval, 0, a_alarm_timer, aa_info);
    }      
    return;
  }
  else if(aa_info->hh_sv != NAN && *aa_info->pv >= aa_info->hh_sv){
    if(aa_info->state != HH && aa_info->down_timer_id == MGOS_INVALID_TIMER_ID){
      aa_info->down_timer_id = mgos_set_timer(aa_info->set_interval, 0, a_alarm_timer, aa_info);
    }
    if(aa_info->up_timer_id != MGOS_INVALID_TIMER_ID){
      aa_info->up_timer_id = mgos_set_timer(aa_info->set_interval, 0, a_alarm_timer, aa_info);
    }    
    return;  
  }
  else if(aa_info->ll_sv != NAN && *aa_info->pv <= aa_info->ll_sv){
    if(aa_info->state != LL && aa_info->down_timer_id == MGOS_INVALID_TIMER_ID){
      aa_info->down_timer_id = mgos_set_timer(aa_info->set_interval, 0, a_alarm_timer, aa_info);
    }
    if(aa_info->up_timer_id != MGOS_INVALID_TIMER_ID){
      aa_info->up_timer_id = mgos_set_timer(aa_info->set_interval, 0, a_alarm_timer, aa_info);
    }     
    return; 
  }
}

/*
 * Main alarm service timer
 */
static void mgos_alarm_timer(void *arg) {
  //iterate through digital alarms
  struct d_alarm_info *da_info;
  mgos_rlock(s_d_alarm_data_lock);
  LIST_FOREACH(da_info, &s_d_alarm_data->d_alarms, d_alarm_entries) {
    if(da_info->enabled) mgos_d_alarm_logic(da_info);
  }
  mgos_runlock(s_d_alarm_data_lock);
  
  //loop through analog alarms and check if an alarm with passed name exists
  struct a_alarm_info *aa_info;
  mgos_rlock(s_a_alarm_data_lock);
  LIST_FOREACH(aa_info, &s_a_alarm_data->a_alarms, a_alarm_entries) {
    if(aa_info->enabled) mgos_a_alarm_logic(aa_info);
  }
  mgos_runlock(s_a_alarm_data_lock);
}

/*
 * Initilise alarm list, rlock and main timer routine
 */
bool mgos_alarm_init(int poll_interval) {
  //register alarm event base, exit if this fails
  if (!mgos_event_register_base(MGOS_EVENT_GRP_ALARM, "alm")) {
    return false;
  }
  //initilise digital and analog list headers
  struct d_alarm_data *da_data = (struct d_alarm_data *) calloc(1, sizeof(*da_data));
  struct a_alarm_data *aa_data = (struct a_alarm_data *) calloc(1, sizeof(*aa_data));
  //if memory is not allocated for either list exit
  if(da_data == NULL) return false;
  if(aa_data == NULL) return false;
  //set list header pointer to allocated memory pointer 
  s_d_alarm_data = da_data;
  s_d_alarm_data->length = 0;
  s_a_alarm_data = aa_data;
  s_a_alarm_data->length = 0;
  //create recursive lock 
  s_d_alarm_data_lock = mgos_rlock_create();
  s_a_alarm_data_lock = mgos_rlock_create();
  //set alarm master checker
  mgos_set_timer(poll_interval, MGOS_TIMER_REPEAT, mgos_alarm_timer, NULL);
  //init successful
  return true;
}