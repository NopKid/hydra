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

/* Key-value pair — the "extras" bundle in an Intent */
typedef struct {
    char key[MAX_STR];
    char value[MAX_STR];
} Extra;

/* The Intent structure */
typedef struct {
    char   action[MAX_STR];       /* what to do */
    char   target[MAX_STR];       /* component name, empty = implicit */
    Extra  extras[MAX_EXTRAS];    /* payload */
    int    extra_count;
    int    flags;                 /* behavior flags */
} Intent;

/* Intent flags — these are real Android flag values */
#define FLAG_NEW_TASK              0x10000000
#define FLAG_SINGLE_TOP            0x20000000
#define FLAG_TASK_ON_HOME          0x00004000  /* used in task hijacking */
#define FLAG_ACTIVITY_CLEAR_TASK   0x00008000

/* Simulate an app's component registration */
typedef struct {
    char name[MAX_STR];
    char handles_action[MAX_STR];
    int  exported;               /* 1 = any app can reach, 0 = private */
} Component;

/* Build an Intent — like new Intent() in Java */
Intent* create_intent(const char *action, const char *target) {
    Intent *intent = malloc(sizeof(Intent));
    memset(intent, 0, sizeof(Intent));
    strncpy(intent->action, action, MAX_STR - 1);
    if (target)
        strncpy(intent->target, target, MAX_STR - 1);
    return intent;
}

/* Add data to the Intent payload */
void put_extra(Intent *intent, const char *key, const char *value) {
    if (intent->extra_count >= MAX_EXTRAS) {
        printf("[-] Intent extras full\n");
        return;
    }
    Extra *e = &intent->extras[intent->extra_count++];
    strncpy(e->key, key, MAX_STR - 1);
    strncpy(e->value, value, MAX_STR - 1);
}

/* Get a value from Intent payload */
const char* get_extra(Intent *intent, const char *key) {
    for (int i = 0; i < intent->extra_count; i++) {
        if (strcmp(intent->extras[i].key, key) == 0)
            return intent->extras[i].value;
    }
    return NULL;
}

/* Simulate the Android system resolving an Intent */
Component* resolve_intent(Intent *intent, Component *components, int count) {
    printf("[*] System resolving intent: action='%s' target='%s'\n",
           intent->action,
           strlen(intent->target) ? intent->target : "(implicit)");

    /* Explicit intent — target is named directly */
    if (strlen(intent->target) > 0) {
        for (int i = 0; i < count; i++) {
            if (strcmp(components[i].name, intent->target) == 0) {
                if (!components[i].exported) {
                    printf("[-] BLOCKED: %s is not exported\n",
                           components[i].name);
                    return NULL;
                }
                printf("[+] Resolved to: %s\n", components[i].name);
                return &components[i];
            }
        }
        printf("[-] Component not found: %s\n", intent->target);
        return NULL;
    }

    /* Implicit intent — find ANY exported component that handles it */
    printf("[!] Implicit intent — checking all exported components...\n");
    Component *match = NULL;
    int matches = 0;

    for (int i = 0; i < count; i++) {
        if (components[i].exported &&
            strcmp(components[i].handles_action, intent->action) == 0) {
            printf("    -> Candidate: %s\n", components[i].name);
            match = &components[i];
            matches++;
        }
    }

    if (matches > 1) {
        printf("[!] AMBIGUOUS: %d apps can handle this!\n", matches);
        printf("[!] Android shows a chooser — attacker app appears in list\n");
    }

    return match;
}

/* =======================================
 * DEMO 1: Safe explicit intent
 * ======================================= */
void demo_safe_intent() {
    printf("\n===========================================\n");
    printf(" DEMO 1: Safe — Explicit Intent\n");
    printf("===========================================\n\n");

    /* Faiba app components */
    Component faiba_components[] = {
        {"co.ke.faiba4g.LoginActivity",         "MAIN",           1},
        {"co.ke.faiba4g.PasswordResetActivity", "RESET_PASSWORD", 1},
        {"co.ke.faiba4g.PaymentActivity",       "PAYMENT",        0},
    };

    /* Safe: explicit intent names the exact target */
    Intent *intent = create_intent(
        "RESET_PASSWORD",
        "co.ke.faiba4g.PasswordResetActivity"
    );
    put_extra(intent, "reset_token", "tok_abc123");
    put_extra(intent, "user_email",  "user@faiba.co.ke");

    Component *resolved = resolve_intent(intent, faiba_components, 3);
    if (resolved) {
        const char *token = get_extra(intent, "reset_token");
        printf("[+] PasswordResetActivity received token: %s\n", token);
        printf("[+] Safe — only Faiba handles this\n");
    }

    free(intent);
}

/* =======================================
 * DEMO 2: Vulnerable implicit intent
 * ======================================= */
void demo_implicit_intent_hijack() {
    printf("\n===========================================\n");
    printf(" DEMO 2: VULNERABLE — Implicit Intent\n");
    printf("===========================================\n\n");

    /* All components visible to the system — including attacker */
    Component all_components[] = {
        {"co.ke.faiba4g.PasswordResetActivity", "RESET_PASSWORD", 1},
        {"com.attacker.evil.StealerActivity",   "RESET_PASSWORD", 1},
    };

    /* Vulnerable: implicit intent — no target named
     * Faiba sends the reset token without specifying who gets it */
    Intent *intent = create_intent("RESET_PASSWORD", NULL);
    put_extra(intent, "reset_token", "tok_abc123");
    put_extra(intent, "user_email",  "user@faiba.co.ke");

    printf("[*] Faiba sends token via IMPLICIT intent...\n\n");

    Component *resolved = resolve_intent(intent, all_components, 2);
    if (resolved) {
        const char *token = get_extra(intent, "reset_token");
        printf("\n[!] %s received token: %s\n",
               resolved->name, token);

        if (strstr(resolved->name, "attacker")) {
            printf("[!] TOKEN STOLEN — attacker can now reset any account\n");
        }
    }

    free(intent);
}

/* =======================================
 * DEMO 3: Task hijacking flags
 * ======================================= */
void demo_task_hijacking() {
    printf("\n===========================================\n");
    printf(" DEMO 3: Task Hijacking via Flags\n");
    printf("===========================================\n\n");

    printf("[*] Attacker app crafts Intent with hijack flags...\n\n");

    Intent *intent = create_intent(
        "RESET_PASSWORD",
        "co.ke.faiba4g.PasswordResetActivity"
    );

    /* These flags manipulate the Android back stack */
    intent->flags = FLAG_NEW_TASK | FLAG_TASK_ON_HOME;

    printf("    Flags set:\n");
    if (intent->flags & FLAG_NEW_TASK)
        printf("    -> FLAG_NEW_TASK       : launch in new task stack\n");
    if (intent->flags & FLAG_TASK_ON_HOME)
        printf("    -> FLAG_TASK_ON_HOME   : inject into home task\n");

    printf("\n[!] Effect: When victim presses HOME then reopens Faiba,\n");
    printf("    they land on attacker-controlled screen instead\n");
    printf("    of Faiba's real UI.\n");
    printf("[!] Victim types credentials into attacker's fake screen.\n");
    printf("[!] This is Task Affinity Attack — a real CVE class.\n");

    free(intent);
}

int main() {
    printf("\n");
    printf("╔══════════════════════════════════════════╗\n");
    printf("║   INTENT ATTACK SIMULATOR                ║\n");
    printf("║   V0ID Android Research Toolkit          ║\n");
    printf("╚══════════════════════════════════════════╝\n");

    demo_safe_intent();
    demo_implicit_intent_hijack();
    demo_task_hijacking();

    printf("\n[*] Key takeaway:\n");
    printf("    Explicit intents   = safe (named target)\n");
    printf("    Implicit intents   = dangerous (any app intercepts)\n");
    printf("    Exported + no perm = open door for attackers\n\n");

    return 0;
}