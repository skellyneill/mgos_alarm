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

#include <stdio.h>
#include <string.h>

#include "mgos.h"
#include "mgos_timers.h"
#include "mgos_alarm.h"
#include "mgos_adc.h"

bool input_1 = false;
bool input_2 = false;
bool input_3 = false;
int input_1_pin = 13;
int input_2_pin = 33;
int input_3_pin = 27;

float input_4 = 0;
float input_5 = 0;
int input_4_pin = 26;
int input_5_pin = 34;

static void input_checker_cb(void *arg){
  input_1 = mgos_gpio_read(input_1_pin);
  input_2 = mgos_gpio_read(input_2_pin);
  input_3 = mgos_gpio_read(input_3_pin);
  input_4 = mgos_gpio_read(input_4_pin);
  input_5 = mgos_gpio_read(input_5_pin);
  LOG(LL_INFO, ("name: %i", mgos_adc_read(input_5_pin)));
}

enum mgos_app_init_result mgos_app_init(void){

  if(!mgos_alarm_init(500)) return false;

  mgos_gpio_setup_input(input_1_pin, MGOS_GPIO_PULL_DOWN);
  mgos_gpio_setup_input(input_2_pin, MGOS_GPIO_PULL_DOWN);
  mgos_gpio_setup_input(input_3_pin, MGOS_GPIO_PULL_DOWN);

  mgos_adc_enable(input_4_pin);
  mgos_adc_enable(input_5_pin);

  mgos_set_timer(1000, MGOS_TIMER_REPEAT, input_checker_cb, NULL);

 // mgos_set_timer(5000, MGOS_TIMER_REPEAT, status_timer, NULL);

  mgos_add_d_alarm(true, &input_1, ACTIVE_HIGH, 1000, 1000, "alarm1");
  mgos_add_d_alarm(true, &input_2, ACTIVE_HIGH, 2000, 2000, "alarm2");
  mgos_add_d_alarm(true, &input_3, ACTIVE_LOW, 3000, 3000, "alarm3");

  mgos_add_a_alarm(true, &input_4, 0.2, 0.3, 0.4, 0.5, 1000, "alarm4");
  mgos_add_a_alarm(true, &input_5, 0.2, 0.3, 0.4, 0.5, 1000, "alarm5");

  struct alarm_list *list =  mgos_list_alarms();
  for(size_t i = 0; i < list->length; i++){
    LOG(LL_INFO, ("name: %*s", strlen(list->info[i].name), list->info[i].name));
  }
  free(list);

  // mgos_remove_alarm("alarm5");
  // mgos_remove_alarm("alm5");
  // mgos_remove_alarm("alarm1");
  // mgos_remove_alarm("alarm3");
  // mgos_remove_alarm("alarm4");

  // mgos_disable_alarm("alarm5");
  // mgos_disable_alarm("alarm2");
  // mgos_disable_alarm("alarm4");
  
  // struct alarm_list *list2 =  mgos_list_alarms();
  // for(size_t i = 0; i < list2->length; i++){
  //   LOG(LL_INFO, ("name: %*s", strlen(list2->info[i].name), list2->info[i].name));
  //   LOG(LL_INFO, ("enabled: %i", list2->info[i].enabled));
  //   if(list2->info[i].type == DIGITAL){
  //     LOG(LL_INFO, ("Digital State: %i", list2->info[i].state.d_state));
  //   }
  //   else{
  //     LOG(LL_INFO, ("ANALOG State: %i", list2->info[i].state.a_state));
  //   }
  // }
  //free(list2);

  return MGOS_APP_INIT_SUCCESS;
}