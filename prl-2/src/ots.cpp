/*
 * ODD-EVEN TRANSPOSITION SORT
 *
 * Brno University of Technology,
 * Parallel and Distributed Algorithms course
 *
 * Martin Cagas <xcagas01@stud.fit.vutbr.cz>
 */

#include "ots.hpp"

//#define TESTING

using namespace std;

int main(int argc, char *argv[])
{
    int my_rank = 0;
    int proc_count = 0;
    int nr_number = 0;
    int my_number = 0;

#ifdef TESTING
    // promenne pro mereni casu
    auto t1 = std::chrono::high_resolution_clock::now();
    auto t2 = std::chrono::high_resolution_clock::now();
#endif // TESTING

    // Inicializcae MPI, deklarace vsech potrebnuch struktur
    MPI_Status status;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &proc_count);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    /*
     *  CAST 1
     *
     *  - Nacteni souboru 'numbers'
     *  - Rozeslani hodnot vsem operujicim procesorum
     *  - Vypsani nactenych hodnot na stdout dle zadani
     *
     *  - Tuto cast provadi pouze procesor s rankem 0, ostatni cekaji na prideleni cisla
     *
     */
    if (0 == my_rank) {
        int number = 0;
        int proc_id = 0;

        // pokusi se otevrit soubor 'numbers', vrati 404 pokud soubor neexistuje
        ifstream f("numbers");
        if (true == f.fail()) {
            return RET_FILE_NOT_FOUND;
        }

        // cteni hodnot ze souboru
        while (true == f.good()) {
            number = f.get();

            if (false == f.good()) {
#ifndef TESTING
                cout << endl;
#endif // NOT TESTING
                break;
            }

            // zasle prave prectene cislo prave jednomu procesoru
            MPI_Send(&number, 1, MPI_INT, proc_id, 0, MPI_COMM_WORLD);

#ifndef TESTING
            // na standartni vystup provede zapis dle specifikace
            cout << number << " ";
#endif // NOT TESTING

            proc_id++;
        }

        f.close();
    }

    // vsechny procesory prijmou svou hodnotu zaslanou procesorem s rankem 0
    MPI_Recv(&my_number, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    /*
     *  CAST 2
     *
     *  - Vypocet konstant potrebnuch pro provedeni razeni
     *  - Samotny algoritmus razeni provedeny vsemi procesory (jedina skutecne paralelni cast
     * programu)
     *  - Procesory provedou n operaci, kde n je pocet procesoru
     *  - V kazdem kroku stridaji sve cinnosti sude a liche procesory
     *
     */

    // Indexy krajnich procesoru - sude a liche procesory s rankem presahujicim tyto limity
    // zustavaji necinne Jde o nutne osetreni okrajovych prvku - posledni procesor nesmi odesilat
    // zpravu vpravo
    const int odd_limit = 2 * (proc_count / 2) - 1;
    const int even_limit = 2 * ((proc_count - 1) / 2);

#ifdef TESTING
    MPI_Barrier(MPI_COMM_WORLD);

    if (0 == my_rank) {
        t1 = std::chrono::high_resolution_clock::now();
    }
#endif // TESTING

    for (int i = 0; i < proc_count; i++) {
        if (0 == i % 2) {
            /*  SUDY KROK
             *
             *  - Sudy procesor v sudem kroku odesle sve cislo svemu sousedovi po pravici a ceka na
             * odpoved
             *  - Cislo, ktere dostane zpet si zapise, porovnani provadi lichy procesor
             */
            if ((0 == my_rank % 2) && (my_rank < odd_limit)) {
                MPI_Send(&my_number, 1, MPI_INT, my_rank + 1, 0, MPI_COMM_WORLD);
                MPI_Recv(&my_number, 1, MPI_INT, my_rank + 1, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            }
            /*
             *  - Lichy procesor ziska cislo od sudeho procesoru po sve levici
             *  - Pokud je ziskane cislo vetsi, zapamatuje si jej a zpet odesle sve cislo
             *  - V opacnem pripade odesle zpet cislo, ktere prijal
             *  - Prave timto zkusobem probiha razeni
             */
            else if (my_rank <= odd_limit) {
                MPI_Recv(&nr_number, 1, MPI_INT, my_rank - 1, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                // V pripade, ze soused po levici ma vetsi hodnotu, provedu vymenu
                if (nr_number > my_number) {
                    MPI_Send(&my_number, 1, MPI_INT, my_rank - 1, 0, MPI_COMM_WORLD);
                    my_number = nr_number;
                }
                else {
                    MPI_Send(&nr_number, 1, MPI_INT, my_rank - 1, 0, MPI_COMM_WORLD);
                }
            }
        }
        else {
            /*  LICHY KROK
             *
             *  - V lichem kroku je situace prakticky totozna, pouze odesilaji sve hodnoty liche
             * procesory
             */
            if ((1 == my_rank % 2) && (my_rank < even_limit)) {
                MPI_Send(&my_number, 1, MPI_INT, my_rank + 1, 0, MPI_COMM_WORLD);
                MPI_Recv(&my_number, 1, MPI_INT, my_rank + 1, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            }
            else if (my_rank <= even_limit && my_rank != 0) {
                MPI_Recv(&nr_number, 1, MPI_INT, my_rank - 1, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                if (nr_number > my_number) {
                    MPI_Send(&my_number, 1, MPI_INT, my_rank - 1, 0, MPI_COMM_WORLD);
                    my_number = nr_number;
                }
                else {
                    MPI_Send(&nr_number, 1, MPI_INT, my_rank - 1, 0, MPI_COMM_WORLD);
                }
            }
        }
    }

#ifdef TESTING
    MPI_Barrier(MPI_COMM_WORLD);

    if (0 == my_rank) {
        t2 = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();

        cout << duration << " ";
    }
#endif // TESTING

    /*
     *  CAST 3
     *
     *  - Sesbirani vysledku procesorem s rankem 0
     *  - Vypis vysledku na stdout
     *  - Finalizace MPI
     *
     */

    int *sorted_numbers = new int[proc_count];

    for (int i = 1; i < proc_count; i++) {
        if (my_rank == 0) {
            MPI_Recv(&nr_number, 1, MPI_INT, i, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            sorted_numbers[i] = nr_number;
        }
        else if (my_rank == i) {
            MPI_Send(&my_number, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
    }

    if (my_rank == 0) {
        sorted_numbers[0] = my_number;

#ifndef TESTING
        for (int i = 0; i < proc_count; i++) {
            cout << sorted_numbers[i] << endl;
        }
#endif // NOT TESTING
    }

    MPI_Finalize();

    return RET_OK;
}
