#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <json-c/json.h>  // Assure-toi d'avoir installé json-c

const int NB_LED = 400;         // Number of led (400m)
const int NB_LED_PER_SEGMENT = 50;
const int NB_SEGMENT = 8;

const int NB_MAX_TRACE = 1;

#define TARGET_PERIOD_US 10000  // 10 ms = 100 Hz

int running = 1;

int start_red = 0;
double red_period = 0.0;
double red_sum_time = 0;
double red_last_time = 0.0;

long get_time_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000L + tv.tv_usec;
}

void send_i2C(const char *message) {
    long t = get_time_us();
    printf("[SEND I2C] %s | time = %ld us\n", message, t);
    usleep(1000);
}


void send_web(const char *message) {
    long t = get_time_us();
    printf("[SEND web] %s | time = %ld us\n", message, t);

    // Création de l'objet racine
    struct json_object *root = json_object_new_object();
    struct json_object *trails = json_object_new_array();

    // Exemple de 3 traînées (tu peux les générer dynamiquement)
    struct {
        int start;
        int end;
        int r, g, b;
    } trail_data[] = {
        //{50, 83, 255, 0, 0},
        //{290, 320, 0, 0, 255},
        {start_red, (start_red + (int)(1/red_period))%400, 255, 0, 0}
    };

    int num_trails = sizeof(trail_data) / sizeof(trail_data[0]);

    for (int i = 0; i < num_trails; i++) {
        struct json_object *trail = json_object_new_object();
        json_object_object_add(trail, "start", json_object_new_int(trail_data[i].start));
        json_object_object_add(trail, "end", json_object_new_int(trail_data[i].end));
        json_object_object_add(trail, "r", json_object_new_int(trail_data[i].r));
        json_object_object_add(trail, "g", json_object_new_int(trail_data[i].g));
        json_object_object_add(trail, "b", json_object_new_int(trail_data[i].b));
        json_object_array_add(trails, trail);
    }

    json_object_object_add(root, "trails", trails);

    // Écriture dans le fichier
    FILE *fp = fopen("wavelight_web/led_state.json", "w");
    if (fp) {
        fputs(json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY), fp);
        fclose(fp);
    } else {
        perror("Erreur lors de l’ouverture de led_state.json");
    }

    json_object_put(root);  // Libération mémoire
    usleep(1000);
}

void send(const char* message) {
    //send_i2C(message);
    send_web(message);
}

void* send_loop(void* arg) {
    const char *message = (const char*)arg;

    while (running) {
        long start = get_time_us();

        send(message);

        red_sum_time += get_time_us() - red_last_time;
        red_last_time = get_time_us();

        if (red_sum_time > red_period*1000000) {
            start_red += 1;
            red_sum_time = 0.0;
        }

        if (start_red > NB_LED) start_red = 0;

        long elapsed = get_time_us() - start;
        long remaining = TARGET_PERIOD_US - elapsed;
        if (remaining > 0)
            usleep(remaining);
        else
            printf("[WARN] Cycle overrun by %ld us\n", -remaining);
    }

    return NULL;
}

// Transform string pace to double pace (number of seconds)
// ex : "4m15s" -> 4 min and 15 sec = 255 sec
double str_to_double(const char *allure_str) {
    int min = 0;
    int sec = 0;

    const char *m_ptr = strchr(allure_str, 'm');
    const char *s_ptr = strchr(allure_str, 's');

    if (m_ptr != NULL) {
        char min_str[10] = {0};
        strncpy(min_str, allure_str, m_ptr - allure_str);
        min = atoi(min_str);
    }

    if (s_ptr != NULL && m_ptr != NULL) {
        char sec_str[10] = {0};
        strncpy(sec_str, m_ptr + 1, s_ptr - m_ptr - 1);
        sec = atoi(sec_str);
    }

    return min * 60 + sec;
}

int main() {
    const char *allure_str = "3m00s";
    double allure_double = str_to_double(allure_str);
    double period = allure_double * 0.4 / 400.0;    // Time between two LEDs
    red_period = period;

    printf("Allure time : %.0f sec\n", allure_double);
    printf("Period : %.3f sec\n", period);

    // Lancement du thread d'envoi
    pthread_t thread_id;
    const char *message = "LED data";
    pthread_create(&thread_id, NULL, send_loop, (void*)message);

    // Laisse tourner le thread pendant 2 secondes
    sleep(18*4*10);

    // Stoppe le thread
    running = 0;
    pthread_join(thread_id, NULL);

    return 0;
}
