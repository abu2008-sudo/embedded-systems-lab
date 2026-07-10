#include <stdio.h>
#include <string.h>

#define MAX_HISTORY 32

typedef struct
{
    char label[16];
    int reading;

    int global_min;
    int global_max;

    int range_min;
    int range_max;
    int avg;

    int history[MAX_HISTORY];
    int buffer_size;
    int histry_indx;
    int history_counts;
} ADC_Channel_t;

ADC_Channel_t ADC_Channel_create(const char *label, int buffer_size)
{
    ADC_Channel_t ch;
    strcpy(ch.label, label);
    ch.reading = 0;

    if (buffer_size > MAX_HISTORY) {
        printf("[ERROR] [%s] Requested buffer_size %d exceeds MAX_HISTORY %d — capping to %d\n",
               label, buffer_size, MAX_HISTORY, MAX_HISTORY);
        buffer_size = MAX_HISTORY;
    }
    if (buffer_size <= 0) {
        printf("[ERROR] [%s] Invalid buffer_size %d — defaulting to 8\n", label, buffer_size);
        buffer_size = 8;
    }
    ch.buffer_size = buffer_size;

    ch.global_min = 4095;
    ch.global_max = 0;

    ch.range_min = 4095;
    ch.range_max = 0;
    ch.avg = 0;

    ch.history_counts = 0;
    ch.histry_indx = 0;

    return ch;
}

// Stub replacing ADC_read() — instead of talking to real hardware,
// takes a simulated reading value fed in from outside (Python via stdin).
void ADC_read_stub(ADC_Channel_t *ch, int simulated_value)
{
    ch->reading = simulated_value;
}

void ADC_update_stats(ADC_Channel_t *ch)
{
    ch->history[ch->histry_indx] = ch->reading;
    ch->histry_indx++;
    if (ch->histry_indx == ch->buffer_size) { ch->histry_indx = 0; }
    if (ch->history_counts < ch->buffer_size) { ch->history_counts++; }
}

void ADC_update_range_stats(ADC_Channel_t *ch)
{
    int range_min = 4095;
    int range_max = 0;
    int sum = 0;

    if (ch->history_counts == 0) return;

    for (int i = 0; i < ch->history_counts; i++) {
        if (ch->history[i] < range_min) { range_min = ch->history[i]; }
        if (ch->history[i] > range_max) { range_max = ch->history[i]; }
        sum += ch->history[i];
    }

    ch->range_min = range_min;
    ch->range_max = range_max;
    ch->avg = sum / ch->history_counts;

    if (range_min < ch->global_min) { ch->global_min = range_min; }
    if (range_max > ch->global_max) { ch->global_max = range_max; }
}

int ADC_get_stat(ADC_Channel_t *ch, const char *stat)
{
    if (strcmp(stat, "range_min") == 0) { return ch->range_min; }
    if (strcmp(stat, "range_max") == 0) { return ch->range_max; }
    if (strcmp(stat, "global_min") == 0) { return ch->global_min; }
    if (strcmp(stat, "global_max") == 0) { return ch->global_max; }
    if (strcmp(stat, "avg") == 0) { return ch->avg; }
    return -1;
}

void ADC_log(int cycle, ADC_Channel_t *ch)
{
    printf("Cycle %2d | [%s] Raw: %4d | Range(N=%d) Min:%4d Max:%4d Avg:%4d | Global Min:%4d Max:%4d\n",
           cycle, ch->label, ch->reading, ch->buffer_size,
           ADC_get_stat(ch, "range_min"),
           ADC_get_stat(ch, "range_max"),
           ADC_get_stat(ch, "avg"),
           ADC_get_stat(ch, "global_min"),
           ADC_get_stat(ch, "global_max"));
}

int main(void)
{
    ADC_Channel_t ch = ADC_Channel_create("Pot1", 8);

    int value;
    int cycle = 0;

    // Reads one integer reading per line from stdin, fed in by the Python
    // stimulus script. Same firmware logic as the real ESP32 version —
    // only ADC_read() is replaced by ADC_read_stub().
    while (scanf("%d", &value) == 1) {
        cycle++;
        ADC_read_stub(&ch, value);
        ADC_update_stats(&ch);
        ADC_update_range_stats(&ch);
        ADC_log(cycle, &ch);
    }

    return 0;
}
