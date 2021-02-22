/*
 * VISIBILITY ALGORITHM
 *
 * Brno University of Technology,
 * Parallel and Distributed Algorithms course
 *
 * Martin Cagas <xcagas01@stud.fit.vutbr.cz>
 */

#include "vid.hpp"

using namespace std;

/*
 * THE VISIBILITY PROBLEM
 *
 * This problem is pretty simple. The program is given a vector of altitudes. Using the first
 * element of the vector as the base altitude, it then calculates angles of all points relative to
 * the base altitude. Lastly, it determines which points are visible from the base altitude
 * following a simple rule - the point is visible, if there are no previous angles greater than
 * the current angle.
 */

/*
 * THE PRESCAN ALGORITHM
 *
 * To get a vector of max previous angles for the visibility algorithm, a prescan algorithm is used.
 * However, it was specified in the assignment the standard MPI_Scan() or MPI_Reduce() functions may
 * not be used and therefore the prescan algorithm had to be implemented manually.
 *
 * The prescan algorithm consists of two parts - UpSweep and DownSweep. UpSweep is the reduction
 * algorithm. It traverses a complete binary tree level-by-level from leaves to root and on every
 * node node in a level, it stores the result of a specified binary operation between the two child
 * nodes. After reaching the top, it then replaces the value of the top node with a neutral element
 * of the given binary operation and performs a DownSweep. The DownSweep traverses the tree
 * level-by-level from root to leaves, and on every node in a level, it sends the current node's
 * value to the left, to the right it sends the result of the binary operation between the current
 * node's value and the left node's value. After completing the DownSweep, the leaf nodes contain
 * the result of the prescan operation.
 *
 * The binary operation for the visibility algorithm is the max operation. The neutral element of
 * the max operation on natural numbers is 0.
 *
 * The binary tree in this algorithm is implemented using linear arrays, where the root of every
 * pair of elements is the right element of the two. This approach is sufficient, given the value of
 * the right node doesn't need to be remembered.
 */

int main(int argc, char *argv[])
{
    int total_procs = 0;
    int my_rank = 0;
    const int root = 0;

    // Initialize the MPI, declare required MPI variables
    MPI_Status status;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &total_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    unsigned int base_elems = stoi(argv[1]);
    unsigned int total_elems = base_elems;

    /*
     * Round the count of input elements to the lowest higher power of two, since this is required
     * by the prescan algorithm - it works only on a complete binary tree. If a complete binary tree
     * can't be constructed from the input vector, we'll later pad the missing elements with the
     * neutral element (0).
     */
    // Code block just to scope out the 'counter' variable
    {
        unsigned int counter = 0;

        while (total_elems != 0) {
            total_elems = (total_elems >> 1);
            counter++;
        }

        total_elems = (1 << counter);
    }

    int base_altitude = stoi(argv[2]);

    int overflowed_elems = base_elems % total_procs;
    int base_per_proc = static_cast<unsigned int>(base_elems / total_procs);
    int total_per_proc = static_cast<unsigned int>(total_elems / total_procs);

    int input_matrix[total_procs][total_per_proc] = {0};
    char visibility_matrix[total_procs][total_per_proc] = {0};

    int proc_altitudes[total_per_proc] = {0};
    double proc_angles[total_per_proc] = {0.0};
    double proc_prescan[total_per_proc] = {0.0};
    double proc_prescan_max = 0.0;
    double neighbor_prescan_max = 0.0;
    char proc_visibilities[total_per_proc] = {0};

    memset(&input_matrix, 0, sizeof(input_matrix));
    memset(&visibility_matrix, 0, sizeof(visibility_matrix));

    memset(&proc_altitudes, 0, sizeof(proc_altitudes));
    memset(&proc_angles, 0, sizeof(proc_angles));
    memset(&proc_prescan, 0, sizeof(proc_prescan));
    memset(&proc_visibilities, 0, sizeof(proc_visibilities));

    /*
     * INITIALIZE THE ALGORITHM
     *
     * Assign elements to the input matrix so that there is {elements % rows} rows with
     * base_per_proc + 1 elements and the rest of the rows with base_per_proc elements.
     */

    if (root == my_rank) {
        ifstream infile("input");
        if (true == infile.fail()) {
            return RET_FILE_NOT_FOUND;
        }

        // Fill the first {elements % rows} of rows with numbers from input
        for (int i = 0; i < overflowed_elems; i++) {
            for (int j = 0; j < base_per_proc + 1; j++) {
                infile >> input_matrix[i][j];
            }
        }

        // Fill the rest of the rows with numbers from input
        for (int i = overflowed_elems; i < total_procs; i++) {
            for (int j = 0; j < base_per_proc; j++) {
                infile >> input_matrix[i][j];
            }
        }

        infile.close();
    }

    // Scatter the input matrix into internal vectors of each CPU
    MPI_Scatter(&input_matrix, total_per_proc, MPI_INTEGER, &proc_altitudes, total_per_proc,
                MPI_INTEGER, root, MPI_COMM_WORLD);

    // Compute a vector of angles for each CPU
    for (int i = 0; i < total_per_proc; i++) {
        if ((proc_altitudes[i] == 0) || (proc_altitudes[i] == base_altitude)) {
            proc_angles[i] = 0.0;
        }
        else {
            if (my_rank < overflowed_elems) {
                proc_angles[i] =
                    atan2((proc_altitudes[i] - base_altitude), (i + my_rank * (base_per_proc + 1)));
            }
            else {
                proc_angles[i] = atan2((proc_altitudes[i] - base_altitude),
                                       (i + overflowed_elems * (base_per_proc + 1) +
                                        (my_rank - overflowed_elems) * base_per_proc));
            }
        }
    }

    // Copy the vector of angles into a vector that represents the leaves of a "virtual" binary tree
    // that is processes sequentially on every CPU
    memcpy(&proc_prescan, &proc_angles, sizeof(proc_prescan));

    /*
     * COMPUTE THE PRESCAN VECTOR
     *
     * The prescan algorithm was described in more detail the beginning of the source file. If my
     * explanation wasn't sufficient (which is very likely), I invite anyone to look up the
     * MPI_Reduce() and MPI_Scan() algorithms. Prescan differs from the scan algorithm in shifting
     * the final output vector by 1 to the right (dropping out the right-most element) and inserting
     * a neutral element of the operation on the left-most position of the vector.
     */

    // Sequential reduction (UpSweep) on a CPU
    for (int i = 1; i < total_per_proc; i = (i << 1)) {
        for (int j = total_per_proc - 1; j > 0; j = j - (i << 1)) {
            proc_prescan[j] =
                (proc_prescan[j - i] > proc_prescan[j]) ? proc_prescan[j - i] : proc_prescan[j];
        }
    }

    // Propagating the right-most (root) value from the "virtual" tree of the CPU to the variable
    // that is used to hold values of a processor when in a processor tree
    proc_prescan_max = proc_prescan[total_per_proc - 1];

    unsigned int send_mask = 1;
    unsigned int inactivity_mask = 0;

    int reverse_rank = total_procs - (my_rank + 1);

    // Parallel reduction (UpSweep) on the binary tree
    // The tree is still technically organized as a linear array of nodes (as described earlier)
    while (send_mask < total_procs) {
        if ((reverse_rank & inactivity_mask) == 0) {
            if ((reverse_rank & send_mask) == 0) {
                MPI_Recv(&neighbor_prescan_max, 1, MPI_DOUBLE, my_rank - send_mask, MPI_ANY_TAG,
                         MPI_COMM_WORLD, &status);
                proc_prescan_max = (neighbor_prescan_max > proc_prescan_max) ? neighbor_prescan_max
                                                                             : proc_prescan_max;
            }
            else {
                MPI_Send(&proc_prescan_max, 1, MPI_DOUBLE, my_rank + send_mask, 0, MPI_COMM_WORLD);
            }
        }

        send_mask = (send_mask << 1);
        inactivity_mask = (inactivity_mask << 1) + 1;
    }

    // Replacing the top element of the tree with a neutral element of the operation
    if (my_rank == total_procs - 1) {
        proc_prescan_max = 0.0;
    }

    send_mask = (send_mask >> 1);
    inactivity_mask = (inactivity_mask >> 1);

    // Parallel DownSweep on the binary tree
    // The elements are accesses as during the reduction algorithm, only in reverse
    while (send_mask > 0) {
        if ((reverse_rank & inactivity_mask) == 0) {
            if ((reverse_rank & send_mask) == 0) {
                MPI_Recv(&neighbor_prescan_max, 1, MPI_DOUBLE, my_rank - send_mask, MPI_ANY_TAG,
                         MPI_COMM_WORLD, &status);
                MPI_Send(&proc_prescan_max, 1, MPI_DOUBLE, my_rank - send_mask, 0, MPI_COMM_WORLD);

                proc_prescan_max = (neighbor_prescan_max > proc_prescan_max) ? neighbor_prescan_max
                                                                             : proc_prescan_max;
            }
            else {
                MPI_Send(&proc_prescan_max, 1, MPI_DOUBLE, my_rank + send_mask, 0, MPI_COMM_WORLD);
                MPI_Recv(&proc_prescan_max, 1, MPI_DOUBLE, my_rank + send_mask, MPI_ANY_TAG,
                         MPI_COMM_WORLD, &status);
            }
        }

        send_mask = (send_mask >> 1);
        inactivity_mask = (inactivity_mask >> 1);
    }

    // Copying the CPU's value to the "root" of the internal tree
    proc_prescan[total_per_proc - 1] = proc_prescan_max;

    // Sequantial DownSweep on a CPU
    for (int i = total_per_proc >> 1; i > 0; i = (i >> 1)) {
        for (int j = total_per_proc - 1; j > 0; j = j - (i << 1)) {
            double swap = proc_prescan[j];
            proc_prescan[j] =
                (proc_prescan[j - i] > proc_prescan[j]) ? proc_prescan[j - i] : proc_prescan[j];
            proc_prescan[j - i] = swap;
        }
    }

    /*
     * OUTPUT
     *
     * ... corresponds to the algorithm that reads the input from the input file. The structure of
     * the output matrix corresponds to the input matrix and so the same set of loops can be used.
     *
     */

    // Calculate the visibility of individual elements
    for (int i = 0; i < total_per_proc; i++) {
        if (proc_angles[i] > proc_prescan[i]) {
            proc_visibilities[i] = 1;
        }
    }

    // Have the root CPU gather the data from all CPUs
    MPI_Gather(&proc_visibilities, total_per_proc, MPI_CHAR, &visibility_matrix, total_per_proc,
               MPI_CHAR, root, MPI_COMM_WORLD);

    // Print the output matrix to stdout following the format specified in the assignment
    if (root == my_rank) {
        cout << "_";

        for (int i = 0; i < overflowed_elems; i++) {
            for (int j = 0; j < base_per_proc + 1; j++) {
                if ((i == 0) && (j == 0)) {
                    continue;
                }

                if (visibility_matrix[i][j] == 1) {
                    cout << ",v";
                }
                else {
                    cout << ",u";
                }
            }
        }

        for (int i = overflowed_elems; i < total_procs; i++) {
            for (int j = 0; j < base_per_proc; j++) {
                if ((i == 0) && (j == 0)) {
                    continue;
                }

                if (visibility_matrix[i][j] == 1) {
                    cout << ",v";
                }
                else {
                    cout << ",u";
                }
            }
        }

        cout << endl;
    }

    MPI_Finalize();

    return RET_OK;
}
