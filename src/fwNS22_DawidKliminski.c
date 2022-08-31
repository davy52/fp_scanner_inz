

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// stany systemu  
// przyjeto identyczne mozliwe stany dla ogolnego systemu 
// i poszczegolnych modulow
typedef enum {
    STATE0 = 0,
    STATE1 = 1,
    STATE2 = 2,
    STATE3 = 3,
    STATE4 = 4
} State;

// struktura zestawu danych do transmisji
typedef struct {
    float temp;
    State main_state : 4;
    uint8_t no_modules : 4;
    State *module_states;
} Data;

// Funkjca odpowidzialna za liczenie sumy kontrolnej przeslanej ramki
// wykorzystano inwersje sumy bajtow 
uint32_t checksum_calculate(uint8_t* buff, int buff_size)
{
    uint32_t sum = 0;
    for (int i = 0; i < buff_size; i++) {
        sum += buff[i];
    }

    return ~(sum);
}


// Funkcja przygotowujaca ramke
// 
//      Ramka:
//          data_length         : 1 byte
//          data (min 6 bytes):
//              temp            : 4 bytes
//              main_state      : 1 byte
//              no_modules      : 1 byte
//              module_states   : (0:15) bytes
//          checksum            : 4 bytes
int comm_protTst_prepare(Data* data, uint8_t* buff_ptr, int buff_max_size)
{
    uint8_t* buff = buff_ptr;
    uint32_t checksum = 0;

    // testy wywolania funkcji
    if (buff_max_size < 9)
        return 0;
    if (data->no_modules < 0)
        return 0;
    if (buff_max_size < data->no_modules + 9)
        return 0;


    // zapis dlugosci danych ramki
    *(buff++) = 6 + data->no_modules;
    
    // zapis zmiennoprzecinkowej wartosci do 4 bajtow ramki
    uint8_t* temp = (uint8_t*)&data->temp;
    *(buff++) = temp[0];
    *(buff++) = temp[1];
    *(buff++) = temp[2];
    *(buff++) = temp[3];

    // zapis ogolnego stanu systemu
    *(buff++) = data->main_state;

    // zapis liczby modulow w raporcie
    *(buff++) = data->no_modules;

    // zapis stanow poszczegolnych modulow
    if (data->no_modules > 0) {
        for (int i = 0; i < data->no_modules; i++) {
            *(buff++) = data->module_states[i];
        }
    }

    // obliczanie sumy kontrolnej i zapis na 2 bajtach
    checksum = checksum_calculate(buff_ptr + 1, buff_ptr[0]);
    for (int i = 0; i < 4; i++) {
        *(buff++) = ((uint8_t*)&checksum)[i];
    }

    // zwrot wykorzystanej dlugosci ramki
    return (9 + data->no_modules);
}

// funkcja odpowiedzialna za zapis ramki na wyjsciu (stdout)
void write_frame(uint8_t* frame_buff, int frame_size) {
    for (int i = 0; i < frame_size; i++) {
        fprintf(stdout, "%d.\t %02x\n", i + 1, frame_buff[i]);
    }
}

// funkcja odbierajaca ilosc modulow z wejscia (stdin)
// w razie blednego odczytu wartosci proba jest ponawiana
// do 5-ciu razy
uint8_t no_modules_rcv(void)
{
    int number_recieved = 0;
    int c, i = 1;

    int scan_ret = fscanf(stdin, "%d", &number_recieved);

    while (scan_ret != 1 || number_recieved > 15) {
        while ((c = getchar()) != '\n' && c != EOF);
        scan_ret = fscanf(stdin, "%d", &number_recieved);
        i++; 
        if (i > 4)
            break;
    }

    return number_recieved;
}

// glowna funkcja programu
int main()
{
    //definicja danych i bufora ramki
    int BUFF_MAX_SIZE = 32;
    uint8_t buff[32];
    int no_modules = 0;

    State* module_states;

    Data data = {
        22.4f,
        STATE0,
        0,
        NULL
    };

    // odbior ilosci modulow
    no_modules = no_modules_rcv();
    printf("%d\n\n", no_modules);
    if (no_modules > 0) {
        module_states = (State*)malloc(sizeof(State) * no_modules);

        // generacja losowych stanow modulow
        srand(time(NULL));
        for (int i = 0; i < no_modules; i++) {  
            module_states[i] = rand() % 5;
        }

        data.no_modules = no_modules;
        data.module_states = module_states;
    }

    // generowanie ramki
    int buff_used = comm_protTst_prepare(&data, buff, BUFF_MAX_SIZE);


    // test poprawnosci sumy kontrolnej
    uint8_t* temp = (buff + 1 + buff[0]);
    uint32_t checksum = 0;
    for (int i = 3; i >= 0; i--) {
        checksum |= temp[i] << 8 * i;
    }

    if (checksum == checksum_calculate(buff + 1, buff_used - 3)) {
        // przeslanie ramki na wyjscie (stdout) w razie poprawnej sumy kontrolnej 
        write_frame(buff, buff_used);
    }
    else {
        printf("[ERROR]: Checksum bad, message discarded\n");
    }


    if (no_modules > 0) {
        free(module_states);
    }

    return 0;
}


