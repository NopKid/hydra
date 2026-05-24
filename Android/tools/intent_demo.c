#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * INTENT SYSTEM SIMULATION
 *
 * Android Intents are essentially messages passed between
 * components. At their core they are just structured data:
 *
 *   - action    : what to do    ("RESET_PASSWORD", "VIEW", "SEND")
 *   - target    : who does it   (explicit = named, implicit = anyone)
 *   - extras    : the payload   (key-value pairs of data)
 *   - flags     : behavior bits (how the activity stack behaves)
 *
 * Binder IPC carries these messages between processes.
 * We simulate the concept here in pure C.
 */

#define MAX_EXTRAS    16
#define MAX_STR       256


/* Key-value pair the "extras" bundle in an intent */

typedef struct {
  char key[MAX_STR];
  char value[MAX_STR];

} Extras;

/* The intent structure */
typedef struct {
  char action[MAX_STR]; // what to do
  char target[MAX_STR]; // component name, empty = implicit
  Extras extras[MAX_EXTRAS]; // payload
  int extra_count;
  int flags; //behaviour flags

} Intent;

/* Intent flags -> these are real Android flags values */

#define FLAG_NEW_TASK               0x10000000
#define FLAG_SINGLE_TOP             0x20000000
#define FLAG_TASK_ON_HOME           0x00004000 // used in task hijacking
#define FLAG_ACTIVITY_CLEAR_TASK    0x00008000

/* Simulating an app's component registration */
typedef struct {
  char name[MAX_STR];
  char handle_action[MAX_STR];
  int exported;     /* 1 = any app can reach, 0 = private */

} Component;

/* Building an Intent, like new Intent() in Java */

Intent* create_intent(const char *action, const char *target) {
  Intent *intent = malloc(sizeof(Intent));
  memset(intent, 0, sizeof(Intent));
  strncpy(intent->action, action, MAX_STR - 1);
  if(target) {
    strncpy(intent->target, target, MAX_STR - 1);
  return intent;

  }
  Extra *e = &intent->etras[intent->extra count++];
  strncpy(e->key, key, MAX_STR - 1);
  strncpy(e->value, value, MAX_STR - 1);

}

/* Get a value from Intent payload */
const char* get_extra(Intent *intent, const char *key) {
  for (int i = 0; i < intent->extra_count; i++) {
    if (strncpy(int->extras[i].key, key) == 0)
      return intent->etxras[i].value;

  }
  
  return NULL;

}

/* Simulating the Android system resolving an Intent */
Component* resolve_intent(Intent *intent, Component *components, int count) {
  printf("[+] System resolving intent: action='%s' target='%s\n",
         intent->action,
         strlen(intent->target) ? intent->target : "(implicit)");

  /* Explicit intent, target is name directly */
  if (strlen(intent->target) > 0) {
    for (int i = 0; i < count; i++) {
      if (strcmp(components[i].name, intent->target) == 0) {
        if (!components[i].exported) {
          printf("[-] Blocked: %s is not exported\n", components[i].name);
          return NULL;

        }
      }
      printf("[+] Resolved to: %s\n", components[i].name);
      return &components[i];
    }
  }
  printf("[-] Component not found: %s\n", intent->target);
  return NULL;

}

/* Implicit intent, finding any exported component that handles it */
printf("[!] Implicit intent, checking all exported components...\n");
Components *match = NULL;
int matches = 0;


for (int = 0; i < count; i++) {
  if (components[i].exported &&
      strcmp(components[i].handle_action, intent->action) == 0) {
      printf("      -> Candidate: %s\n", components[i].name);
      match = &components[i];
      matches++;
  }
}

  if (matches > 1) {
    printf("[*] Ambiguous: %d apps can handle this!\n", matches);
    printf("[*] Let's go schizo mode\n");


  }
  return match;

}
